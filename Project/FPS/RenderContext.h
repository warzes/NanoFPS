#pragma once

struct RenderContextCreateInfo
{
	struct
	{
		std::string_view appName{ "FPS Game" };
		std::string_view engineName{ "Nano VK Engine" };
		uint32_t appVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t engineVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t requireVersion{ VK_MAKE_VERSION(1, 3, 0) };
		bool useValidationLayers{ false };
	} vulkan;
};

namespace RenderContext
{
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();
	void BeginFrame();
	void EndFrame();
}