#include "MapData.h"

extern std::unique_ptr<PbrRenderer> renderer;
extern std::unique_ptr<PhysicsSystem> physicsSystem;
extern std::unique_ptr<PhysicsScene> physicsScene;

#pragma region MapData

bool MapData::Entity::GetPropertyString(const std::string& key, std::string& value) const
{
	value = getProperty(key);
	return !value.empty();
}

bool MapData::Entity::GetPropertyInteger(const std::string& key, int& value) const
{
	const std::string& literal = getProperty(key);
	if (literal.empty()) return false;

	std::stringstream literalStream(literal);

	int x;
	literalStream >> x;
	if (literalStream.bad()) return false;

	value = x;
	return true;
}

bool MapData::Entity::GetPropertyFloat(const std::string& key, float& value) const
{
	const std::string& literal = getProperty(key);
	if (literal.empty()) return false;

	std::stringstream literalStream(literal);

	float x;
	literalStream >> x;
	if (literalStream.bad()) return false;

	value = x;
	return true;
}

bool MapData::Entity::GetPropertyColor(const std::string& key, glm::vec3& value) const
{
	return getPropertyVec3(key, value);
}

bool MapData::Entity::GetPropertyVector(const std::string& key, glm::vec3& value) const
{
	if (!getPropertyVec3(key, value)) return false;
	// convert from quake direction to engine direction
	std::swap(value.y, value.z);
	value /= 32.0f;
	return true;
}

const std::string& MapData::Entity::getProperty(const std::string& key, const std::string& fallback) const
{
	auto pair = Properties.find(key);
	if (pair == Properties.end()) return fallback;
	return pair->second;
}

bool MapData::Entity::getPropertyVec3(const std::string& key, glm::vec3& value) const
{
	const std::string& literal = getProperty(key);
	if (literal.empty()) return false;

	std::stringstream literalStream(literal);

	float x, y, z;
	literalStream >> x >> y >> z;
	if (literalStream.bad()) return false;

	value = { x, y, z };
	return true;
}

#pragma endregion

#pragma region MapParser

MapParser::MapParser(const std::string& filename)
	: BinaryParser(filename)
{
	parseMap(m_map);
}

void MapParser::parseFace(MapData::Face& face)
{
	if (!readShortString(face.Texture))
		Fatal("Failed to parse Face::texture.");
	if (!readVec3(face.Normal))
		Fatal("Failed to parse Face::normal.");
	if (!readVector(face.Vertices))
		Fatal("Failed to parse Face::numVertices and Face::vertices.");
}

void MapParser::parseBrush(MapData::Brush& brush)
{
	if (!readVector(brush.Vertices))
		Fatal("Failed to parse Brush::numVertices and Brush::vertices.");

	uint16_t numFaces;
	if (!readU16(numFaces))
		Fatal("Failed to parse Brush::numFaces.");
	brush.Faces.resize(numFaces);
	for (int i = 0; i < numFaces; i++)
	{
		parseFace(brush.Faces[i]);
	}
}

void MapParser::parseEntity(MapData::Entity& entity)
{
	uint16_t numProperties;
	if (!readU16(numProperties))
		Fatal("Failed to parse Entity::numProperties.");

	std::string key;
	std::string value;
	for (int i = 0; i < numProperties; i++)
	{
		if (!readShortString(key))
			Fatal("Failed to parse KeyValue::key.");
		if (!readShortString(value))
			Fatal("Failed to parse KeyValue::value.");
		entity.Properties.emplace(key, value);
	}

	uint16_t numBrushes;
	if (!readU16(numBrushes))
		Fatal("Failed to parse Entity::numBrushes.");
	entity.Brushes.resize(numBrushes);
	for (int i = 0; i < numBrushes; i++)
	{
		parseBrush(entity.Brushes[i]);
	}
}

void MapParser::parseMap(MapData::Map& map)
{
	uint16_t numEntities;
	if (!readU16(numEntities))
		Fatal("Failed to parse Map::numEntities.");
	map.Entities.resize(numEntities);
	for (int i = 0; i < numEntities; i++)
	{
		parseEntity(map.Entities[i]);
	}
}

#pragma endregion

#pragma region Brushes

glm::vec3 calculateCenter(const std::vector<MapData::Brush>& brushes)
{
	float numVertices = 0;
	glm::vec3 sum{ 0.0f };
	for (const auto& brush : brushes)
	{
		for (const auto& vertex : brush.Vertices)
		{
			numVertices += 1.0f;
			sum += vertex;
		}
	}
	if (numVertices == 0.0f) return { 0.0f, 0.0f, 0.0f };

	return sum / numVertices;
}

Brushes::Brushes(const std::vector<MapData::Brush>& brushes, BrushType type, PhysicsLayer layer)
	: m_type(type)
	, m_center(calculateCenter(brushes))
{
	switch (type)
	{
	case BrushType::Normal:
	case BrushType::Trigger:
		createMeshes(brushes);
		createColliders(brushes, layer);
		break;
	case BrushType::NoCollision:
		createMeshes(brushes);
		break;
	case BrushType::NoMesh:
		createColliders(brushes, layer);
		break;
	}
}

Brushes::~Brushes()
{
	renderer->WaitDeviceIdle();

	for (physx::PxShape* shape : m_shapes)
		PX_RELEASE(shape);

	for (physx::PxConvexMesh* collider : m_colliders)
		PX_RELEASE(collider);
}

void Brushes::AttachToRigidActor(physx::PxRigidActor* actor)
{
	for (const auto& shape : m_shapes)
		actor->attachShape(*shape);
}

void Brushes::Draw(const glm::mat4& model)
{
	for (const auto& [mesh, material] : m_meshes)
		renderer->Draw(&mesh, model, material);
}

void Brushes::createMeshes(const std::vector<MapData::Brush>& brushes)
{
	std::map<std::string, std::vector<VertexBase>> textureToVertices;
	for (const auto& brush : brushes)
	{
		for (const auto& face : brush.Faces)
		{
			std::vector<VertexBase>& vertices = textureToVertices[face.Texture];
			// triangulate
			for (int k = 1; k < face.Vertices.size() - 1; k++)
			{
				vertices.emplace_back(
					face.Vertices[0].Position - m_center,
					face.Normal,
					face.Vertices[0].TexCoord
				);
				vertices.emplace_back(
					face.Vertices[k].Position - m_center,
					face.Normal,
					face.Vertices[k].TexCoord
				);
				vertices.emplace_back(
					face.Vertices[k + 1].Position - m_center,
					face.Normal,
					face.Vertices[k + 1].TexCoord
				);
			}
		}
	}
	for (const auto& [texture, vertices] : textureToVertices)
	{
		if (texture == "trigger")
		{
			if (m_type != BrushType::Trigger)
			{
				Warning("Trigger faces should only be used on trigger brushes!");
			}
		}
		else
		{
			if (m_type == BrushType::Trigger)
			{
				Warning("Trigger brushes should only contain trigger faces!");
			}
		}

		m_meshes.emplace_back(
			renderer->CreateMesh(vertices),
			renderer->LoadPbrMaterial("materials/" + texture + ".json")
		);
	}
}

void Brushes::createColliders(const std::vector<MapData::Brush>& brushes, PhysicsLayer layer)
{
	const physx::PxFilterData filterData = PhysicsFilterDataFromLayer(layer);

	m_colliders.reserve(brushes.size());
	m_shapes.reserve(brushes.size());
	for (const auto& brush : brushes)
	{
		std::vector<physx::PxVec3> collider;
		collider.reserve(brush.Vertices.size());
		for (const glm::vec3& vertex : brush.Vertices)
		{
			collider.emplace_back(
				vertex.x - m_center.x,
				vertex.y - m_center.y,
				vertex.z - m_center.z
			);
		}

		physx::PxConvexMesh* brushCollider = physicsSystem->CreateConvexMesh(collider.size(), collider.data());
		m_colliders.push_back(brushCollider);

		physx::PxShape* brushShape = physicsScene->CreateShape(
			physx::PxConvexMeshGeometry(brushCollider),
			true,
			m_type == BrushType::Trigger
		);
		brushShape->setQueryFilterData(filterData);
		m_shapes.push_back(brushShape);
	}
}

#pragma endregion