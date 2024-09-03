#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"

#pragma region VulkanFence

VulkanFence::VulkanFence(RenderDevice& device, const FenceCreateInfo& createInfo)
	: m_device(device)
{
	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags             = createInfo.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkResult result = vkCreateFence(m_device.Device(), &fenceInfo, nullptr, &m_fence);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create fence objects:" + std::string(string_VkResult(result)));
		m_fence = nullptr;
	}
}

VulkanFence::~VulkanFence()
{
	if (m_fence)
		vkDestroyFence(m_device.Device(), m_fence, nullptr);
}

bool VulkanFence::Wait(uint64_t timeout)
{
	if (!IsValid()) return false;

	VkResult result = vkWaitForFences(m_device.Device(), 1, &m_fence, VK_TRUE, timeout);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to wait fence objects:" + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

bool VulkanFence::Reset()
{
	if (!IsValid()) return false;

	VkResult result = vkResetFences(m_device.Device(), 1, &m_fence);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to reset fence objects:" + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

VkResult VulkanFence::Status() const
{
	return vkGetFenceStatus(m_device.Device(), m_fence);
}

#pragma endregion

#pragma region VulkanSemaphore

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

VulkanSemaphore::VulkanSemaphore(RenderDevice& device, const SemaphoreCreateInfo& createInfo)
	: m_device(device)
{
	VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	timelineCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue              = createInfo.initialValue;

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreInfo.pNext = (createInfo.semaphoreType == SEMAPHORE_TYPE_TIMELINE) ? &timelineCreateInfo : nullptr;

	VkResult result = vkCreateSemaphore(m_device.Device(), &semaphoreInfo, nullptr, &m_semaphore);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create semaphore objects:" + std::string(string_VkResult(result)));
		m_semaphore = nullptr;
	}
}

VulkanSemaphore::~VulkanSemaphore()
{
	if (m_semaphore)
		vkDestroySemaphore(m_device.Device(), m_semaphore, nullptr);
}

uint64_t VulkanSemaphore::GetCounterValue() const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return UINT64_MAX;
	}

	return timelineCounterValue();
}

bool VulkanSemaphore::Wait(uint64_t value, uint64_t timeout) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return false;
	}
	return timelineWait(value, timeout);
}

bool VulkanSemaphore::Signal(uint64_t value) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return false;
	}
	return timelineSignal(value);
}

bool VulkanSemaphore::timelineWait(uint64_t value, uint64_t timeout) const
{
	VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	waitInfo.semaphoreCount      = 1;
	waitInfo.pSemaphores         = &m_semaphore;
	waitInfo.pValues             = &value;

	VkResult result = vkWaitSemaphores(m_device.Device(), &waitInfo, timeout);
	if (result != VK_SUCCESS) return false;

	return true;
}

bool VulkanSemaphore::timelineSignal(uint64_t value) const
{
	// See header for explanation
	if (value > m_timelineSignaledValue)
	{
		VkSemaphoreSignalInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
		signalInfo.semaphore             = m_semaphore;
		signalInfo.value                 = value;

		VkResult result = vkSignalSemaphore(m_device.Device(), &signalInfo);
		if (result != VK_SUCCESS) return false;

		m_timelineSignaledValue = value;
	}

	return true;
}

uint64_t VulkanSemaphore::timelineCounterValue() const
{
	uint64_t value = UINT64_MAX;
	VkResult result = vkGetSemaphoreCounterValue(m_device.Device(), m_semaphore, &value);
	// Not clear if value is written to upon failure so prefer safety.
	return (result == VK_SUCCESS) ? value : UINT64_MAX;
}

#pragma endregion