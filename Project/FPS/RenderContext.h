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

class VulkanInstance final
{
public:
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();
	void Finish();

	VkInstance Instance{ nullptr };
	VkDebugUtilsMessengerEXT DebugMessenger{ nullptr };
	
	VkSurfaceKHR Surface{ nullptr };

	VkPhysicalDevice PhysicalDevice{ nullptr };
	VkPhysicalDeviceProperties PhysicalDeviceProperties{};
	
	VkDevice Device{ nullptr };

	VkQueue GraphicsQueue{ nullptr };
	uint32_t GraphicsQueueFamily{ 0 };
	VkQueue PresentQueue{ nullptr };
	uint32_t PresentQueueFamily{ 0 };
	VkQueue ComputeQueue{ nullptr };
	uint32_t ComputeQueueFamily{ 0 };

	VmaAllocator Allocator{ nullptr };
private:
	bool getQueues(vkb::Device& vkbDevice);
};

namespace RenderContext
{
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();
	void BeginFrame();
	void EndFrame();

	[[nodiscard]] VulkanInstance& GetInstance();
}