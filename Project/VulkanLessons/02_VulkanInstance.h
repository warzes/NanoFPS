#pragma once

#include "01_VulkanCore.h"

class VulkanInstance final
{
public:
	VulkanInstance();
	~VulkanInstance();

	std::vector<PhysicalDevice> QueryPhysicalDevices();

private:
	VkInstance               m_instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
};