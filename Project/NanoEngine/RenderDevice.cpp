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

VkDevice& RenderDevice::GetVkDevice()
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

uint32_t RenderDevice::GetGraphicsQueueCount() const
{
	return GetGraphicsQueue()->QueueFamily;
}

uint32_t RenderDevice::GetComputeQueueCount() const
{
	return GetComputeQueue()->QueueFamily;
}

uint32_t RenderDevice::GetTransferQueueCount() const
{
	return GetTransferQueue()->QueueFamily;
}

std::array<uint32_t, 3> RenderDevice::GetAllQueueFamilyIndices() const
{
	return { GetGraphicsQueueCount(), GetComputeQueueCount(), GetTransferQueueCount() };
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


