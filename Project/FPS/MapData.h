#pragma once

#include "NanoEngineVK.h"

// https://github.com/chunky-cat/qformats
#include <qformats/map/map.h>
#include <qformats/wad/wad.h>

#pragma region MapData

namespace MapData 
{
	struct Vertex final
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	struct Face final
	{
		std::string         Texture;
		glm::vec3           Normal;
		std::vector<Vertex> Vertices;
	};

	struct Brush final
	{
		std::vector<glm::vec3> Vertices;
		std::vector<Face>      Faces;
	};

	struct Entity final 
	{
		[[nodiscard]] bool GetPropertyString(const std::string& key, std::string& value) const;
		[[nodiscard]] bool GetPropertyInteger(const std::string& key, int& value) const;
		[[nodiscard]] bool GetPropertyFloat(const std::string& key, float& value) const;
		[[nodiscard]] bool GetPropertyColor(const std::string& key, glm::vec3& value) const;
		[[nodiscard]] bool GetPropertyVector(const std::string& key, glm::vec3& value) const;

		std::map<std::string, std::string> Properties;
		std::vector<Brush>                 Brushes;

	private:
		[[nodiscard]] const std::string& getProperty(const std::string& key, const std::string& fallback = {}) const;
		[[nodiscard]] bool getPropertyVec3(const std::string& key, glm::vec3& value) const;
	};

	struct Map final
	{
		std::vector<Entity> Entities;
	};
} // namespace MapData

#pragma endregion

#pragma region MapParser

class MapParser final : private BinaryParser
{
public:
	explicit MapParser(const std::string& filename);

	[[nodiscard]] const MapData::Map& GetMap() const { return m_map; }

private:
	void parseFace(MapData::Face& face);
	void parseBrush(MapData::Brush& brush);
	void parseEntity(MapData::Entity& entity);
	void parseMap(MapData::Map& map);

	MapData::Map m_map;
};

#pragma endregion

#pragma region Brushes

enum class BrushType
{
	Normal,      // mesh + collision
	NoCollision, // mesh
	NoMesh,      // collision
	Trigger      // collision(trigger)
};

class Brushes final
{
public:
	explicit Brushes(const std::vector<MapData::Brush>& brushes, BrushType type = BrushType::Normal, PhysicsLayer layer = PHYSICS_LAYER_0);
	Brushes(const Brushes&) = delete;
	Brushes(Brushes&&) = delete;
	~Brushes();

	Brushes& operator=(const Brushes&) = delete;
	Brushes& operator=(Brushes&&) = delete;

	[[nodiscard]] const BrushType& GetType() const { return m_type; }
	[[nodiscard]] const glm::vec3& GetCenter() const { return m_center; }

	void AttachToRigidActor(physx::PxRigidActor* actor);

	void Draw(const glm::mat4& model);

private:
	void createMeshes(const std::vector<MapData::Brush>& brushes);
	void createColliders(const std::vector<MapData::Brush>& brushes, PhysicsLayer layer);

	BrushType m_type = BrushType::Normal;
	glm::vec3 m_center;

	std::vector<std::pair<VulkanMesh, PbrMaterial*>> m_meshes;

	std::vector<physx::PxConvexMesh*> m_colliders;
	std::vector<physx::PxShape*>      m_shapes;
};

#pragma endregion

