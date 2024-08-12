#include "GameLua.h"
#include "GameScene.h"
#include "GameHUD.h"

extern std::unique_ptr<Scene> scene;
extern std::unique_ptr<GameLua> lua;
extern std::unique_ptr<GameHUD> hud;
extern void ScheduleMapLoad(const std::string& mapName);

GameLua::GameLua()
{
	SetGlobalFunction("loadMap", [](lua_State* L) {
		const std::string mapName = luaL_checkstring(L, 1);
		ScheduleMapLoad(mapName);
		return 0;
		});

	SetGlobalFunction("signal", [](lua_State* L) {
		const std::string name = luaL_checkstring(L, 1);
		Actor* actor = scene->FindActorWithName(name);
		if (actor != nullptr) actor->LuaSignal(L);
		return 0;
		});

	SetGlobalFunction("delay", [](lua_State* L) {
		const auto delay = static_cast<float>(luaL_checknumber(L, 1));
		luaL_checktype(L, 2, LUA_TFUNCTION);
		const int function = lua->CreateReference();
		// using another vector because we might want to queue another delayed function when executing delayed functions and we don't want to add items when iterating the vector
		lua->m_queuedDelayedLuaFunctions.emplace_back(delay, function);
		return 0;
		});

	SetGlobalFunction("playAudio", [](lua_State* L) {
		const std::string path = luaL_checkstring(L, 1);
		//g_Audio->PlayOneShot(path);
		return 0;
		});

	SetGlobalFunction("fadeToColor", [](lua_State* L) {
		const auto duration = static_cast<float>(luaL_checknumber(L, 1));
		const auto r = static_cast<float>(luaL_checknumber(L, 2));
		const auto g = static_cast<float>(luaL_checknumber(L, 3));
		const auto b = static_cast<float>(luaL_checknumber(L, 4));
		hud->FadeToColor(duration, { r, g, b });
		return 0;
		});

	SetGlobalFunction("fadeFromColor", [](lua_State* L) {
		const auto duration = static_cast<float>(luaL_checknumber(L, 1));
		const auto r = static_cast<float>(luaL_checknumber(L, 2));
		const auto g = static_cast<float>(luaL_checknumber(L, 3));
		const auto b = static_cast<float>(luaL_checknumber(L, 4));
		hud->FadeFromColor(duration, { r, g, b });
		return 0;
		});

	SetGlobalFunction("fadeToBlack", [](lua_State* L) {
		const auto duration = static_cast<float>(luaL_checknumber(L, 1));
		hud->FadeToColor(duration, { 0.0f, 0.0f, 0.0f });
		return 0;
		});

	SetGlobalFunction("fadeFromBlack", [](lua_State* L) {
		const auto duration = static_cast<float>(luaL_checknumber(L, 1));
		hud->FadeFromColor(duration, { 0.0f, 0.0f, 0.0f });
		return 0;
		});

	SetGlobalFunction("showObjective", [](lua_State* L) {
		const std::string objective = luaL_checkstring(L, 1);
		hud->ShowObjective(objective);
		return 0;
		});
}

void GameLua::Update(float deltaTime)
{
	std::vector<std::tuple<float, int>> delayedLuaFunctions = std::move(m_queuedDelayedLuaFunctions);
	for (auto [delay, function] : m_delayedLuaFunctions)
	{
		delay -= deltaTime;
		if (delay <= 0.0f)
		{
			PushReference(function);
			PCall(0, 0);
			FreeReference(function);
		}
		else
		{
			delayedLuaFunctions.emplace_back(delay, function);
		}
	}
	m_delayedLuaFunctions = std::move(delayedLuaFunctions);
}