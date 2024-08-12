#pragma once

#include "NanoEngineVK.h"

class GameLua final : public LuaSandbox
{
public:
	GameLua();
	GameLua(GameLua&&) = delete;
	GameLua(const GameLua&) = delete;

	GameLua& operator=(const GameLua&) = delete;
	GameLua& operator=(GameLua&&) = delete;

	void Update(float deltaTime);

private:
	std::vector<std::tuple<float, int>> m_delayedLuaFunctions;
	std::vector<std::tuple<float, int>> m_queuedDelayedLuaFunctions;
};