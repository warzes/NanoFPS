#pragma once

#include "Player.h"

struct WorldCreateInfo final
{
	PlayerCreateInfo player;
};

class World final
{
public:
	bool Setup(GameApplication* mame, const WorldCreateInfo& createInfo);
	void Shutdown();

	void Update(float deltaTime);
	void Render();

	Player& GetPlayer() { return m_player; }

	glm::mat4& GetProjection() { return m_perspectiveProjection; }

private:
	GameApplication* m_game;

	Player m_player;
	glm::mat4 m_perspectiveProjection;
};