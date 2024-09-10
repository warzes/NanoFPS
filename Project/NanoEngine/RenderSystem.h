#pragma once

#include "RenderDevice.h"

class EngineApplication;
class RenderSystem;

#pragma region VulkanInstance

struct InstanceCreateInfo final
{
	std::string_view         applicationName{ "Game" };
	std::string_view         engineName{ "Nano VK Engine" };
	uint32_t                 appVersion{ VK_MAKE_VERSION(0, 0, 1) };
	uint32_t                 engineVersion{ VK_MAKE_VERSION(0, 0, 1) };
	uint32_t                 requireVersion{ VK_MAKE_VERSION(1, 3, 0) };
	bool                     useValidationLayers{ false };

	ShadingRateMode          supportShadingRateMode = SHADING_RATE_NONE; // TODO: возможно вынести в другой конфиг

	std::vector<const char*> instanceExtensions = {
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
	};

	// TODO: перенести
	// TODO: на релизе убрать лишнее
	// TODO: обязательное для движка вынести в инициализацию
	std::vector<const char*> deviceExtensions = 
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
		//VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,    // Variable rate shading -> ShadingRateMode::SHADING_RATE_VRS
		//VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME,   // Fragment density map -> ShadingRateMode::SHADING_RATE_FDM
		//VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME, // Fragment density map -> ShadingRateMode::SHADING_RATE_FDM
		VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,      // Variable rate shading or Fragment density map
		VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, // TODO: не забыть пофиксить RenderDevice::HasDepthClipEnabled() если изменю
		VK_KHR_MULTIVIEW_EXTENSION_NAME, // RenderDevice::HasMultiView()
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
		VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME,
		VK_EXT_INDEX_TYPE_UINT8_EXTENSION_NAME,

		VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME, // TODO: проверить на 4060. на 1060 почему-то есть
		VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
		VK_KHR_MAINTENANCE2_EXTENSION_NAME,

		VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME, // RenderDevice::HasDescriptorIndexingFeatures()
		//VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME,
		VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
		VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME,

		//VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		//VK_KHR_RAY_QUERY_EXTENSION_NAME,
		//VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		//VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		//VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		//VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		//VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
		//VK_NV_MESH_SHADER_EXTENSION_NAME,
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

	VkInstance                                 instance{ nullptr };
	VkDebugUtilsMessengerEXT                   debugMessenger{ nullptr };

	VkSurfaceKHR                               surface{ nullptr };

	VkPhysicalDevice                           physicalDevice{ nullptr };
	VkPhysicalDeviceProperties                 physicalDeviceProperties{};
	VkPhysicalDeviceFeatures                   physicalDeviceFeatures{};
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures = {};

	uint32_t                                   maxPushDescriptors{ 0 };

	VkDevice                                   device{ nullptr };

	DeviceQueuePtr                             graphicsQueue{ new DeviceQueue() };
	DeviceQueuePtr                             presentQueue{ new DeviceQueue() };
	DeviceQueuePtr                             transferQueue{ new DeviceQueue() };
	DeviceQueuePtr                             computeQueue{ new DeviceQueue() };

	VmaAllocatorPtr                            vmaAllocator{ nullptr };

private:
	std::optional<vkb::Instance> createInstance(const InstanceCreateInfo& createInfo);
	VkSurfaceKHR createSurfaceGLFW(GLFWwindow* window, VkAllocationCallbacks* allocator = nullptr);
	std::optional<vkb::PhysicalDevice> selectDevice(const vkb::Instance& instance, const InstanceCreateInfo& createInfo);
	bool getQueues(vkb::Device& vkbDevice);
	bool initVma();
	bool getDeviceInfo();
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

	[[nodiscard]] VkInstance& GetVkInstance() { return m_instance.instance; }
	[[nodiscard]] VkSurfaceKHR& GetVkSurface() { return m_instance.surface; }
	[[nodiscard]] VkPhysicalDevice& GetVkPhysicalDevice() { return m_instance.physicalDevice; }
	[[nodiscard]] VkDevice& GetVkDevice() { return m_instance.device; }
	[[nodiscard]] VmaAllocatorPtr GetVmaAllocator() { return m_instance.vmaAllocator; }

	[[nodiscard]] const VkPhysicalDeviceFeatures& GetDeviceFeatures() const { return m_instance.physicalDeviceFeatures; }
	[[nodiscard]] const VkPhysicalDeviceLimits& GetDeviceLimits() const { return m_instance.GetDeviceLimits(); }
	[[nodiscard]] float GetDeviceTimestampPeriod() const { return m_instance.GetDeviceTimestampPeriod(); }
	[[nodiscard]] uint32_t GetMaxPushDescriptors() const { return m_instance.maxPushDescriptors; }
	[[nodiscard]] bool PartialDescriptorBindingsSupported() const { return m_instance.descriptorIndexingFeatures.descriptorBindingPartiallyBound; }

	[[nodiscard]] DeviceQueuePtr GetVkGraphicsQueue() { return m_instance.graphicsQueue; }
	[[nodiscard]] DeviceQueuePtr GetVkPresentQueue() { return m_instance.presentQueue; }
	[[nodiscard]] DeviceQueuePtr GetVkTransferQueue() { return m_instance.transferQueue; }
	[[nodiscard]] DeviceQueuePtr GetVkComputeQueue() { return m_instance.computeQueue; }

	[[nodiscard]] RenderDevice& GetRenderDevice() { return m_device; }

private:
	EngineApplication& m_engine;
	
	VulkanInstance m_instance{ *this };
	VulkanSwapchain m_swapChain{ *this };

	RenderDevice m_device{ m_engine, *this };
};

#pragma endregion