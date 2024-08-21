#pragma once

struct RenderContextCreateInfo
{
	struct
	{
		std::string_view appName{ "FPS Game" };
		std::string_view engineName{ "Nano VK Engine" };
		uint32_t appVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t engineVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t requireVersion{ VK_MAKE_VERSION(1, 3, 0) };
		bool useValidationLayers{ false };
	} vulkan;
};

struct VulkanQueue final
{
	VkQueue Queue{ nullptr };
	uint32_t QueueFamily{ 0 };
};

struct sBufferingObjects
{
	VkFence RenderFence{ nullptr };
	VkSemaphore PresentSemaphore{ nullptr };
	VkSemaphore RenderSemaphore{ nullptr };
	VkCommandBuffer CommandBuffer{ nullptr };
};


#pragma region VulkanInstance

class VulkanInstance final
{
public:
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();

	void WaitIdle();

	[[nodiscard]] const VkPhysicalDeviceProperties& GetDeviceProperties() const { return PhysicalDeviceProperties; }
	[[nodiscard]] const VkPhysicalDeviceLimits& GetLimits() const { return PhysicalDeviceProperties.limits; }
	[[nodiscard]] const VkPhysicalDeviceFeatures& GetPhysicalFeatures() const { return PhysicalDeviceFeatures; }

	[[nodiscard]] float GetTimestampPeriod() const { return PhysicalDeviceProperties.limits.timestampPeriod; }

	VkInstance Instance{ nullptr };
	VkDebugUtilsMessengerEXT DebugMessenger{ nullptr };

	VkSurfaceKHR Surface{ nullptr };

	VkPhysicalDevice PhysicalDevice{ nullptr };
	VkPhysicalDeviceProperties PhysicalDeviceProperties{};
	VkPhysicalDeviceFeatures PhysicalDeviceFeatures{};

	VkDevice Device{ nullptr };

	VulkanQueue GraphicsQueue;
	VulkanQueue PresentQueue;
	VulkanQueue TransferQueue;
	VulkanQueue ComputeQueue;

	VmaAllocator Allocator{ nullptr };

	VkCommandPool CommandPool{ nullptr };
	VkDescriptorPool DescriptorPool{ nullptr };

	VkFence ImmediateFence{ nullptr };
	VkCommandBuffer ImmediateCommandBuffer{ nullptr };

	// TODO: временно
	vk::Queue m_graphicsQueue;
	vk::PhysicalDevice m_physicalDevice;
	vk::Device m_device;
	vk::SurfaceKHR m_surface;
	vk::CommandPool m_commandPool;
	vk::DescriptorPool m_descriptorPool;
	vk::Fence         m_immediateFence;
	vk::CommandBuffer m_immediateCommandBuffer;
private:
	bool createInstanceAndDevice(const RenderContextCreateInfo& createInfo);
	bool getQueues(vkb::Device& vkbDevice);
	bool createCommandPool();
	bool createDescriptorPool();
	bool createAllocator(uint32_t vulkanApiVersion);
	bool createImmediateContext();
	std::optional<VkFence> createFence(VkFenceCreateFlags flags = {});
	std::optional<VkCommandBuffer> allocateCommandBuffer();
	void temp();
};

#pragma endregion

namespace RenderContext
{
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();
	void BeginFrame();
	void EndFrame();
}