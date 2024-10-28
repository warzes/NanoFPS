#include "03_VulkanPhysicalDevice.h"

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device)
	: m_device(device)
{
	auto retExtensions = QueryWithMethod<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, m_device, nullptr);
	CheckResult(retExtensions.first, "vkEnumerateDeviceExtensionProperties");
	m_extensions = retExtensions.second;
	vkGetPhysicalDeviceProperties(m_device, &m_properties);
	vkGetPhysicalDeviceMemoryProperties(m_device, &m_memory);
}