#pragma once

#include "RenderResources.h"

class EngineApplication;
class RenderSystem;
class DeviceQueue;

#pragma region RenderDevice

class RenderDevice final
{
public:
	RenderDevice(EngineApplication& engine, RenderSystem& render);

	VkDevice& GetVkDevice();

	DeviceQueuePtr GetGraphicsQueue() const;
	DeviceQueuePtr GetPresentQueue() const;
	DeviceQueuePtr GetTransferQueue() const;
	DeviceQueuePtr GetComputeQueue() const;
	uint32_t GetGraphicsQueueCount() const;
	uint32_t GetComputeQueueCount() const;
	uint32_t GetTransferQueueCount() const;
	std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

	VulkanFencePtr CreateFence(const FenceCreateInfo& createInfo);
	VulkanSemaphorePtr CreateSemaphore(const SemaphoreCreateInfo& createInfo);

	VulkanCommandPoolPtr CreateCommandPool(const CommandPoolCreateInfo& createInfo) { return {}; }

	bool AllocateCommandBuffer(VulkanCommandPoolPtr pool, VulkanCommandBufferPtr& commandBuffer, uint32_t resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT, uint32_t samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT) { return true; }
	void FreeCommandBuffer(VulkanCommandBufferPtr& commandBuffer) {}

private:
	EngineApplication& m_engine;
	RenderSystem& m_render;
};

#pragma endregion