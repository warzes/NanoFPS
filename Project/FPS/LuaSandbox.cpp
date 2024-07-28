#include "Base.h"
#include "Core.h"
#include "LuaSandbox.h"

int LuaPackageSearcher(lua_State* L) noexcept
{
	const std::string path = luaL_checkstring(L, 1);
	Print("Loading Lua package: " + path);
	const std::string source = FileSystem::Read(path);
	luaL_loadbuffer(L, source.data(), source.size(), path.c_str());
	return 1;
}

int LuaPrint(lua_State* L) noexcept
{
	const char* message = luaL_checkstring(L, 1);
	Print(message);
	return 0;
}

LuaSandbox::LuaSandbox()
{
	m_luaState = luaL_newstate();
	if(!m_luaState)
		Fatal("Failed to create Lua sandbox.");
	Print(LUA_RELEASE);

	luaL_openlibs(m_luaState);

	setupPackageSearcher();

	SetGlobalFunction("print", LuaPrint);
}

LuaSandbox::~LuaSandbox()
{
	lua_close(m_luaState);
}

void LuaSandbox::setupPackageSearcher()
{
	//
	lua_getglobal(m_luaState, "package");
	// -1: package
	lua_getfield(m_luaState, -1, "searchers");
	// -2: package, -1: packager.searchers

	if (lua_type(m_luaState, -1) != LUA_TTABLE)
	{
		// -2: package, -1: packager.searchers (not table)
		lua_pop(m_luaState, 1);
		// -1: package
		lua_createtable(m_luaState, 1, 0);
		// -2: package, -1: {}
		lua_pushvalue(m_luaState, -1);
		// -3: package, -2: {}, -1: {}
		lua_setfield(m_luaState, -3, "searchers");
		// -2: package, -1: package.searchers
	}

	lua_Integer n = luaL_len(m_luaState, -1);
	lua_pushcfunction(m_luaState, LuaPackageSearcher);
	// -3: package, -2: package.searchers, -1: LuaPackageSearcher
	lua_seti(m_luaState, -2, n + 1);
	// -2: package, -1: packager.searchers

	lua_pop(m_luaState, 2);
	//
}

int LuaSandbox::CreateReference()
{
	return luaL_ref(m_luaState, LUA_REGISTRYINDEX);
}

void LuaSandbox::PushReference(int reference)
{
	lua_rawgeti(m_luaState, LUA_REGISTRYINDEX, reference);
}

void LuaSandbox::FreeReference(int reference)
{
	luaL_unref(m_luaState, LUA_REGISTRYINDEX, reference);
}

int LuaSandbox::GetGlobalVariableReference(const std::string& name)
{
	lua_getglobal(m_luaState, name.c_str());
	return CreateReference();
}

void LuaSandbox::SetGlobalFunction(const std::string& name, lua_CFunction function)
{
	lua_register(m_luaState, name.c_str(), function);
}

void LuaSandbox::CallGlobalFunction(const std::string& name)
{
	const int type = lua_getglobal(m_luaState, name.c_str());

	if (type != LUA_TFUNCTION)
	{
		Error(name + " is not a Lua function.");
		lua_pop(m_luaState, 1);
		return;
	}

	PCall(0, 0);
}

void LuaSandbox::CallGlobalFunction(const std::string& name, const std::string& arg)
{
	const int type = lua_getglobal(m_luaState, name.c_str());

	if (type != LUA_TFUNCTION)
	{
		Error(name + " is not a Lua function.");
		lua_pop(m_luaState, 1);
		return;
	}

	lua_pushstring(m_luaState, arg.c_str());

	PCall(1, 0);
}

void LuaSandbox::DoSource(const std::string& source, const std::string& name)
{
	Print("Executing Lua script " + name);

	if (luaL_loadbuffer(m_luaState, source.data(), source.size(), name.c_str()) != LUA_OK)
	{
		Error("Failed to load Lua script: " + std::string(lua_tostring(m_luaState, -1)));
		lua_pop(m_luaState, 1);
		return;
	}

	PCall(0, 0);
}

void LuaSandbox::DoFile(const std::string& filename)
{
	const std::string source = FileSystem::Read(filename);
	DoSource(source, filename);
}

void LuaSandbox::PCall(int nArgs, int nResults)
{
	if (lua_pcall(m_luaState, nArgs, nResults, 0) != LUA_OK)
	{
		Error("Failed to execute Lua script: " + std::string(lua_tostring(m_luaState, -1)));
		lua_pop(m_luaState, 1);
		return;
	}
}