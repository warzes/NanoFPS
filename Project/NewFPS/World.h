#pragma once

#include "Player.h"
#include "Light.h"
#include "LoaderMapData.h"

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
	void Draw(vkr::CommandBufferPtr cmd);

	void UpdateUniformBuffer();

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
};