#pragma once

namespace EngineApp
{
	bool Create();
	void Destroy();

	bool ShouldClose();
	void BeginFrame();
	void EndFrame();
	float GetDeltaTime();
}