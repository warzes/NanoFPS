#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"

namespace vkf {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice device)
	: m_device(device)
{
	vkGetPhysicalDeviceProperties(m_device, &m_properties);
	vkGetPhysicalDeviceFeatures(m_device, &m_features);
	vkGetPhysicalDeviceMemoryProperties(m_device, &m_memory);

	m_queueFamilies = QueryWithMethodNoError<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, m_device);
}

PhysicalDeviceType PhysicalDevice::GetDeviceType() const
{
	switch (m_properties.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return PhysicalDeviceType::Other;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return PhysicalDeviceType::IntegratedGPU;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return PhysicalDeviceType::DiscreteGPU;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return PhysicalDeviceType::VirtualGPU;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:            return PhysicalDeviceType::CPU;
	default: break;
	}
	return PhysicalDeviceType::Other;
}

std::vector<VkSurfaceFormatKHR> PhysicalDevice::GetSupportSurfaceFormat(const Surface& surface) const
{
	auto formats = QueryWithMethod<VkSurfaceFormatKHR>(vkGetPhysicalDeviceSurfaceFormatsKHR, m_device, surface);
	if (formats.first != VK_SUCCESS)
	{
		Fatal("vkGetPhysicalDeviceSurfaceFormatsKHR - " + std::string(string_VkResult(formats.first)));
		return {};
	}
	return formats.second;
}

std::vector<VkPresentModeKHR> PhysicalDevice::GetSupportPresentMode(const Surface& surface) const
{
	auto presentModes = QueryWithMethod<VkPresentModeKHR>(vkGetPhysicalDeviceSurfacePresentModesKHR, m_device, surface);
	if (presentModes.first != VK_SUCCESS)
	{
		Fatal("vkGetPhysicalDeviceSurfacePresentModesKHR - " + std::string(string_VkResult(presentModes.first)));
		return {};
	}
	return presentModes.second;
}

std::vector<VkExtensionProperties> PhysicalDevice::GetDeviceExtensions() const
{
	auto retExtensions = QueryWithMethod<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, m_device, nullptr);
	if (retExtensions.first != VK_SUCCESS)
	{
		Fatal("vkEnumerateDeviceExtensionProperties - " + std::string(string_VkResult(retExtensions.first)));
		return {};
	}
	return retExtensions.second;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::GetQueueFamilyProperties() const
{
	return m_queueFamilies;
}

std::optional<uint32_t> PhysicalDevice::FindGraphicsQueueFamilyIndex() const
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
	{
		if (m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			return i;
		}
	}
	Fatal("Graphics Queue Family not find");
	return std::nullopt;
}

std::optional<std::pair<uint32_t, uint32_t>> PhysicalDevice::FindGraphicsAndPresentQueueFamilyIndex(const Surface& surface) const
{
	std::optional<uint32_t> graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex();
	if (!graphicsQueueFamilyIndex.has_value()) return std::nullopt;

	if (GetSurfaceSupportKHR(graphicsQueueFamilyIndex.value(), surface))
	{
		// the first graphicsQueueFamilyIndex does also support presents
		return std::make_pair(graphicsQueueFamilyIndex.value(), graphicsQueueFamilyIndex.value());
	}

	// the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both graphics and present
	for (size_t i = 0; i < m_queueFamilies.size(); i++)
	{
		if ((m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && GetSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
		{
			return std::make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));
		}
	}

	// there's nothing like a single family index that supports both graphics and present -> look for an other family index that supports present
	for (size_t i = 0; i < m_queueFamilies.size(); i++)
	{
		if (GetSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
		{
			return std::make_pair(graphicsQueueFamilyIndex.value(), static_cast<uint32_t>(i));
		}
	}

	Fatal("Could not find queues for both graphics or present -> terminating");
	return std::nullopt;
}

bool PhysicalDevice::IsSupportSwapChain(const Surface& surface) const
{
	const auto formats = GetSupportSurfaceFormat(surface);
	const auto presentModes = GetSupportPresentMode(surface);

	return !formats.empty() && !presentModes.empty();
}

bool PhysicalDevice::GetSurfaceSupportKHR(uint32_t queueFamilyIndex, const Surface& surface) const
{
	VkBool32 supported{ VK_FALSE };
	if (surface != nullptr)
	{
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(m_device, queueFamilyIndex, surface, &supported);
		if (result != VK_SUCCESS)
		{
			Fatal("vkGetPhysicalDeviceSurfaceSupportKHR - " + std::string(string_VkResult(result)));
			return false;
		}
	}
	return supported == VK_TRUE;
}

} // namespace vkf