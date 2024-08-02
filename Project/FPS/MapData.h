#pragma once

#include "NanoEngineVK.h"

#pragma region MapData

namespace MapData {
	struct Vertex {
		glm::vec3 Position;
		glm::vec2 TexCoord;
	};

	struct Face {
		std::string         Texture;
		glm::vec3           Normal;
		std::vector<Vertex> Vertices;
	};

	struct Brush {
		std::vector<glm::vec3> Vertices;
		std::vector<Face>      Faces;
	};

	struct Entity {
		std::map<std::string, std::string> Properties;
		std::vector<Brush>                 Brushes;

		[[nodiscard]] bool GetPropertyString(const std::string& key, std::string& value) const;

		[[nodiscard]] bool GetPropertyInteger(const std::string& key, int& value) const;

		[[nodiscard]] bool GetPropertyFloat(const std::string& key, float& value) const;

		[[nodiscard]] bool GetPropertyColor(const std::string& key, glm::vec3& value) const;

		[[nodiscard]] bool GetPropertyVector(const std::string& key, glm::vec3& value) const;

	private:
		[[nodiscard]] const std::string& GetProperty(const std::string& key, const std::string& fallback = {}) const;

		[[nodiscard]] bool GetPropertyVec3(const std::string& key, glm::vec3& value) const;
	};

	struct Map {
		std::vector<Entity> Entities;
	};
} // namespace MapData

#pragma endregion

#pragma region MapParser

class MapParser : private BinaryParser {
public:
	explicit MapParser(const std::string& filename);

	[[nodiscard]] const MapData::Map& GetMap() const { return m_map; }

private:
	void ParseFace(MapData::Face& face);

	void ParseBrush(MapData::Brush& brush);

	void ParseEntity(MapData::Entity& entity);

	void ParseMap(MapData::Map& map);

	MapData::Map m_map;
};

#pragma endregion

#pragma region Brushes

enum class BrushType {
	Normal,      // mesh + collision
	NoCollision, // mesh
	NoMesh,      // collision
	Trigger      // collision(trigger)
};

class Brushes {
public:
	explicit Brushes(const std::vector<MapData::Brush>& brushes, BrushType type = BrushType::Normal, PhysicsLayer layer = PHYSICS_LAYER_0);

	~Brushes();

	Brushes(const Brushes&) = delete;

	Brushes& operator=(const Brushes&) = delete;

	Brushes(Brushes&&) = delete;

	Brushes& operator=(Brushes&&) = delete;

	[[nodiscard]] const BrushType& GetType() const { return m_type; }

	[[nodiscard]] const glm::vec3& GetCenter() const { return m_center; }

	void AttachToRigidActor(physx::PxRigidActor* actor);

	void Draw(const glm::mat4& model);

private:
	void CreateMeshes(const std::vector<MapData::Brush>& brushes);

	void CreateColliders(const std::vector<MapData::Brush>& brushes, PhysicsLayer layer);

	BrushType m_type = BrushType::Normal;
	glm::vec3 m_center;

	std::vector<std::pair<VulkanMesh, PbrMaterial*>> m_meshes;

	std::vector<physx::PxConvexMesh*> m_colliders;
	std::vector<physx::PxShape*>      m_shapes;
};

#pragma endregion

