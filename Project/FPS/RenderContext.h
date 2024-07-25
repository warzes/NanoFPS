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

struct VulkanRenderPassOptions
{
	bool ForPresentation = false;
	bool PreserveDepth = false;
	bool ShaderReadsDepth = false;
};

class VulkanInstance final
{
public:
	bool Create(const RenderContextCreateInfo& createInfo);
	void Destroy();

	void WaitIdle();

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
	std::array<sBufferingObjects, 3> BufferingObjects;

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

	bool Resize();

	void BeginFrame();
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




	VkImage DepthImage{ nullptr };
	VkDeviceMemory DepthImageMemory{ nullptr };
	VkImageView DepthImageView{ nullptr };
	std::vector<VkCommandBuffer> CommandBuffers;
	size_t CurrentFrame{ 0 };
	std::vector<VkSemaphore> ImageAvailableSemaphores;
	std::vector<VkSemaphore> RenderFinishedSemaphores;
	std::vector<VkFence> InFlightFences;

private:
	bool createSwapChain(uint32_t width, uint32_t height);
	bool createPrimaryRenderPass();
	bool createPrimaryFramebuffers();





	bool createDepthResources();
	std::optional<VkFormat> findDepthFormat();
	bool createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory); // TODO: вынести в отдельный класс
	std::optional<VkImageView> createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels); // TODO: вынести в отдельный класс
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties); // TODO: вынести в отдельный класс
	bool createFramebuffers();
	bool createRenderPass();
	void destroySwapChain();

	bool createCommandBuffers();
	bool createSyncObjects();

	uint32_t m_imageIndex = 0;
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