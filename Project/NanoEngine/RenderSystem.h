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
		VK_KHR_MAINTENANCE3_EXTENSION_NAME,
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

#pragma region VulkanSurface

class VulkanSurface final
{
public:
	VulkanSurface(RenderSystem& render);

	[[nodiscard]] bool Setup();

	[[nodiscard]] VkSurfaceCapabilitiesKHR              GetCapabilities() const;
	[[nodiscard]] const std::vector<VkSurfaceFormatKHR> GetSurfaceFormats() const { return m_surfaceFormats; }

	[[nodiscard]] uint32_t GetMinImageWidth() const;
	[[nodiscard]] uint32_t GetMinImageHeight() const;
	[[nodiscard]] uint32_t GetMinImageCount() const;
	[[nodiscard]] uint32_t GetMaxImageWidth() const;
	[[nodiscard]] uint32_t GetMaxImageHeight() const;
	[[nodiscard]] uint32_t GetMaxImageCount() const;

	[[nodiscard]] uint32_t GetCurrentImageWidth() const;
	[[nodiscard]] uint32_t GetCurrentImageHeight() const;

	static constexpr uint32_t kInvalidExtent = std::numeric_limits<uint32_t>::max();

private:
	RenderSystem&                   m_render;
	std::vector<VkSurfaceFormatKHR> m_surfaceFormats;
	std::vector<VkPresentModeKHR>   m_presentModes;
};

#pragma endregion

#pragma region VulkanSwapchain

struct VulkanSwapChainCreateInfo final
{
	VulkanSurface*      surface = nullptr;
	Queue*              queue = nullptr;
	ShadingRatePattern* shadingRatePattern = nullptr;
	uint32_t            width = 0;
	uint32_t            height = 0;
	Format              colorFormat = FORMAT_UNDEFINED;
	Format              depthFormat = FORMAT_UNDEFINED;
	uint32_t            imageCount = 0;
	uint32_t            arrayLayerCount = 1;
	PresentMode         presentMode = PRESENT_MODE_IMMEDIATE;
};

// TODO: неудачно получается. VulkanSwapChainCreateInfo используется для создания ресурса свапчаина. а SwapChainCreateInfo - настройка пользователем. надо как-то переделать
struct SwapChainCreateInfo final
{
	Format   colorFormat = FORMAT_B8G8R8A8_UNORM;
	Format   depthFormat = FORMAT_UNDEFINED;
	uint32_t imageCount = 2;
};

class VulkanSwapChain final
{
public:
	VulkanSwapChain(RenderSystem& render);

	bool Setup(const VulkanSwapChainCreateInfo& createInfo);
	void Shutdown2();

	uint32_t GetWidth() const { return m_createInfo.width; }
	uint32_t GetHeight() const { return m_createInfo.height; }
	uint32_t GetImageCount() const { return m_createInfo.imageCount; }
	Format   GetColorFormat() const { return m_createInfo.colorFormat; }
	Format   GetDepthFormat() const { return m_createInfo.depthFormat; }

	Result GetColorImage(uint32_t imageIndex, Image** ppImage) const;
	Result GetDepthImage(uint32_t imageIndex, Image** ppImage) const;
	Result GetRenderPass(uint32_t imageIndex, AttachmentLoadOp loadOp, RenderPass** ppRenderPass) const;
	Result GetRenderTargetView(uint32_t imageIndex, AttachmentLoadOp loadOp, RenderTargetView** ppView) const;
	Result GetDepthStencilView(uint32_t imageIndex, AttachmentLoadOp loadOp, DepthStencilView** ppView) const;

	// Convenience functions - returns empty object if index is invalid
	ImagePtr            GetColorImage(uint32_t imageIndex) const;
	ImagePtr            GetDepthImage(uint32_t imageIndex) const;
	RenderPassPtr       GetRenderPass(uint32_t imageIndex, AttachmentLoadOp loadOp = ATTACHMENT_LOAD_OP_CLEAR) const;
	RenderTargetViewPtr GetRenderTargetView(uint32_t imageIndex, AttachmentLoadOp loadOp = ATTACHMENT_LOAD_OP_CLEAR) const;
	DepthStencilViewPtr GetDepthStencilView(uint32_t imageIndex, AttachmentLoadOp loadOp = ATTACHMENT_LOAD_OP_CLEAR) const;

	Result AcquireNextImage(
		uint64_t   timeout,    // Nanoseconds
		Semaphore* pSemaphore, // Wait sempahore
		Fence*     pFence,     // Wait fence
		uint32_t*  pImageIndex);

	Result Present(
		uint32_t                imageIndex,
		uint32_t                waitSemaphoreCount,
		const Semaphore* const* ppWaitSemaphores);

	uint32_t GetCurrentImageIndex() const { return m_currentImageIndex; }

	VkSwapchainKHR& GetVkSwapChain() { return m_swapChain; }

	// OLD =>

	[[nodiscard]] bool Resize(uint32_t width, uint32_t height);
	void Close();

	VkFormat GetFormat();


	size_t GetImageNum() const { return m_swapChainImages.size(); }
	size_t GetImageViewNum() const { return m_swapChainImageViews.size(); }
	VkImageView& GetImageView(size_t i);
	const VkExtent2D& GetExtent() const { return m_swapChainExtent; }

private:
	// Make these protected since D3D12's swapchain resize will need to call them
	void   destroyColorImages();
	Result createDepthImages();
	void   destroyDepthImages();
	Result createRenderPasses();
	void   destroyRenderPasses();
	Result createRenderTargets();
	void   destroyRenderTargets();

	Result acquireNextImageInternal(
		uint64_t   timeout,    // Nanoseconds
		Semaphore* pSemaphore, // Wait sempahore
		Fence*     pFence,     // Wait fence
		uint32_t*  pImageIndex); // TODO: перенесети в AcquireNextImage

	Result presentInternal(
		uint32_t                imageIndex,
		uint32_t                waitSemaphoreCount,
		const Semaphore* const* ppWaitSemaphores);// TODO: перенесети в Present

	RenderSystem&                    m_render;
	VulkanSwapChainCreateInfo              m_createInfo;

	VkSwapchainKHR                   m_swapChain{ nullptr };

	QueuePtr                         m_queue;
	std::vector<ImagePtr>            m_depthImages;
	std::vector<ImagePtr>            m_colorImages;
	std::vector<RenderTargetViewPtr> m_clearRenderTargets;
	std::vector<RenderTargetViewPtr> m_loadRenderTargets;
	std::vector<DepthStencilViewPtr> m_clearDepthStencilViews;
	std::vector<DepthStencilViewPtr> m_loadDepthStencilViews;
	std::vector<RenderPassPtr>       m_clearRenderPasses;
	std::vector<RenderPassPtr>       m_loadRenderPasses;

	// Keeps track of the image index returned by the last AcquireNextImage call.
	uint32_t                         m_currentImageIndex = 0;



	VkFormat                 m_colorFormat{ VK_FORMAT_B8G8R8A8_UNORM };
	std::vector<VkImage>     m_swapChainImages;
	std::vector<VkImageView> m_swapChainImageViews;
	VkExtent2D               m_swapChainExtent{};
};

#pragma endregion

#pragma region ImGui

class ImGuiImpl
{
public:
	ImGuiImpl(RenderSystem& render);

	bool Setup();
	void Shutdown();
	void NewFrame();
	void Render(CommandBuffer* pCommandBuffer);
	void ProcessEvents();

private:
	Result initApiObjects();
	void setColorStyle();
	void newFrameApi();

	RenderSystem& m_render;
	DescriptorPoolPtr m_pool;
};

#pragma endregion

#pragma region RenderSystem

struct RenderCreateInfo final
{
	InstanceCreateInfo  instance;
	SwapChainCreateInfo swapChain;
	bool                showImgui{ false };
};

class RenderSystem final
{
	friend class RenderDevice;
	friend class EngineApplication;
public:
	RenderSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const RenderCreateInfo& createInfo);
	void Shutdown();

	void Update();
	void TestDraw();
	void DrawDebugInfo();

	void DrawImGui(CommandBuffer* pCommandBuffer);

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
	[[nodiscard]] VulkanSwapChain& GetSwapChain() { return m_swapChain; }

	[[nodiscard]] QueuePtr GetGraphicsQueue() { return GetRenderDevice().GetGraphicsQueue(); }

	[[nodiscard]] Rect GetScissor() const;
	[[nodiscard]] Viewport GetViewport(float minDepth = 0.0f, float maxDepth = 1.0f) const;

	[[nodiscard]] uint32_t GetUIWidth() const;
	[[nodiscard]] uint32_t GetUIHeight() const;
	[[nodiscard]] float2 GetNormalizedDeviceCoordinates(int32_t x, int32_t y) const;

	[[nodiscard]] EngineApplication& GetEngine() { return m_engine; }

	Result WaitIdle();

private:
	[[nodiscard]] bool createSwapChains(const SwapChainCreateInfo& swapChain);
	void resize();

	EngineApplication&  m_engine;
	
	VulkanInstance      m_instance{ *this };
	VulkanSurface       m_surface{ *this };
	VulkanSwapChain     m_swapChain{ *this };
	SwapChainCreateInfo m_swapChainInfo{}; // TODO: на самом деле не нужен - вся эта инфа есть в m_swapChain, нужно получать оттуда.

	RenderDevice        m_device{ m_engine, *this };
	ImGuiImpl           m_imgui{ *this };

	bool                m_showImgui{ false };
};

#pragma endregion