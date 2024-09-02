#pragma once

#pragma region VulkanInstance

#pragma endregion

#pragma region VulkanQueue

struct VulkanQueue final
{
	bool Init(vkb::Device& vkbDevice, vkb::QueueType type);

	VkQueue Queue{ nullptr };
	uint32_t QueueFamily{ 0 };
};

#pragma endregion


#pragma region RenderSystem

struct RenderCreateInfo final
{
	struct Vulkan final
	{
		std::string_view appName{ "FPS Game" };
		std::string_view engineName{ "Nano VK Engine" };
		uint32_t appVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t engineVersion{ VK_MAKE_VERSION(0, 0, 1) };
		uint32_t requireVersion{ VK_MAKE_VERSION(1, 3, 0) };
		bool useValidationLayers{ false };
	} vulkan;
};

class EngineApplication;

class RenderSystem final
{
public:
	RenderSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const RenderCreateInfo& createInfo);
	void Shutdown();

	void TestDraw();

private:
	VkSurfaceKHR createSurfaceGLFW(VkAllocationCallbacks* allocator = nullptr);
	bool getQueues(vkb::Device& vkbDevice);

	EngineApplication& m_engine;

	VkInstance m_instance{ nullptr };
	VkDebugUtilsMessengerEXT m_debugMessenger{ nullptr };

	VkSurfaceKHR m_surface{ nullptr };

	VkPhysicalDevice m_physicalDevice{ nullptr };
	VkPhysicalDeviceProperties m_physicalDeviceProperties{};
	VkPhysicalDeviceFeatures m_physicalDeviceFeatures{};

	VkDevice m_device{ nullptr };

	VulkanQueue m_graphicsQueue{};
	VulkanQueue m_presentQueue{};
	VulkanQueue m_transferQueue{};
	VulkanQueue m_computeQueue{};
};

#pragma endregion
