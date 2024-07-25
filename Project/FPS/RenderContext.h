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

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct sBufferingObjects
{
	VkFence RenderFence{ nullptr };
	VkSemaphore PresentSemaphore{ nullptr };
	VkSemaphore RenderSemaphore{ nullptr };
	VkCommandBuffer CommandBuffer{ nullptr };
};

struct VulkanFrameInfo
{
	const VkRenderPassBeginInfo* PrimaryRenderPassBeginInfo = nullptr;
	uint32_t BufferingIndex = 0;
	VkCommandBuffer CommandBuffer{ nullptr };
};

class VulkanInstance final
{
public:
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();

	void WaitIdle();

	[[nodiscard]] size_t GetNumBuffering();

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

	VkCommandPool CommandPool{ nullptr };
	VkDescriptorPool DescriptorPool{ nullptr };
	VkFence ImmediateFence{ nullptr };
	VkCommandBuffer ImmediateCommandBuffer{ nullptr };
	std::array<sBufferingObjects, 3> BufferingObjects; // TODO: в свапчаин

private:
	bool getQueues(vkb::Device& vkbDevice);
	bool createCommandPool();
	bool createDescriptorPool();
	bool createAllocator(uint32_t vulkanApiVersion);
	bool createImmediateContext();
	std::optional<VkFence> createFence(VkFenceCreateFlags flags = {});
	std::optional<VkCommandBuffer> allocateCommandBuffer();
	bool createBufferingObjects();
	std::optional<VkSemaphore> createSemaphore();
};

class VulkanSwapChain final
{
public:
	bool Create();
	void Destroy();

	VulkanFrameInfo BeginFrame();
	void EndFrame();

	VkSwapchainKHR SwapChain{ nullptr };
	VkFormat SwapChainImageFormat{ VK_FORMAT_B8G8R8A8_SRGB }; // TODO: VK_FORMAT_B8G8R8A8_UNORM ???
	std::vector<VkImage> SwapChainImages;
	std::vector<VkImageView> SwapChainImageViews;
	VkExtent2D SwapChainExtent{};

	VkRenderPass PrimaryRenderPass{ nullptr };

	std::vector<VkFramebuffer> PrimaryFramebuffers;
	bool FramebufferResized = false; // TODO: менять если изменилось разрешение экрана

	std::vector<VkRenderPassBeginInfo> PrimaryRenderPassBeginInfos;

	uint32_t CurrentSwapchainImageIndex = 0;
	uint32_t CurrentBufferingIndex = 0;

private:
	bool createSwapChain(uint32_t width, uint32_t height);
	bool createPrimaryRenderPass();
	bool createPrimaryFramebuffers();
	void cleanupSurfaceSwapchainAndImageViews();
	void cleanupPrimaryFramebuffers();
	bool recreateSwapchain();
	VkResult tryAcquiringNextSwapchainImage();
	void acquireNextSwapchainImage();
	void waitAndResetFence(VkFence fence, uint64_t timeout = 100'000'000);
	void submitToGraphicsQueue(const VkSubmitInfo& submitInfo, VkFence fence);
};

namespace RenderContext
{
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();
	void BeginFrame();
	void EndFrame();

	[[nodiscard]] VulkanInstance& GetInstance();
	[[nodiscard]] VulkanSwapChain& GetSwapChain();


}