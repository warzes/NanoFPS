#pragma once

#pragma region VulkanQueue

struct VulkanQueue final
{
	bool Init(vkb::Device& vkbDevice, vkb::QueueType type);

	VkQueue Queue{ nullptr };
	uint32_t QueueFamily{ 0 };
};

#pragma endregion

#pragma region VulkanSwapchain

class RenderSystem;

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
	RenderSystem& m_render;
	VkSwapchainKHR m_swapChain{ nullptr };
	VkFormat m_swapChainImageFormat{ VK_FORMAT_B8G8R8A8_SRGB }; // TODO: VK_FORMAT_B8G8R8A8_UNORM ???
	std::vector<VkImage> m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkExtent2D m_swapChainExtent{};
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

	VkInstance& GetInstance() { return m_instance; }
	VkSurfaceKHR& GetSurface() { return m_surface; }
	VkPhysicalDevice& GetPhysicalDevice() { return m_physicalDevice; }
	VkDevice& GetDevice() { return m_device; }

	VulkanQueue& GetGraphicsQueue() { return m_graphicsQueue; }
	VulkanQueue& GetPresentQueue() { return m_presentQueue; }
	VulkanQueue& GetTransferQueue() { return m_transferQueue; }
	VulkanQueue& GetComputeQueue() { return m_computeQueue; }

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

	VulkanSwapchain m_swapChain{*this};
};

#pragma endregion
