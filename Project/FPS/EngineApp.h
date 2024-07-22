#pragma once

struct EngineCreateInfo final
{
	WindowCreateInfo window;
	RenderContextCreateInfo renderContext;
};

namespace EngineApp
{
	bool Create(const EngineCreateInfo& createInfo);
	void Destroy();

	bool ShouldClose();

	void Update();
	void BeginFrame();
	void EndFrame();

	float GetDeltaTime();

	void Exit();
}