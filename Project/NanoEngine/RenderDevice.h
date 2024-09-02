#pragma once

#include "RenderResources.h"

class EngineApplication;
class RenderSystem;

#pragma region RenderDevice

class RenderDevice final
{
public:
	RenderDevice(EngineApplication& engine, RenderSystem& render);

	VkDevice& Device();

	VulkanFencePtr CreateFence(const FenceCreateInfo& createInfo);
	void DestroyFence(VulkanFencePtr& fence);

private:

	EngineApplication& m_engine;
	RenderSystem& m_render;
};

#pragma endregion