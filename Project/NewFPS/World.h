#pragma once

#include "Player.h"
#include "Light.h"

struct WorldCreateInfo final
{
	PlayerCreateInfo player;
};

class World final
{
public:
	bool Setup(GameApplication* game, const WorldCreateInfo& createInfo);
	void Shutdown();

	void Update(float deltaTime);
	void Draw(vkr::CommandBufferPtr cmd);

	Player& GetPlayer() { return m_player; }
	DirectionalLight& GetMainLight() { return m_mainLight; }
	std::vector<GameEntity>& GetEntities() { return m_entities; }

private:
	bool setupEntities();
	bool addTestEntities();
	GameApplication* m_game;
	Player m_player;
	DirectionalLight m_mainLight;

	// Entities
	vkr::DescriptorSetLayoutPtr m_drawObjectSetLayout;
	vkr::PipelineInterfacePtr m_drawObjectPipelineInterface;
	vkr::GraphicsPipelinePtr m_drawObjectPipeline;
	std::vector<GameEntity> m_entities;
};