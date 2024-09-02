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
		Fatal("Failed to create sync objects:" + std::string(string_VkResult(result)));
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
		Fatal("Failed to wait sync objects:" + std::string(string_VkResult(result)));
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
		Fatal("Failed to reset sync objects:" + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

#pragma endregion
