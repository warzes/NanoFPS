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

	std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

	VulkanFencePtr CreateFence(const FenceCreateInfo& createInfo);
	VulkanSemaphorePtr CreateSemaphore(const SemaphoreCreateInfo& createInfo);

private:

	EngineApplication& m_engine;
	RenderSystem& m_render;
};

#pragma endregion