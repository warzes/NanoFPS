#pragma once

#include "Player.h"
#include "Light.h"
#include "LoaderMapData.h"
#include "TestPhysicalBox.h"

struct WorldCreateInfo final
{
	PlayerCreateInfo player;
	std::string_view startMapName = "test.te3";
};

class World final
{
public:
	bool Setup(GameApplication* game, const WorldCreateInfo& createInfo);
	void Shutdown();

	void Update(float deltaTime);
	void FixedUpdate(float fixedDeltaTime);
	void Draw(vkr::CommandBufferPtr cmd);

	void UpdateUniformBuffer();

	glm::mat4 GetViewProjectionMatrix();

	Player& GetPlayer() { return m_player; }
	DirectionalLight& GetMainLight() { return m_mainLight; }
	std::vector<GameEntity>& GetEntities() { return m_entities; }

private:
	bool setupPipelineEntities();
	bool addTestEntities();
	bool loadMap(std::string_view mapFileName);

	GameApplication* m_game;
	Player m_player;
	DirectionalLight m_mainLight;
	glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
	float m_horizFovDegrees = 60.0f;
	float m_vertFovDegrees = 36.98f;
	float m_aspect = 1.0f;
	const float CAMERA_DEFAULT_NEAR_CLIP = 0.1f;
	const float CAMERA_DEFAULT_FAR_CLIP = 10000.0f;

	// Entities
	vkr::DescriptorSetLayoutPtr m_drawObjectSetLayout;
	vkr::PipelineInterfacePtr m_drawObjectPipelineInterface;
	vkr::GraphicsPipelinePtr m_drawObjectPipeline;
	std::vector<GameEntity> m_entities;

	// Map
	LoaderMapData m_mapData;

	struct MeshBild final
	{
		vkr::TriMesh mesh;
		std::string diffuseTextureFileName;
	};
	std::vector<MeshBild> m_mapMeshes;

	TestPhysicalBox m_phBox;
};