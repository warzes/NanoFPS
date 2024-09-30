#include "stdafx.h"
#include "World.h"
#include "GameApp.h"

bool World::Setup(GameApplication* game, const WorldCreateInfo& createInfo)
{
	m_game = game;
	if (!m_player.Setup(m_game, createInfo.player)) return false;

	return true;
}

void World::Shutdown()
{
	m_player.Shutdown();
}

void World::Update(float deltaTime)
{
	m_player.Update(deltaTime);

	{
		const float FOV = 60.0f;
		const float ASPECT_RATIO = m_game->GetWindowAspect();
		m_perspectiveProjection = glm::perspective(FOV, ASPECT_RATIO, CAMERA_DEFAULT_NEAR_CLIP, CAMERA_DEFAULT_FAR_CLIP);
	}
}

void World::Render()
{
}
