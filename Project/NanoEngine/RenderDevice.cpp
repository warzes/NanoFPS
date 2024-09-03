#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"

#pragma region RenderDevice

RenderDevice::RenderDevice(EngineApplication& engine, RenderSystem& render)
	: m_engine(engine)
	, m_render(render)
{
}

VkDevice& RenderDevice::Device()
{
	return m_render.GetVkDevice();
}

DeviceQueuePtr RenderDevice::GetGraphicsQueue() const
{
	return m_render.GetVkGraphicsQueue();
}
DeviceQueuePtr RenderDevice::GetPresentQueue() const
{
	return m_render.GetVkPresentQueue();
}
DeviceQueuePtr RenderDevice::GetTransferQueue() const
{
	return m_render.GetVkTransferQueue();
}
DeviceQueuePtr RenderDevice::GetComputeQueue() const
{
	return m_render.GetVkComputeQueue();
}

std::array<uint32_t, 3> RenderDevice::GetAllQueueFamilyIndices() const
{
	return { GetGraphicsQueue()->QueueFamily, GetComputeQueue()->QueueFamily, GetTransferQueue()->QueueFamily };
}

VulkanFencePtr RenderDevice::CreateFence(const FenceCreateInfo& createInfo)
{
	VulkanFencePtr res = std::make_shared<VulkanFence>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

VulkanSemaphorePtr RenderDevice::CreateSemaphore(const SemaphoreCreateInfo& createInfo)
{
	VulkanSemaphorePtr res = std::make_shared<VulkanSemaphore>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

#pragma endregion


