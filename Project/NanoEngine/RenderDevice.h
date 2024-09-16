#pragma once

#include "RenderResources.h"

class EngineApplication;
class RenderSystem;
class DeviceQueue;

#pragma region RenderDevice

class RenderDevice final
{
public:
	RenderDevice(EngineApplication& engine, RenderSystem& render);

	bool Setup(ShadingRateMode supportShadingRateMode);
	void Shutdown();

	[[nodiscard]] VkDevice& GetVkDevice();
	[[nodiscard]] VmaAllocatorPtr GetVmaAllocator();

	[[nodiscard]] const VkPhysicalDeviceFeatures& GetDeviceFeatures() const;
	[[nodiscard]] const VkPhysicalDeviceLimits& GetDeviceLimits() const;
	[[nodiscard]] float GetDeviceTimestampPeriod() const;

	[[nodiscard]] const ShadingRateCapabilities& GetShadingRateCapabilities() const;

	[[nodiscard]] uint32_t GetMaxPushDescriptors() const;
	[[nodiscard]] bool HasDepthClipEnabled() const;
	[[nodiscard]] bool HasDescriptorIndexingFeatures() const;
	[[nodiscard]] bool HasMultiView() const;
	[[nodiscard]] bool PartialDescriptorBindingsSupported() const;

	[[nodiscard]] DeviceQueuePtr GetGraphicsDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetPresentDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetTransferDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetComputeDeviceQueue() const;
	[[nodiscard]] uint32_t GetGraphicsQueueFamilyIndex() const;
	[[nodiscard]] uint32_t GetComputeQueueFamilyIndex() const;
	[[nodiscard]] uint32_t GetTransferQueueFamilyIndex() const;
	[[nodiscard]] std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

	[[nodiscard]] QueuePtr GetGraphicsQueue() const;
	[[nodiscard]] QueuePtr GetAnyAvailableQueue() const;

	std::vector<char> LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName);
	Result CreateShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName, ShaderModule** ppShaderModule);

	Result CreateBuffer(const BufferCreateInfo& createInfo, Buffer** ppBuffer);
	void   DestroyBuffer(const Buffer* pBuffer);

	Result CreateCommandPool(const CommandPoolCreateInfo& createInfo, CommandPool** ppCommandPool);
	void   DestroyCommandPool(const CommandPool* pCommandPool);

	Result CreateComputePipeline(const ComputePipelineCreateInfo& createInfo, ComputePipeline** ppComputePipeline);
	void   DestroyComputePipeline(const ComputePipeline* pComputePipeline);

	Result CreateDepthStencilView(const DepthStencilViewCreateInfo& createInfo, DepthStencilView** ppDepthStencilView);
	void   DestroyDepthStencilView(const DepthStencilView* pDepthStencilView);

	Result CreateDescriptorPool(const DescriptorPoolCreateInfo& createInfo, DescriptorPool** ppDescriptorPool);
	void   DestroyDescriptorPool(const DescriptorPool* pDescriptorPool);

	Result CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** ppDescriptorSetLayout);
	void   DestroyDescriptorSetLayout(const DescriptorSetLayout* pDescriptorSetLayout);

	Result CreateDrawPass(const DrawPassCreateInfo& createInfo, DrawPass** ppDrawPass);
	Result CreateDrawPass(const DrawPassCreateInfo2& createInfo, DrawPass** ppDrawPass);
	Result CreateDrawPass(const DrawPassCreateInfo3& createInfo, DrawPass** ppDrawPass);
	void   DestroyDrawPass(const DrawPass* pDrawPass);

	Result CreateFence(const FenceCreateInfo& createInfo, Fence** ppFence);
	void   DestroyFence(const Fence* pFence);

	Result CreateShadingRatePattern(const ShadingRatePatternCreateInfo& createInfo, ShadingRatePattern** ppShadingRatePattern);
	void   DestroyShadingRatePattern(const ShadingRatePattern* pShadingRatePattern);

	Result CreateFullscreenQuad(const FullscreenQuadCreateInfo& createInfo, FullscreenQuad** ppFullscreenQuad);
	void   DestroyFullscreenQuad(const FullscreenQuad* pFullscreenQuad);

	Result CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipeline** ppGraphicsPipeline);
	Result CreateGraphicsPipeline(const GraphicsPipelineCreateInfo2& createInfo, GraphicsPipeline** ppGraphicsPipeline);
	void   DestroyGraphicsPipeline(const GraphicsPipeline* pGraphicsPipeline);

	Result CreateImage(const ImageCreateInfo& createInfo, Image** ppImage);
	void   DestroyImage(const Image* pImage);

	Result CreateMesh(const MeshCreateInfo& createInfo, Mesh** ppMesh);
	void   DestroyMesh(const Mesh* pMesh);

	Result CreatePipelineInterface(const PipelineInterfaceCreateInfo& createInfo, PipelineInterface** ppPipelineInterface);
	void   DestroyPipelineInterface(const PipelineInterface* pPipelineInterface);

	Result CreateQuery(const QueryCreateInfo& createInfo, Query** ppQuery);
	void   DestroyQuery(const Query* pQuery);

	Result CreateRenderPass(const RenderPassCreateInfo& createInfo, RenderPass** ppRenderPass);
	Result CreateRenderPass(const RenderPassCreateInfo2& createInfo, RenderPass** ppRenderPass);
	Result CreateRenderPass(const RenderPassCreateInfo3& createInfo, RenderPass** ppRenderPass);
	void   DestroyRenderPass(const RenderPass* pRenderPass);

	Result CreateRenderTargetView(const RenderTargetViewCreateInfo& createInfo, RenderTargetView** ppRenderTargetView);
	void   DestroyRenderTargetView(const RenderTargetView* pRenderTargetView);

	Result CreateSampledImageView(const SampledImageViewCreateInfo& createInfo, SampledImageView** ppSampledImageView);
	void   DestroySampledImageView(const SampledImageView* pSampledImageView);

	Result CreateSampler(const SamplerCreateInfo& createInfo, Sampler** ppSampler);
	void   DestroySampler(const Sampler* pSampler);

	Result CreateSamplerYcbcrConversion(const SamplerYcbcrConversionCreateInfo& createInfo, SamplerYcbcrConversion** ppConversion);
	void   DestroySamplerYcbcrConversion(const SamplerYcbcrConversion* pConversion);

	Result CreateSemaphore(const SemaphoreCreateInfo& createInfo, Semaphore** ppSemaphore);
	void   DestroySemaphore(const Semaphore* pSemaphore);

	Result CreateShaderModule(const ShaderModuleCreateInfo& createInfo, ShaderModule** ppShaderModule);
	void   DestroyShaderModule(const ShaderModule* pShaderModule);

	Result CreateStorageImageView(const StorageImageViewCreateInfo& createInfo, StorageImageView** ppStorageImageView);
	void   DestroyStorageImageView(const StorageImageView* pStorageImageView);

	Result CreateTextDraw(const TextDrawCreateInfo& createInfo, TextDraw** ppTextDraw);
	void   DestroyTextDraw(const TextDraw* pTextDraw);

	Result CreateTexture(const TextureCreateInfo& createInfo, Texture** ppTexture);
	void   DestroyTexture(const Texture* pTexture);

	Result CreateTextureFont(const TextureFontCreateInfo& createInfo, TextureFont** ppTextureFont);
	void   DestroyTextureFont(const TextureFont* pTextureFont);

	Result AllocateCommandBuffer(
		const CommandPool* pPool,
		CommandBuffer** ppCommandBuffer,
		uint32_t                 resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
		uint32_t                 samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
	void FreeCommandBuffer(const CommandBuffer* pCommandBuffer);

	Result AllocateDescriptorSet(DescriptorPool* pPool, const DescriptorSetLayout* pLayout, DescriptorSet** ppSet);
	void   FreeDescriptorSet(const DescriptorSet* pSet);

	//===================================================================
	// OLD
	//===================================================================

	VulkanFencePtr CreateFence(const FenceCreateInfo& createInfo);
	VulkanSemaphorePtr CreateSemaphore(const SemaphoreCreateInfo& createInfo);

	VulkanCommandPoolPtr CreateCommandPool(const CommandPoolCreateInfo& createInfo) { return {}; }

	bool AllocateCommandBuffer(VulkanCommandPoolPtr pool, VulkanCommandBufferPtr& commandBuffer, uint32_t resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT, uint32_t samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT) { return true; }
	void FreeCommandBuffer(VulkanCommandBufferPtr& commandBuffer) {}

private:
	Result allocateObject(Buffer** ppObject);
	Result allocateObject(CommandBuffer** ppObject);
	Result allocateObject(CommandPool** ppObject);
	Result allocateObject(ComputePipeline** ppObject);
	Result allocateObject(DepthStencilView** ppObject);
	Result allocateObject(DescriptorPool** ppObject);
	Result allocateObject(DescriptorSet** ppObject);
	Result allocateObject(DescriptorSetLayout** ppObject);
	Result allocateObject(Fence** ppObject);
	Result allocateObject(GraphicsPipeline** ppObject);
	Result allocateObject(Image** ppObject);
	Result allocateObject(PipelineInterface** ppObject);
	Result allocateObject(Queue** ppObject);
	Result allocateObject(Query** ppObject);
	Result allocateObject(RenderPass** ppObject);
	Result allocateObject(RenderTargetView** ppObject);
	Result allocateObject(SampledImageView** ppObject);
	Result allocateObject(Sampler** ppObject);
	Result allocateObject(SamplerYcbcrConversion** ppObject);
	Result allocateObject(Semaphore** ppObject);
	Result allocateObject(ShaderModule** ppObject);
	Result allocateObject(ShaderProgram** ppObject);
	Result allocateObject(ShadingRatePattern** ppObject);
	Result allocateObject(StorageImageView** ppObject);
	Result allocateObject(DrawPass** ppObject);
	Result allocateObject(FullscreenQuad** ppObject);
	Result allocateObject(Mesh** ppObject);
	Result allocateObject(TextDraw** ppObject);
	Result allocateObject(Texture** ppObject);
	Result allocateObject(TextureFont** ppObject);

	Result createGraphicsQueue(const internal::QueueCreateInfo& pCreateInfo, Queue** ppQueue);
	Result createComputeQueue(const internal::QueueCreateInfo& pCreateInfo, Queue** ppQueue);
	Result createTransferQueue(const internal::QueueCreateInfo& pCreateInfo, Queue** ppQueue);

	template <typename ObjectT, typename CreateInfoT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	Result createObject(const CreateInfoT& createInfo, ContainerT& container, ObjectT** ppObject);

	template <typename ObjectT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	void destroyObject(ContainerT& container, const ObjectT* pObject);

	template <typename ObjectT>
	void destroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

	std::optional<std::filesystem::path> getShaderPathSuffix(const std::filesystem::path& baseName);

	EngineApplication& m_engine;
	RenderSystem& m_render;

	std::vector<BufferPtr>                 mBuffers;
	std::vector<CommandBufferPtr>          mCommandBuffers;
	std::vector<CommandPoolPtr>            mCommandPools;
	std::vector<ComputePipelinePtr>        mComputePipelines;
	std::vector<DepthStencilViewPtr>       mDepthStencilViews;
	std::vector<DescriptorPoolPtr>         mDescriptorPools;
	std::vector<DescriptorSetPtr>          mDescriptorSets;
	std::vector<DescriptorSetLayoutPtr>    mDescriptorSetLayouts;
	std::vector<DrawPassPtr>               mDrawPasses;
	std::vector<FencePtr>                  mFences;
	std::vector<ShadingRatePatternPtr>     mShadingRatePatterns;
	std::vector<FullscreenQuadPtr>         mFullscreenQuads;
	std::vector<GraphicsPipelinePtr>       mGraphicsPipelines;
	std::vector<ImagePtr>                  mImages;
	std::vector<MeshPtr>                   mMeshes;
	std::vector<PipelineInterfacePtr>      mPipelineInterfaces;
	std::vector<QueryPtr>                  mQuerys;
	std::vector<RenderPassPtr>             mRenderPasses;
	std::vector<RenderTargetViewPtr>       mRenderTargetViews;
	std::vector<SampledImageViewPtr>       mSampledImageViews;
	std::vector<SamplerPtr>                mSamplers;
	std::vector<SamplerYcbcrConversionPtr> mSamplerYcbcrConversions;
	std::vector<SemaphorePtr>              mSemaphores;
	std::vector<ShaderModulePtr>           mShaderModules;
	std::vector<ShaderProgramPtr>          mShaderPrograms;
	std::vector<StorageImageViewPtr>       mStorageImageViews;
	std::vector<TextDrawPtr>               mTextDraws;
	std::vector<TexturePtr>                mTextures;
	std::vector<TextureFontPtr>            mTextureFonts;
	std::vector<QueuePtr>                  mGraphicsQueues;
	std::vector<QueuePtr>                  mComputeQueues;
	std::vector<QueuePtr>                  mTransferQueues;
	ShadingRateCapabilities                mShadingRateCapabilities{};

	QueuePtr                               m_graphicsQueue;
};

#pragma endregion