#pragma once

class LuaSandbox
{
public:
	LuaSandbox();
	LuaSandbox(const LuaSandbox&) = delete;
	LuaSandbox(LuaSandbox&&) = delete;
	~LuaSandbox();

	LuaSandbox& operator=(const LuaSandbox&) = delete;
	LuaSandbox& operator=(LuaSandbox&&) = delete;

	int CreateReference();

	void PushReference(int reference);

	void FreeReference(int reference);

	int GetGlobalVariableReference(const std::string& name);

	void SetGlobalFunction(const std::string& name, lua_CFunction function);

	void CallGlobalFunction(const std::string& name);

	void CallGlobalFunction(const std::string& name, const std::string& arg);

	void DoSource(const std::string& source, const std::string& name);

	void DoFile(const std::string& filename);

	void PCall(int nArgs, int nResults);

private:
	void setupPackageSearcher();

	lua_State* m_luaState = nullptr;
};