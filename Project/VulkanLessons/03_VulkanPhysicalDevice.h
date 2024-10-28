#pragma once

#include "01_VulkanCore.h"

class PhysicalDevice final
{
public:
	PhysicalDevice() = delete;
	PhysicalDevice(VkPhysicalDevice device);

private:
	VkPhysicalDevice                   m_device{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties         m_properties;
	std::vector<VkExtensionProperties> m_extensions;
	VkPhysicalDeviceMemoryProperties   m_memory;
};