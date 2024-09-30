#include "stdafx.h"
#include "World.h"
#include "GameApp.h"

bool World::Setup(GameApplication* game, const WorldCreateInfo& createInfo)
{
	m_game = game;
	if (!m_player.Setup(m_game, createInfo.player)) return false;
	if (!m_mainLight.Setup(m_game)) return false;

	return true;
}

void World::Shutdown()
{
	m_mainLight.Shutdown();
	m_player.Shutdown();
}

void World::Update(float deltaTime)
{
	m_player.Update(deltaTime);
	m_mainLight.Update(deltaTime);
}

void World::Render()
{
	// TODO:
}
