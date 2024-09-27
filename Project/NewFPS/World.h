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

	glm::mat4& GetView() { return m_perspectiveView; }
	glm::mat4& GetProjection() { return m_perspectiveProjection; }
	glm::mat4 GetVP() { return m_perspectiveProjection * glm::scale(float3(1, -1, 1)) * m_perspectiveView; }

private:
	GameApplication* m_game;

	Player m_player;
	glm::mat4 m_perspectiveView;
	glm::mat4 m_perspectiveProjection;
};