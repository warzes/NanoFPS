#pragma once

#include "RenderDevice.h"

class EngineApplication;
class RenderSystem;

#pragma region VulkanQueue

struct VulkanQueue final
{
	bool Init(vkb::Device& vkbDevice, vkb::QueueType type);

	VkQueue Queue{ nullptr };
	uint32_t QueueFamily{ 0 };
};

#pragma endregion

#pragma region VulkanInstance

struct InstanceCreateInfo final
{
	std::string_view         applicationName{ "Game" };
	std::string_view         engineName{ "Nano VK Engine" };
	uint32_t                 appVersion{ VK_MAKE_VERSION(0, 0, 1) };
	uint32_t                 engineVersion{ VK_MAKE_VERSION(0, 0, 1) };
	uint32_t                 requireVersion{ VK_MAKE_VERSION(1, 3, 0) };
	bool                     useValidationLayers{ false };

	std::vector<const char*> vulkanExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};
};

class VulkanInstance final
{
public:
	VulkanInstance(RenderSystem& render);

	[[nodiscard]] bool Setup(const InstanceCreateInfo& createInfo, GLFWwindow* window);
	void Shutdown();

	[[nodiscard]] const VkPhysicalDeviceLimits& GetDeviceLimits() const { return physicalDeviceProperties.limits; }
	[[nodiscard]] float GetDeviceTimestampPeriod() const { return physicalDeviceProperties.limits.timestampPeriod; }
	[[nodiscard]] const char* GetDeviceName() const { return physicalDeviceProperties.deviceName; }

	VkInstance                 instance{ nullptr };
	VkDebugUtilsMessengerEXT   debugMessenger{ nullptr };

	VkSurfaceKHR               surface{ nullptr };

	VkPhysicalDevice           physicalDevice{ nullptr };
	VkPhysicalDeviceProperties physicalDeviceProperties{};
	VkPhysicalDeviceFeatures   physicalDeviceFeatures{};

	VkDevice                   device{ nullptr };

	VulkanQueue                graphicsQueue{};
	VulkanQueue                presentQueue{};
	VulkanQueue                transferQueue{};
	VulkanQueue                computeQueue{};

private:
	std::optional<vkb::Instance> createInstance(const InstanceCreateInfo& createInfo);
	VkSurfaceKHR createSurfaceGLFW(GLFWwindow* window, VkAllocationCallbacks* allocator = nullptr);
	std::optional<vkb::PhysicalDevice> selectDevice(const vkb::Instance& instance);
	bool getQueues(vkb::Device& vkbDevice);
	RenderSystem& m_render;
};

#pragma endregion

#pragma region VulkanSwapchain

class VulkanSwapchain final
{
public:
	VulkanSwapchain(RenderSystem& render);

	[[nodiscard]] bool Setup(uint32_t width, uint32_t height);
	void Shutdown();

	VkFormat GetFormat();

	VkSwapchainKHR& Get() { return m_swapChain; }
	size_t GetImageNum() const { return m_swapChainImages.size(); }
	size_t GetImageViewNum() const { return m_swapChainImageViews.size(); }
	VkImageView& GetImageView(size_t i);
	const VkExtent2D& GetExtent() const { return m_swapChainExtent; }

private:
	RenderSystem&            m_render;
	VkSwapchainKHR           m_swapChain{ nullptr };
	VkFormat                 m_colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
	std::vector<VkImage>     m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkExtent2D               m_swapChainExtent{};
};

#pragma endregion

#pragma region RenderSystem

struct RenderCreateInfo final
{
	InstanceCreateInfo instance;
};

class RenderSystem final
{
	friend class RenderDevice;
public:
	RenderSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const RenderCreateInfo& createInfo);
	void Shutdown();

	void TestDraw();

	VkInstance& GetVkInstance() { return m_instance.instance; }
	VkSurfaceKHR& GetVkSurface() { return m_instance.surface; }
	VkPhysicalDevice& GetVkPhysicalDevice() { return m_instance.physicalDevice; }
	VkDevice& GetVkDevice() { return m_instance.device; }

	VulkanQueue& GetVkGraphicsQueue() { return m_instance.graphicsQueue; }
	VulkanQueue& GetVkPresentQueue() { return m_instance.presentQueue; }
	VulkanQueue& GetVkTransferQueue() { return m_instance.transferQueue; }
	VulkanQueue& GetVkComputeQueue() { return m_instance.computeQueue; }

	RenderDevice& GetDevice() { return m_device; }

private:
	EngineApplication& m_engine;
	
	VulkanInstance m_instance{ *this };
	VulkanSwapchain m_swapChain{ *this };

	RenderDevice m_device{ m_engine, *this };
};

#pragma endregion