#pragma once

#include "Application.h"

struct QueueFamilyIndices final
{
	bool isComplete() { return graphicsFamilyHasValue && presentFamilyHasValue; }

	uint32_t graphicsFamily;
	uint32_t presentFamily;
	bool graphicsFamilyHasValue = false;
	bool presentFamilyHasValue = false;
};

struct SwapChainSupportDetails final
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

class Test001 final : public Application
{
public:
	ApplicationCreateInfo GetConfig() final { return {}; }

	bool Init() final;
	void Close() final;
	void Update() final;
	void Draw() final;

	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);


private:
	bool checkValidationLayerSupport();
	std::vector<const char*> getRequiredExtensions();
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	bool hasGLFWRequiredInstanceExtensions();
	bool isDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

	bool createInstance();
	bool setupDebugMessenger();
	bool createSurface();
	bool pickPhysicalDevice();
	bool createLogicalDevice();
	bool getDeviceQueue();
	bool createCommandPool();

	VkInstance                 m_instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT   m_debugMessenger{ VK_NULL_HANDLE };
	VkSurfaceKHR               m_surface{ VK_NULL_HANDLE };
	VkPhysicalDevice           m_physicalDevice{ VK_NULL_HANDLE };
	VkPhysicalDeviceProperties m_properties{};
	VkDevice                   m_device{ VK_NULL_HANDLE };
	uint32_t                   m_graphicsQueueFamilyIndex{ 0 };
	VkQueue                    m_graphicsQueue{ VK_NULL_HANDLE };
	uint32_t                   m_presentQueueFamilyIndex{ 0 };
	VkQueue                    m_presentQueue{ VK_NULL_HANDLE };
	VkCommandPool              m_commandPool{ VK_NULL_HANDLE };
};