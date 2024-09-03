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


