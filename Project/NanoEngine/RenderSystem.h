#pragma once

#pragma region RenderSystem

struct RenderCreateInfo final
{
};

class EngineApplication;

class RenderSystem final
{
public:
	RenderSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const RenderCreateInfo& createInfo);
	void Shutdown();

	void TestDraw();

private:
	EngineApplication& m_engine;
};

#pragma endregion
