#pragma once

#include "VulkanCore.h"

namespace vkf {

class PhysicalDevice final
{
public:
	PhysicalDevice() = delete;
	PhysicalDevice(VkPhysicalDevice device);

	operator VkPhysicalDevice() const { return m_device; }

	const VkPhysicalDeviceFeatures&         GetFeatures() const { return m_features; }
	const VkPhysicalDeviceProperties&       GetProperties() const { m_properties; }
	const VkPhysicalDeviceLimits&           GetLimits() const { m_properties.limits; }
	const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_memory; }
	std::string                             GetDeviceName() const { return m_properties.deviceName; }
	PhysicalDeviceType                      GetDeviceType() const;
	std::vector<VkSurfaceFormatKHR>         GetSupportSurfaceFormat(const Surface& surface) const;
	std::vector<VkPresentModeKHR>           GetSupportPresentMode(const Surface& surface) const;

	std::vector<VkExtensionProperties>      GetDeviceExtensions() const;
	const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;

	std::optional<uint32_t> FindGraphicsQueueFamilyIndex() const;
	std::optional<std::pair<uint32_t, uint32_t>> FindGraphicsAndPresentQueueFamilyIndex(const Surface& surface) const;

	bool IsSupportSwapChain(const Surface& surface) const;

	bool GetSurfaceSupportKHR(uint32_t queueFamilyIndex, const Surface& surface) const;

private:
	VkPhysicalDevice                     m_device{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties           m_properties{};
	VkPhysicalDeviceFeatures             m_features{};
	VkPhysicalDeviceMemoryProperties     m_memory{};
	std::vector<VkQueueFamilyProperties> m_queueFamilies;
};

} // namespace vkf