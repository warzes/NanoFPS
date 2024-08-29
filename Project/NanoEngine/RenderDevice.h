#pragma once

#pragma region Gpu

namespace grfx {

	namespace internal {

		struct GpuCreateInfo
		{
			int32_t featureLevel = InvalidValue<int32_t>(); // D3D12
			void* pApiObject = nullptr;
		};

	} // namespace internal

	class Gpu
		: public grfx::InstanceObject<grfx::internal::GpuCreateInfo>
	{
	public:
		Gpu() {}
		virtual ~Gpu() {}

		const char* GetDeviceName() const { return mDeviceName.c_str(); }
		grfx::VendorId GetDeviceVendorId() const { return mDeviceVendorId; }

		virtual uint32_t GetGraphicsQueueCount() const = 0;
		virtual uint32_t GetComputeQueueCount() const = 0;
		virtual uint32_t GetTransferQueueCount() const = 0;

	protected:
		std::string    mDeviceName;
		grfx::VendorId mDeviceVendorId = grfx::VENDOR_ID_UNKNOWN;
	};

} // namespace grfx

#pragma endregion

#pragma region Swapchain

namespace grfx {

	struct SurfaceCreateInfo
	{
		grfx::Gpu* pGpu = nullptr;
#if defined(_LINUX_WAYLAND)
		struct wl_display* display;
		struct wl_surface* surface;
#elif defined(_LINUX_XCB)
		xcb_connection_t* connection;
		xcb_window_t          window;
#elif defined(_LINUX_XLIB)
		Display* dpy;
		Window                window;
#elif defined(_WIN32)
		HINSTANCE             hinstance;
		HWND                  hwnd;
#elif defined(_ANDROID)
		android_app* androidAppContext;
#endif
	};

	//! @class Surface
	//!
	//!
	class Surface
		: public grfx::InstanceObject<grfx::SurfaceCreateInfo>
	{
	public:
		Surface() {}
		virtual ~Surface() {}

		virtual uint32_t GetMinImageWidth() const = 0;
		virtual uint32_t GetMinImageHeight() const = 0;
		virtual uint32_t GetMinImageCount() const = 0;
		virtual uint32_t GetMaxImageWidth() const = 0;
		virtual uint32_t GetMaxImageHeight() const = 0;
		virtual uint32_t GetMaxImageCount() const = 0;

		static constexpr uint32_t kInvalidExtent = std::numeric_limits<uint32_t>::max();

		virtual uint32_t GetCurrentImageWidth() const { return kInvalidExtent; }
		virtual uint32_t GetCurrentImageHeight() const { return kInvalidExtent; }
	};

	// -------------------------------------------------------------------------------------------------

	//! @struct SwapchainCreateInfo
	//!
	//! NOTE: The member \b imageCount is the minimum image count.
	//!       On Vulkan, the actual number of images created by
	//!       the swapchain may be greater than this value.
	//!
	struct SwapchainCreateInfo
	{
		grfx::Queue* pQueue = nullptr;
		grfx::Surface* pSurface = nullptr;
		grfx::ShadingRatePattern* pShadingRatePattern = nullptr;
		uint32_t                  width = 0;
		uint32_t                  height = 0;
		grfx::Format              colorFormat = grfx::FORMAT_UNDEFINED;
		grfx::Format              depthFormat = grfx::FORMAT_UNDEFINED;
		uint32_t                  imageCount = 0;
		uint32_t                  arrayLayerCount = 1; // Used only for XR swapchains.
		grfx::PresentMode         presentMode = grfx::PRESENT_MODE_IMMEDIATE;
	};

	//! @class Swapchain
	//!
	//!
	class Swapchain
		: public grfx::DeviceObject<grfx::SwapchainCreateInfo>
	{
	public:
		Swapchain() {}
		virtual ~Swapchain() {}

		bool         IsHeadless() const;
		uint32_t     GetWidth() const { return mCreateInfo.width; }
		uint32_t     GetHeight() const { return mCreateInfo.height; }
		uint32_t     GetImageCount() const { return mCreateInfo.imageCount; }
		grfx::Format GetColorFormat() const { return mCreateInfo.colorFormat; }
		grfx::Format GetDepthFormat() const { return mCreateInfo.depthFormat; }

		Result GetColorImage(uint32_t imageIndex, grfx::Image** ppImage) const;
		Result GetDepthImage(uint32_t imageIndex, grfx::Image** ppImage) const;
		Result GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderPass** ppRenderPass) const;
		Result GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::RenderTargetView** ppView) const;
		Result GetDepthStencilView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp, grfx::DepthStencilView** ppView) const;

		// Convenience functions - returns empty object if index is invalid
		grfx::ImagePtr            GetColorImage(uint32_t imageIndex) const;
		grfx::ImagePtr            GetDepthImage(uint32_t imageIndex) const;
		grfx::RenderPassPtr       GetRenderPass(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;
		grfx::RenderTargetViewPtr GetRenderTargetView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;
		grfx::DepthStencilViewPtr GetDepthStencilView(uint32_t imageIndex, grfx::AttachmentLoadOp loadOp = grfx::ATTACHMENT_LOAD_OP_CLEAR) const;

		Result AcquireNextImage(
			uint64_t         timeout,    // Nanoseconds
			grfx::Semaphore* pSemaphore, // Wait sempahore
			grfx::Fence* pFence,     // Wait fence
			uint32_t* pImageIndex);

		Result Present(
			uint32_t                      imageIndex,
			uint32_t                      waitSemaphoreCount,
			const grfx::Semaphore* const* ppWaitSemaphores);

		uint32_t GetCurrentImageIndex() const { return mCurrentImageIndex; }

		// D3D12 only, will return ERROR_FAILED on Vulkan
		virtual Result Resize(uint32_t width, uint32_t height) = 0;

	protected:
		virtual Result Create(const grfx::SwapchainCreateInfo* pCreateInfo) override;
		virtual void   Destroy() override;
		friend class grfx::Device;

		// Make these protected since D3D12's swapchain resize will need to call them
		void   DestroyColorImages();
		Result CreateDepthImages();
		void   DestroyDepthImages();
		Result CreateRenderPasses();
		void   DestroyRenderPasses();
		Result CreateRenderTargets();
		void   DestroyRenderTargets();

	private:
		virtual Result AcquireNextImageInternal(
			uint64_t         timeout,    // Nanoseconds
			grfx::Semaphore* pSemaphore, // Wait sempahore
			grfx::Fence* pFence,     // Wait fence
			uint32_t* pImageIndex) = 0;

		virtual Result PresentInternal(
			uint32_t                      imageIndex,
			uint32_t                      waitSemaphoreCount,
			const grfx::Semaphore* const* ppWaitSemaphores) = 0;

		Result AcquireNextImageHeadless(
			uint64_t         timeout,
			grfx::Semaphore* pSemaphore,
			grfx::Fence* pFence,
			uint32_t* pImageIndex);

		Result PresentHeadless(
			uint32_t                      imageIndex,
			uint32_t                      waitSemaphoreCount,
			const grfx::Semaphore* const* ppWaitSemaphores);

		std::vector<grfx::CommandBufferPtr> mHeadlessCommandBuffers;

	protected:
		grfx::QueuePtr                         mQueue;
		std::vector<grfx::ImagePtr>            mDepthImages;
		std::vector<grfx::ImagePtr>            mColorImages;
		std::vector<grfx::RenderTargetViewPtr> mClearRenderTargets;
		std::vector<grfx::RenderTargetViewPtr> mLoadRenderTargets;
		std::vector<grfx::DepthStencilViewPtr> mClearDepthStencilViews;
		std::vector<grfx::DepthStencilViewPtr> mLoadDepthStencilViews;
		std::vector<grfx::RenderPassPtr>       mClearRenderPasses;
		std::vector<grfx::RenderPassPtr>       mLoadRenderPasses;

		// Keeps track of the image index returned by the last AcquireNextImage call.
		uint32_t mCurrentImageIndex = 0;
	};

} // namespace grfx

#pragma endregion

#pragma region Device

namespace grfx {

	struct DeviceCreateInfo
	{
		grfx::Gpu* pGpu = nullptr;
		uint32_t                 graphicsQueueCount = 0;
		uint32_t                 computeQueueCount = 0;
		uint32_t                 transferQueueCount = 0;
		std::vector<std::string> vulkanExtensions = {};      // [OPTIONAL] Additional device extensions
		const void* pVulkanDeviceFeatures = nullptr; // [OPTIONAL] Pointer to custom VkPhysicalDeviceFeatures
		bool                     multiView = false;   // [OPTIONAL] Whether to allow multiView features
		ShadingRateMode          supportShadingRateMode = SHADING_RATE_NONE;
	};

	//! @class Device
	//!
	//!
	class Device
		: public grfx::InstanceObject<grfx::DeviceCreateInfo>
	{
	public:
		Device() {}
		virtual ~Device() {}

		bool isDebugEnabled() const;

		grfx::Api GetApi() const;

		grfx::GpuPtr GetGpu() const { return mCreateInfo.pGpu; }

		const char* GetDeviceName() const;
		grfx::VendorId GetDeviceVendorId() const;

		Result CreateBuffer(const grfx::BufferCreateInfo* pCreateInfo, grfx::Buffer** ppBuffer);
		void   DestroyBuffer(const grfx::Buffer* pBuffer);

		Result CreateCommandPool(const grfx::CommandPoolCreateInfo* pCreateInfo, grfx::CommandPool** ppCommandPool);
		void   DestroyCommandPool(const grfx::CommandPool* pCommandPool);

		Result CreateComputePipeline(const grfx::ComputePipelineCreateInfo* pCreateInfo, grfx::ComputePipeline** ppComputePipeline);
		void   DestroyComputePipeline(const grfx::ComputePipeline* pComputePipeline);

		Result CreateDepthStencilView(const grfx::DepthStencilViewCreateInfo* pCreateInfo, grfx::DepthStencilView** ppDepthStencilView);
		void   DestroyDepthStencilView(const grfx::DepthStencilView* pDepthStencilView);

		Result CreateDescriptorPool(const grfx::DescriptorPoolCreateInfo* pCreateInfo, grfx::DescriptorPool** ppDescriptorPool);
		void   DestroyDescriptorPool(const grfx::DescriptorPool* pDescriptorPool);

		Result CreateDescriptorSetLayout(const grfx::DescriptorSetLayoutCreateInfo* pCreateInfo, grfx::DescriptorSetLayout** ppDescriptorSetLayout);
		void   DestroyDescriptorSetLayout(const grfx::DescriptorSetLayout* pDescriptorSetLayout);

		Result CreateDrawPass(const grfx::DrawPassCreateInfo* pCreateInfo, grfx::DrawPass** ppDrawPass);
		Result CreateDrawPass(const grfx::DrawPassCreateInfo2* pCreateInfo, grfx::DrawPass** ppDrawPass);
		Result CreateDrawPass(const grfx::DrawPassCreateInfo3* pCreateInfo, grfx::DrawPass** ppDrawPass);
		void   DestroyDrawPass(const grfx::DrawPass* pDrawPass);

		Result CreateFence(const grfx::FenceCreateInfo* pCreateInfo, grfx::Fence** ppFence);
		void   DestroyFence(const grfx::Fence* pFence);

		Result CreateShadingRatePattern(const grfx::ShadingRatePatternCreateInfo* pCreateInfo, grfx::ShadingRatePattern** ppShadingRatePattern);
		void   DestroyShadingRatePattern(const grfx::ShadingRatePattern* pShadingRatePattern);

		Result CreateFullscreenQuad(const grfx::FullscreenQuadCreateInfo* pCreateInfo, grfx::FullscreenQuad** ppFullscreenQuad);
		void   DestroyFullscreenQuad(const grfx::FullscreenQuad* pFullscreenQuad);

		Result CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline);
		Result CreateGraphicsPipeline(const grfx::GraphicsPipelineCreateInfo2* pCreateInfo, grfx::GraphicsPipeline** ppGraphicsPipeline);
		void   DestroyGraphicsPipeline(const grfx::GraphicsPipeline* pGraphicsPipeline);

		Result CreateImage(const grfx::ImageCreateInfo* pCreateInfo, grfx::Image** ppImage);
		void   DestroyImage(const grfx::Image* pImage);

		Result CreateMesh(const grfx::MeshCreateInfo* pCreateInfo, grfx::Mesh** ppMesh);
		void   DestroyMesh(const grfx::Mesh* pMesh);

		Result CreatePipelineInterface(const grfx::PipelineInterfaceCreateInfo* pCreateInfo, grfx::PipelineInterface** ppPipelineInterface);
		void   DestroyPipelineInterface(const grfx::PipelineInterface* pPipelineInterface);

		Result CreateQuery(const grfx::QueryCreateInfo* pCreateInfo, grfx::Query** ppQuery);
		void   DestroyQuery(const grfx::Query* pQuery);

		Result CreateRenderPass(const grfx::RenderPassCreateInfo* pCreateInfo, grfx::RenderPass** ppRenderPass);
		Result CreateRenderPass(const grfx::RenderPassCreateInfo2* pCreateInfo, grfx::RenderPass** ppRenderPass);
		Result CreateRenderPass(const grfx::RenderPassCreateInfo3* pCreateInfo, grfx::RenderPass** ppRenderPass);
		void   DestroyRenderPass(const grfx::RenderPass* pRenderPass);

		Result CreateRenderTargetView(const grfx::RenderTargetViewCreateInfo* pCreateInfo, grfx::RenderTargetView** ppRenderTargetView);
		void   DestroyRenderTargetView(const grfx::RenderTargetView* pRenderTargetView);

		Result CreateSampledImageView(const grfx::SampledImageViewCreateInfo* pCreateInfo, grfx::SampledImageView** ppSampledImageView);
		void   DestroySampledImageView(const grfx::SampledImageView* pSampledImageView);

		Result CreateSampler(const grfx::SamplerCreateInfo* pCreateInfo, grfx::Sampler** ppSampler);
		void   DestroySampler(const grfx::Sampler* pSampler);

		Result CreateSamplerYcbcrConversion(const grfx::SamplerYcbcrConversionCreateInfo* pCreateInfo, grfx::SamplerYcbcrConversion** ppConversion);
		void   DestroySamplerYcbcrConversion(const grfx::SamplerYcbcrConversion* pConversion);

		Result CreateSemaphore(const grfx::SemaphoreCreateInfo* pCreateInfo, grfx::Semaphore** ppSemaphore);
		void   DestroySemaphore(const grfx::Semaphore* pSemaphore);

		Result CreateShaderModule(const grfx::ShaderModuleCreateInfo* pCreateInfo, grfx::ShaderModule** ppShaderModule);
		void   DestroyShaderModule(const grfx::ShaderModule* pShaderModule);

		Result CreateStorageImageView(const grfx::StorageImageViewCreateInfo* pCreateInfo, grfx::StorageImageView** ppStorageImageView);
		void   DestroyStorageImageView(const grfx::StorageImageView* pStorageImageView);

		Result CreateSwapchain(const grfx::SwapchainCreateInfo* pCreateInfo, grfx::Swapchain** ppSwapchain);
		void   DestroySwapchain(const grfx::Swapchain* pSwapchain);

		Result CreateTextDraw(const grfx::TextDrawCreateInfo* pCreateInfo, grfx::TextDraw** ppTextDraw);
		void   DestroyTextDraw(const grfx::TextDraw* pTextDraw);

		Result CreateTexture(const grfx::TextureCreateInfo* pCreateInfo, grfx::Texture** ppTexture);
		void   DestroyTexture(const grfx::Texture* pTexture);

		Result CreateTextureFont(const grfx::TextureFontCreateInfo* pCreateInfo, grfx::TextureFont** ppTextureFont);
		void   DestroyTextureFont(const grfx::TextureFont* pTextureFont);

		// See comment section for grfx::internal::CommandBufferCreateInfo for
		// details about 'resourceDescriptorCount' and 'samplerDescriptorCount'.
		//
		Result AllocateCommandBuffer(
			const grfx::CommandPool* pPool,
			grfx::CommandBuffer** ppCommandBuffer,
			uint32_t                 resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
			uint32_t                 samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
		void FreeCommandBuffer(const grfx::CommandBuffer* pCommandBuffer);

		Result AllocateDescriptorSet(grfx::DescriptorPool* pPool, const grfx::DescriptorSetLayout* pLayout, grfx::DescriptorSet** ppSet);
		void   FreeDescriptorSet(const grfx::DescriptorSet* pSet);

		uint32_t       GetGraphicsQueueCount() const;
		Result         GetGraphicsQueue(uint32_t index, grfx::Queue** ppQueue) const;
		grfx::QueuePtr GetGraphicsQueue(uint32_t index = 0) const;

		uint32_t       GetComputeQueueCount() const;
		Result         GetComputeQueue(uint32_t index, grfx::Queue** ppQueue) const;
		grfx::QueuePtr GetComputeQueue(uint32_t index = 0) const;

		uint32_t       GetTransferQueueCount() const;
		Result         GetTransferQueue(uint32_t index, grfx::Queue** ppQueue) const;
		grfx::QueuePtr GetTransferQueue(uint32_t index = 0) const;

		grfx::QueuePtr GetAnyAvailableQueue() const;

		const grfx::ShadingRateCapabilities& GetShadingRateCapabilities() const { return mShadingRateCapabilities; }

		virtual Result WaitIdle() = 0;
		virtual bool   MultiViewSupported() const = 0;
		virtual bool   PipelineStatsAvailable() const = 0;
		virtual bool   DynamicRenderingSupported() const = 0;
		virtual bool   IndependentBlendingSupported() const = 0;
		virtual bool   FragmentStoresAndAtomicsSupported() const = 0;
		virtual bool   PartialDescriptorBindingsSupported() const = 0;
		virtual bool   IndexTypeUint8Supported() const = 0;

	protected:
		virtual Result Create(const grfx::DeviceCreateInfo* pCreateInfo) override;
		virtual void   Destroy() override;
		friend class grfx::Instance;

		virtual Result AllocateObject(grfx::Buffer** ppObject) = 0;
		virtual Result AllocateObject(grfx::CommandBuffer** ppObject) = 0;
		virtual Result AllocateObject(grfx::CommandPool** ppObject) = 0;
		virtual Result AllocateObject(grfx::ComputePipeline** ppObject) = 0;
		virtual Result AllocateObject(grfx::DepthStencilView** ppObject) = 0;
		virtual Result AllocateObject(grfx::DescriptorPool** ppObject) = 0;
		virtual Result AllocateObject(grfx::DescriptorSet** ppObject) = 0;
		virtual Result AllocateObject(grfx::DescriptorSetLayout** ppObject) = 0;
		virtual Result AllocateObject(grfx::Fence** ppObject) = 0;
		virtual Result AllocateObject(grfx::GraphicsPipeline** ppObject) = 0;
		virtual Result AllocateObject(grfx::Image** ppObject) = 0;
		virtual Result AllocateObject(grfx::PipelineInterface** ppObject) = 0;
		virtual Result AllocateObject(grfx::Queue** ppObject) = 0;
		virtual Result AllocateObject(grfx::Query** ppObject) = 0;
		virtual Result AllocateObject(grfx::RenderPass** ppObject) = 0;
		virtual Result AllocateObject(grfx::RenderTargetView** ppObject) = 0;
		virtual Result AllocateObject(grfx::SampledImageView** ppObject) = 0;
		virtual Result AllocateObject(grfx::Sampler** ppObject) = 0;
		virtual Result AllocateObject(grfx::SamplerYcbcrConversion** ppObject) = 0;
		virtual Result AllocateObject(grfx::Semaphore** ppObject) = 0;
		virtual Result AllocateObject(grfx::ShaderModule** ppObject) = 0;
		virtual Result AllocateObject(grfx::ShaderProgram** ppObject) = 0;
		virtual Result AllocateObject(grfx::ShadingRatePattern** ppObject) = 0;
		virtual Result AllocateObject(grfx::StorageImageView** ppObject) = 0;
		virtual Result AllocateObject(grfx::Swapchain** ppObject) = 0;

		virtual Result AllocateObject(grfx::DrawPass** ppObject);
		virtual Result AllocateObject(grfx::FullscreenQuad** ppObject);
		virtual Result AllocateObject(grfx::Mesh** ppObject);
		virtual Result AllocateObject(grfx::TextDraw** ppObject);
		virtual Result AllocateObject(grfx::Texture** ppObject);
		virtual Result AllocateObject(grfx::TextureFont** ppObject);

		template <
			typename ObjectT,
			typename CreateInfoT,
			typename ContainerT = std::vector<ObjPtr<ObjectT>>>
		Result CreateObject(const CreateInfoT* pCreateInfo, ContainerT& container, ObjectT** ppObject);

		template <
			typename ObjectT,
			typename ContainerT = std::vector<ObjPtr<ObjectT>>>
		void DestroyObject(ContainerT& container, const ObjectT* pObject);

		template <typename ObjectT>
		void DestroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

		Result CreateGraphicsQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);
		Result CreateComputeQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);
		Result CreateTransferQueue(const grfx::internal::QueueCreateInfo* pCreateInfo, grfx::Queue** ppQueue);

	protected:
		grfx::InstancePtr                            mInstance;
		std::vector<grfx::BufferPtr>                 mBuffers;
		std::vector<grfx::CommandBufferPtr>          mCommandBuffers;
		std::vector<grfx::CommandPoolPtr>            mCommandPools;
		std::vector<grfx::ComputePipelinePtr>        mComputePipelines;
		std::vector<grfx::DepthStencilViewPtr>       mDepthStencilViews;
		std::vector<grfx::DescriptorPoolPtr>         mDescriptorPools;
		std::vector<grfx::DescriptorSetPtr>          mDescriptorSets;
		std::vector<grfx::DescriptorSetLayoutPtr>    mDescriptorSetLayouts;
		std::vector<grfx::DrawPassPtr>               mDrawPasses;
		std::vector<grfx::FencePtr>                  mFences;
		std::vector<grfx::ShadingRatePatternPtr>     mShadingRatePatterns;
		std::vector<grfx::FullscreenQuadPtr>         mFullscreenQuads;
		std::vector<grfx::GraphicsPipelinePtr>       mGraphicsPipelines;
		std::vector<grfx::ImagePtr>                  mImages;
		std::vector<grfx::MeshPtr>                   mMeshes;
		std::vector<grfx::PipelineInterfacePtr>      mPipelineInterfaces;
		std::vector<grfx::QueryPtr>                  mQuerys;
		std::vector<grfx::RenderPassPtr>             mRenderPasses;
		std::vector<grfx::RenderTargetViewPtr>       mRenderTargetViews;
		std::vector<grfx::SampledImageViewPtr>       mSampledImageViews;
		std::vector<grfx::SamplerPtr>                mSamplers;
		std::vector<grfx::SamplerYcbcrConversionPtr> mSamplerYcbcrConversions;
		std::vector<grfx::SemaphorePtr>              mSemaphores;
		std::vector<grfx::ShaderModulePtr>           mShaderModules;
		std::vector<grfx::ShaderProgramPtr>          mShaderPrograms;
		std::vector<grfx::StorageImageViewPtr>       mStorageImageViews;
		std::vector<grfx::SwapchainPtr>              mSwapchains;
		std::vector<grfx::TextDrawPtr>               mTextDraws;
		std::vector<grfx::TexturePtr>                mTextures;
		std::vector<grfx::TextureFontPtr>            mTextureFonts;
		std::vector<grfx::QueuePtr>                  mGraphicsQueues;
		std::vector<grfx::QueuePtr>                  mComputeQueues;
		std::vector<grfx::QueuePtr>                  mTransferQueues;
		grfx::ShadingRateCapabilities                mShadingRateCapabilities;
	};

} // namespace grfx

#pragma endregion

#pragma region Instance

namespace grfx {

	struct InstanceCreateInfo
	{
		grfx::Api                api = grfx::API_UNDEFINED; // Direct3D or Vulkan.
		bool                     createDevices = false;               // Create grfx::Device objects with default options.
		bool                     enableDebug = false;               // Enable graphics API debug layers.
		bool                     enableSwapchain = true;                // Enable support for swapchain.
		bool                     useSoftwareRenderer = false;               // Use a software renderer instead of a hardware device (WARP on DirectX).
		std::string              applicationName;                           // [OPTIONAL] Application name.
		std::string              engineName;                                // [OPTIONAL] Engine name.
		bool                     forceDxDiscreteAllocations = false;        // [OPTIONAL] Forces D3D12 to make discrete allocations for resources.
		std::vector<std::string> vulkanLayers;                              // [OPTIONAL] Additional instance layers.
		std::vector<std::string> vulkanExtensions;                          // [OPTIONAL] Additional instance extensions.
#if defined(BUILD_XR)
		XrComponent* pXrComponent = nullptr;
#endif
	};

	class Instance
	{
	public:
		Instance() {}
		virtual ~Instance() {}

		bool IsDebugEnabled() const { return mCreateInfo.enableDebug; }
		bool IsSwapchainEnabled() const
		{
			return mCreateInfo.enableSwapchain;
		}
		bool ForceDxDiscreteAllocations() const
		{
			return mCreateInfo.forceDxDiscreteAllocations;
		}

		grfx::Api GetApi() const
		{
			return mCreateInfo.api;
		}

		uint32_t GetGpuCount() const
		{
			return CountU32(mGpus);
		}
		Result GetGpu(uint32_t index, grfx::Gpu** ppGpu) const;

		uint32_t GetDeviceCount() const
		{
			return CountU32(mDevices);
		}
		Result GetDevice(uint32_t index, grfx::Device** ppDevice) const;

		Result CreateDevice(const grfx::DeviceCreateInfo* pCreateInfo, grfx::Device** ppDevice);
		void   DestroyDevice(const grfx::Device* pDevice);

		Result CreateSurface(const grfx::SurfaceCreateInfo* pCreateInfo, grfx::Surface** ppSurface);
		void   DestroySurface(const grfx::Surface* pSurface);

#if defined(BUILD_XR)
		bool isXREnabled() const
		{
			return mCreateInfo.pXrComponent != nullptr;
		}
		virtual const XrBaseInStructure* XrGetGraphicsBinding() const = 0;
		virtual bool                     XrIsGraphicsBindingValid() const = 0;
		virtual void                     XrUpdateDeviceInGraphicsBinding() = 0;
#endif
	protected:
		Result CreateGpu(const grfx::internal::GpuCreateInfo* pCreateInfo, grfx::Gpu** ppGpu);
		void   DestroyGpu(const grfx::Gpu* pGpu);

		virtual Result AllocateObject(grfx::Device** ppDevice) = 0;
		virtual Result AllocateObject(grfx::Gpu** ppGpu) = 0;
		virtual Result AllocateObject(grfx::Surface** ppSurface) = 0;

		template <
			typename ObjectT,
			typename CreateInfoT,
			typename ContainerT = std::vector<ObjPtr<ObjectT>>>
		Result CreateObject(const CreateInfoT* pCreateInfo, ContainerT& container, ObjectT** ppObject);

		template <
			typename ObjectT,
			typename ContainerT = std::vector<ObjPtr<ObjectT>>>
		void DestroyObject(ContainerT& container, const ObjectT* pObject);

		template <typename ObjectT>
		void DestroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

	protected:
		virtual Result CreateApiObjects(const grfx::InstanceCreateInfo* pCreateInfo) = 0;
		virtual void   DestroyApiObjects() = 0;
		friend Result  CreateInstance(const grfx::InstanceCreateInfo* pCreateInfo, grfx::Instance** ppInstance);
		friend void    DestroyInstance(const grfx::Instance* pInstance);

	private:
		virtual Result Create(const grfx::InstanceCreateInfo* pCreateInfo);
		virtual void   Destroy();
		friend Result  CreateInstance(const grfx::InstanceCreateInfo* pCreateInfo, grfx::Instance** ppInstance);
		friend void    DestroyInstance(const grfx::Instance* pInstance);

	protected:
		grfx::InstanceCreateInfo      mCreateInfo = {};
		std::vector<grfx::GpuPtr>     mGpus;
		std::vector<grfx::DevicePtr>  mDevices;
		std::vector<grfx::SurfacePtr> mSurfaces;
	};

	Result CreateInstance(const grfx::InstanceCreateInfo* pCreateInfo, grfx::Instance** ppInstance);
	void   DestroyInstance(const grfx::Instance* pInstance);

} // namespace grfx

#pragma endregion
