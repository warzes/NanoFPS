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
	void Render();

	Player& GetPlayer() { return m_player; }
	DirectionalLight& GetMainLight() { return m_mainLight; }

private:
	GameApplication* m_game;

	Player m_player;

	DirectionalLight m_mainLight;
};