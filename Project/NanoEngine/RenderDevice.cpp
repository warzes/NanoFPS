#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"

namespace vkr {

#pragma region RenderDevice

RenderDevice::RenderDevice(EngineApplication& engine, RenderSystem& render)
	: m_engine(engine)
	, m_render(render)
{
}

bool RenderDevice::Setup(ShadingRateMode supportShadingRateMode)
{
	CHECKED_CALL(createGraphicsQueue(&m_graphicsQueue), "TODO: error text"); // TODO: error text
	CHECKED_CALL(createComputeQueue(&m_computeQueue), "TODO: error text"); // TODO: error text
	if (supportShadingRateMode != SHADING_RATE_NONE)
	{
		// TODO:
	}
	return true;
}

void RenderDevice::Shutdown()
{
	// Destroy queues first to clear any pending work
	destroyAllObjects(mGraphicsQueues);
	destroyAllObjects(mComputeQueues);
	destroyAllObjects(mTransferQueues);

	// Destroy helper objects first
	destroyAllObjects(mDrawPasses);
	destroyAllObjects(mFullscreenQuads);
	destroyAllObjects(mTextDraws);
	destroyAllObjects(mTextures);
	destroyAllObjects(mTextureFonts);

	// Destroy render passes before images and views
	destroyAllObjects(mRenderPasses);

	destroyAllObjects(mBuffers);
	destroyAllObjects(mCommandBuffers);
	destroyAllObjects(mCommandPools);
	destroyAllObjects(mComputePipelines);
	destroyAllObjects(mDepthStencilViews);
	destroyAllObjects(mDescriptorSets); // Descriptor sets need to be destroyed before pools
	destroyAllObjects(mDescriptorPools);
	destroyAllObjects(mDescriptorSetLayouts);
	destroyAllObjects(mFences);
	destroyAllObjects(mImages);
	destroyAllObjects(mGraphicsPipelines);
	destroyAllObjects(mPipelineInterfaces);
	destroyAllObjects(mQuerys);
	destroyAllObjects(mRenderTargetViews);
	destroyAllObjects(mSampledImageViews);
	destroyAllObjects(mSamplers);
	destroyAllObjects(mSemaphores);
	destroyAllObjects(mStorageImageViews);
	destroyAllObjects(mShaderModules);
	// Destroy Ycbcr Conversions after images and views
	destroyAllObjects(mSamplerYcbcrConversions);
}

VkDevice& RenderDevice::GetVkDevice()
{
	return m_render.GetVkDevice();
}

VmaAllocatorPtr RenderDevice::GetVmaAllocator()
{
	return m_render.GetVmaAllocator();
}

const VkPhysicalDeviceFeatures& RenderDevice::GetDeviceFeatures() const
{
	return m_render.GetDeviceFeatures();
}

const VkPhysicalDeviceLimits& RenderDevice::GetDeviceLimits() const
{
	return m_render.GetDeviceLimits();
}

float RenderDevice::GetDeviceTimestampPeriod() const
{
	return m_render.GetDeviceTimestampPeriod();
}

const ShadingRateCapabilities& RenderDevice::GetShadingRateCapabilities() const
{
	return mShadingRateCapabilities;
}

uint32_t RenderDevice::GetMaxPushDescriptors() const
{
	return m_render.GetMaxPushDescriptors();
}

bool RenderDevice::HasDepthClipEnabled() const
{
	return true; // TODO: требуется VK_EXT_DEPTH_CLIP_ENABLE_EXTENSION_NAME, я его сейчас включаю обязательным
}

bool RenderDevice::HasDescriptorIndexingFeatures() const
{
	return true; // TODO: VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME или vk1.3?
}

bool RenderDevice::HasMultiView() const
{
	return true; // VK_KHR_MULTIVIEW_EXTENSION_NAME
}

bool RenderDevice::PartialDescriptorBindingsSupported() const
{
	return m_render.PartialDescriptorBindingsSupported();
}
bool RenderDevice::PipelineStatsAvailable() const
{
	return m_render.GetDeviceFeatures().pipelineStatisticsQuery;
}

bool RenderDevice::IndependentBlendingSupported() const
{
	return m_render.GetDeviceFeatures().independentBlend == VK_TRUE;
}

bool RenderDevice::FragmentStoresAndAtomicsSupported() const
{
	return m_render.GetDeviceFeatures().fragmentStoresAndAtomics == VK_TRUE;
}

DeviceQueuePtr RenderDevice::GetGraphicsDeviceQueue() const
{
	return m_render.GetVkGraphicsQueue();
}
DeviceQueuePtr RenderDevice::GetPresentDeviceQueue() const
{
	return m_render.GetVkPresentQueue();
}
DeviceQueuePtr RenderDevice::GetTransferDeviceQueue() const
{
	return m_render.GetVkTransferQueue();
}
DeviceQueuePtr RenderDevice::GetComputeDeviceQueue() const
{
	return m_render.GetVkComputeQueue();
}

uint32_t RenderDevice::GetGraphicsQueueFamilyIndex() const
{
	return GetGraphicsDeviceQueue()->QueueFamily;
}

uint32_t RenderDevice::GetComputeQueueFamilyIndex() const
{
	return GetComputeDeviceQueue()->QueueFamily;
}

uint32_t RenderDevice::GetTransferQueueFamilyIndex() const
{
	return GetTransferDeviceQueue()->QueueFamily;
}

std::array<uint32_t, 3> RenderDevice::GetAllQueueFamilyIndices() const
{
	return { GetGraphicsQueueFamilyIndex(), GetComputeQueueFamilyIndex(), GetTransferQueueFamilyIndex() };
}

QueuePtr RenderDevice::GetGraphicsQueue() const
{
	return m_graphicsQueue;
}

QueuePtr RenderDevice::GetComputeQueue() const
{
	return m_computeQueue;
}

QueuePtr RenderDevice::GetAnyAvailableQueue() const
{
	return GetGraphicsQueue(); // TODO: по идее сюда можно вставить любую очередь, не только графическую, а более свободную
}

std::vector<char> RenderDevice::LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName)
{
	ASSERT_MSG(baseDir.is_relative(), "baseDir must be relative. Do not call GetAssetPath() on the directory.");
	ASSERT_MSG(baseName.is_relative(), "baseName must be relative. Do not call GetAssetPath() on the directory.");
	auto suffix = getShaderPathSuffix(baseName);
	if (!suffix.has_value())
	{
		ASSERT_MSG(false, "unsupported API");
		return {};
	}

	const auto filePath = baseDir / suffix.value();
	auto       bytecode = LoadFile(filePath);
	if (!bytecode.has_value()) {
		ASSERT_MSG(false, "could not load file: " + filePath.string());
		return {};
	}

	Print("Loaded shader from " + filePath.string());
	return bytecode.value();
}

Result RenderDevice::CreateShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName, ShaderModule** ppShaderModule)
{
	std::vector<char> bytecode = LoadShader(baseDir, baseName);
	if (bytecode.empty())
	{
		Fatal("Shader bytecode load failed");
		return ERROR_GRFX_INVALID_SHADER_BYTE_CODE;
	}

	ShaderModuleCreateInfo shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
	Result ppxres = CreateShaderModule(shaderCreateInfo, ppShaderModule);
	if (Failed(ppxres))
	{
		Error("Shader not create.");
		return ppxres;
	}

	return SUCCESS;
}

Result RenderDevice::CreateBuffer(const BufferCreateInfo& pCreateInfo, Buffer** ppBuffer)
{
	ASSERT_NULL_ARG(ppBuffer);
	return createObject(pCreateInfo, mBuffers, ppBuffer);
}

void RenderDevice::DestroyBuffer(const Buffer* pBuffer)
{
	ASSERT_NULL_ARG(pBuffer);
	destroyObject(mBuffers, pBuffer);
}

Result RenderDevice::CreateCommandPool(const CommandPoolCreateInfo& pCreateInfo, CommandPool** ppCommandPool)
{
	ASSERT_NULL_ARG(ppCommandPool);
	return createObject(pCreateInfo, mCommandPools, ppCommandPool);
}

void RenderDevice::DestroyCommandPool(const CommandPool* pCommandPool)
{
	ASSERT_NULL_ARG(pCommandPool);
	destroyObject(mCommandPools, pCommandPool);
}

Result RenderDevice::CreateComputePipeline(const ComputePipelineCreateInfo& pCreateInfo, ComputePipeline** ppComputePipeline)
{
	ASSERT_NULL_ARG(ppComputePipeline);
	return createObject(pCreateInfo, mComputePipelines, ppComputePipeline);
}

void RenderDevice::DestroyComputePipeline(const ComputePipeline* pComputePipeline)
{
	ASSERT_NULL_ARG(pComputePipeline);
	destroyObject(mComputePipelines, pComputePipeline);
}

Result RenderDevice::CreateDepthStencilView(const DepthStencilViewCreateInfo& pCreateInfo, DepthStencilView** ppDepthStencilView)
{
	ASSERT_NULL_ARG(ppDepthStencilView);
	return createObject(pCreateInfo, mDepthStencilViews, ppDepthStencilView);
}

void RenderDevice::DestroyDepthStencilView(const DepthStencilView* pDepthStencilView)
{
	ASSERT_NULL_ARG(pDepthStencilView);
	destroyObject(mDepthStencilViews, pDepthStencilView);
}

Result RenderDevice::CreateDescriptorPool(const DescriptorPoolCreateInfo& pCreateInfo, DescriptorPool** ppDescriptorPool)
{
	ASSERT_NULL_ARG(ppDescriptorPool);
	return createObject(pCreateInfo, mDescriptorPools, ppDescriptorPool);
}

void RenderDevice::DestroyDescriptorPool(const DescriptorPool* pDescriptorPool)
{
	ASSERT_NULL_ARG(pDescriptorPool);
	destroyObject(mDescriptorPools, pDescriptorPool);
}

Result RenderDevice::CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& pCreateInfo, DescriptorSetLayout** ppDescriptorSetLayout)
{
	ASSERT_NULL_ARG(ppDescriptorSetLayout);
	return createObject(pCreateInfo, mDescriptorSetLayouts, ppDescriptorSetLayout);
}

void RenderDevice::DestroyDescriptorSetLayout(const DescriptorSetLayout* pDescriptorSetLayout)
{
	ASSERT_NULL_ARG(pDescriptorSetLayout);
	destroyObject(mDescriptorSetLayouts, pDescriptorSetLayout);
}

Result RenderDevice::CreateDrawPass(const DrawPassCreateInfo& pCreateInfo, DrawPass** ppDrawPass)
{
	ASSERT_NULL_ARG(ppDrawPass);

	internal::DrawPassCreateInfo createInfo = internal::DrawPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mDrawPasses, ppDrawPass);
}

Result RenderDevice::CreateDrawPass(const DrawPassCreateInfo2& pCreateInfo, DrawPass** ppDrawPass)
{
	ASSERT_NULL_ARG(ppDrawPass);

	internal::DrawPassCreateInfo createInfo = internal::DrawPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mDrawPasses, ppDrawPass);
}

Result RenderDevice::CreateDrawPass(const DrawPassCreateInfo3& pCreateInfo, DrawPass** ppDrawPass)
{
	ASSERT_NULL_ARG(ppDrawPass);

	internal::DrawPassCreateInfo createInfo = internal::DrawPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mDrawPasses, ppDrawPass);
}

void RenderDevice::DestroyDrawPass(const DrawPass* pDrawPass)
{
	ASSERT_NULL_ARG(pDrawPass);
	destroyObject(mDrawPasses, pDrawPass);
}

Result RenderDevice::CreateFence(const FenceCreateInfo& createInfo, Fence** ppFence)
{
	ASSERT_NULL_ARG(ppFence);
	return createObject(createInfo, mFences, ppFence);
}

void RenderDevice::DestroyFence(const Fence* fence)
{
	ASSERT_NULL_ARG(fence);
	destroyObject(mFences, fence);
}

Result RenderDevice::CreateShadingRatePattern(const ShadingRatePatternCreateInfo& pCreateInfo, ShadingRatePattern** ppShadingRatePattern)
{
	ASSERT_NULL_ARG(ppShadingRatePattern);
	return createObject(pCreateInfo, mShadingRatePatterns, ppShadingRatePattern);
}

void RenderDevice::DestroyShadingRatePattern(const ShadingRatePattern* pShadingRatePattern)
{
	ASSERT_NULL_ARG(pShadingRatePattern);
	destroyObject(mShadingRatePatterns, pShadingRatePattern);
}

Result RenderDevice::CreateFullscreenQuad(const FullscreenQuadCreateInfo& pCreateInfo, FullscreenQuad** ppFullscreenQuad)
{
	ASSERT_NULL_ARG(ppFullscreenQuad);
	return createObject(pCreateInfo, mFullscreenQuads, ppFullscreenQuad);
}

void RenderDevice::DestroyFullscreenQuad(const FullscreenQuad* pFullscreenQuad)
{
	ASSERT_NULL_ARG(pFullscreenQuad);
	destroyObject(mFullscreenQuads, pFullscreenQuad);
}

Result RenderDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& pCreateInfo, GraphicsPipeline** ppGraphicsPipeline)
{
	ASSERT_NULL_ARG(ppGraphicsPipeline);
	return createObject(pCreateInfo, mGraphicsPipelines, ppGraphicsPipeline);
}

Result RenderDevice::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo2& pCreateInfo, GraphicsPipeline** ppGraphicsPipeline)
{
	ASSERT_NULL_ARG(ppGraphicsPipeline);

	GraphicsPipelineCreateInfo createInfo = {};
	internal::FillOutGraphicsPipelineCreateInfo(pCreateInfo, &createInfo);

	return createObject(createInfo, mGraphicsPipelines, ppGraphicsPipeline);
}

void RenderDevice::DestroyGraphicsPipeline(const GraphicsPipeline* pGraphicsPipeline)
{
	ASSERT_NULL_ARG(pGraphicsPipeline);
	destroyObject(mGraphicsPipelines, pGraphicsPipeline);
}

Result RenderDevice::CreateImage(const ImageCreateInfo& pCreateInfo, Image** ppImage)
{
	ASSERT_NULL_ARG(ppImage);
	return createObject(pCreateInfo, mImages, ppImage);
}

void RenderDevice::DestroyImage(const Image* pImage)
{
	ASSERT_NULL_ARG(pImage);
	destroyObject(mImages, pImage);
}

Result RenderDevice::CreateMesh(const MeshCreateInfo& pCreateInfo, Mesh** ppMesh)
{
	ASSERT_NULL_ARG(ppMesh);
	return createObject(pCreateInfo, mMeshes, ppMesh);
}

void RenderDevice::DestroyMesh(const Mesh* pMesh)
{
	ASSERT_NULL_ARG(pMesh);
	destroyObject(mMeshes, pMesh);
}

Result RenderDevice::CreatePipelineInterface(const PipelineInterfaceCreateInfo& pCreateInfo, PipelineInterface** ppPipelineInterface)
{
	ASSERT_NULL_ARG(ppPipelineInterface);
	return createObject(pCreateInfo, mPipelineInterfaces, ppPipelineInterface);
}

void RenderDevice::DestroyPipelineInterface(const PipelineInterface* pPipelineInterface)
{
	ASSERT_NULL_ARG(pPipelineInterface);
	destroyObject(mPipelineInterfaces, pPipelineInterface);
}

Result RenderDevice::CreateQuery(const QueryCreateInfo& pCreateInfo, Query** ppQuery)
{
	ASSERT_NULL_ARG(ppQuery);
	return createObject(pCreateInfo, mQuerys, ppQuery);
}

void RenderDevice::DestroyQuery(const Query* pQuery)
{
	ASSERT_NULL_ARG(pQuery);
	destroyObject(mQuerys, pQuery);
}

Result RenderDevice::CreateRenderPass(const RenderPassCreateInfo& pCreateInfo, RenderPass** ppRenderPass)
{
	ASSERT_NULL_ARG(ppRenderPass);

	internal::RenderPassCreateInfo createInfo = internal::RenderPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mRenderPasses, ppRenderPass);
}

Result RenderDevice::CreateRenderPass(const RenderPassCreateInfo2& pCreateInfo, RenderPass** ppRenderPass)
{
	ASSERT_NULL_ARG(ppRenderPass);

	internal::RenderPassCreateInfo createInfo = internal::RenderPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mRenderPasses, ppRenderPass);
}

Result RenderDevice::CreateRenderPass(const RenderPassCreateInfo3& pCreateInfo, RenderPass** ppRenderPass)
{
	ASSERT_NULL_ARG(ppRenderPass);

	internal::RenderPassCreateInfo createInfo = internal::RenderPassCreateInfo(pCreateInfo);

	return createObject(createInfo, mRenderPasses, ppRenderPass);
}

void RenderDevice::DestroyRenderPass(const RenderPass* pRenderPass)
{
	ASSERT_NULL_ARG(pRenderPass);
	destroyObject(mRenderPasses, pRenderPass);
}

Result RenderDevice::CreateRenderTargetView(const RenderTargetViewCreateInfo& pCreateInfo, RenderTargetView** ppRenderTargetView)
{
	ASSERT_NULL_ARG(ppRenderTargetView);
	return createObject(pCreateInfo, mRenderTargetViews, ppRenderTargetView);
}

void RenderDevice::DestroyRenderTargetView(const RenderTargetView* pRenderTargetView)
{
	ASSERT_NULL_ARG(pRenderTargetView);
	destroyObject(mRenderTargetViews, pRenderTargetView);
}

Result RenderDevice::CreateSampledImageView(const SampledImageViewCreateInfo& pCreateInfo, SampledImageView** ppSampledImageView)
{
	ASSERT_NULL_ARG(ppSampledImageView);
	return createObject(pCreateInfo, mSampledImageViews, ppSampledImageView);
}

void RenderDevice::DestroySampledImageView(const SampledImageView* pSampledImageView)
{
	ASSERT_NULL_ARG(pSampledImageView);
	destroyObject(mSampledImageViews, pSampledImageView);
}

Result RenderDevice::CreateSampler(const SamplerCreateInfo& pCreateInfo, Sampler** ppSampler)
{
	ASSERT_NULL_ARG(ppSampler);
	return createObject(pCreateInfo, mSamplers, ppSampler);
}

void RenderDevice::DestroySampler(const Sampler* pSampler)
{
	ASSERT_NULL_ARG(pSampler);
	destroyObject(mSamplers, pSampler);
}

Result RenderDevice::CreateSamplerYcbcrConversion(const SamplerYcbcrConversionCreateInfo& pCreateInfo, SamplerYcbcrConversion** ppConversion)
{
	ASSERT_NULL_ARG(ppConversion);
	return createObject(pCreateInfo, mSamplerYcbcrConversions, ppConversion);
}

void RenderDevice::DestroySamplerYcbcrConversion(const SamplerYcbcrConversion* pConversion)
{
	ASSERT_NULL_ARG(pConversion);
	destroyObject(mSamplerYcbcrConversions, pConversion);
}

Result RenderDevice::CreateSemaphore(const SemaphoreCreateInfo& pCreateInfo, Semaphore** ppSemaphore)
{
	ASSERT_NULL_ARG(ppSemaphore);
	return createObject(pCreateInfo, mSemaphores, ppSemaphore);
}

void RenderDevice::DestroySemaphore(const Semaphore* pSemaphore)
{
	ASSERT_NULL_ARG(pSemaphore);
	destroyObject(mSemaphores, pSemaphore);
}

Result RenderDevice::CreateShaderModule(const ShaderModuleCreateInfo& pCreateInfo, ShaderModule** ppShaderModule)
{
	ASSERT_NULL_ARG(ppShaderModule);
	return createObject(pCreateInfo, mShaderModules, ppShaderModule);
}

void RenderDevice::DestroyShaderModule(const ShaderModule* pShaderModule)
{
	ASSERT_NULL_ARG(pShaderModule);
	destroyObject(mShaderModules, pShaderModule);
}

Result RenderDevice::CreateStorageImageView(const StorageImageViewCreateInfo& pCreateInfo, StorageImageView** ppStorageImageView)
{
	ASSERT_NULL_ARG(ppStorageImageView);
	return createObject(pCreateInfo, mStorageImageViews, ppStorageImageView);
}

void RenderDevice::DestroyStorageImageView(const StorageImageView* pStorageImageView)
{
	ASSERT_NULL_ARG(pStorageImageView);
	destroyObject(mStorageImageViews, pStorageImageView);
}

Result RenderDevice::CreateTextDraw(const TextDrawCreateInfo& pCreateInfo, TextDraw** ppTextDraw)
{
	ASSERT_NULL_ARG(ppTextDraw);
	return createObject(pCreateInfo, mTextDraws, ppTextDraw);
}

void RenderDevice::DestroyTextDraw(const TextDraw* pTextDraw)
{
	ASSERT_NULL_ARG(pTextDraw);
	destroyObject(mTextDraws, pTextDraw);
}

Result RenderDevice::CreateTexture(const TextureCreateInfo& pCreateInfo, Texture** ppTexture)
{
	ASSERT_NULL_ARG(ppTexture);
	return createObject(pCreateInfo, mTextures, ppTexture);
}

void RenderDevice::DestroyTexture(const Texture* pTexture)
{
	ASSERT_NULL_ARG(pTexture);
	destroyObject(mTextures, pTexture);
}

Result RenderDevice::CreateTextureFont(const TextureFontCreateInfo& pCreateInfo, TextureFont** ppTextureFont)
{
	ASSERT_NULL_ARG(ppTextureFont);
	return createObject(pCreateInfo, mTextureFonts, ppTextureFont);
}

void RenderDevice::DestroyTextureFont(const TextureFont* pTextureFont)
{
	ASSERT_NULL_ARG(pTextureFont);
	destroyObject(mTextureFonts, pTextureFont);
}

Result RenderDevice::AllocateCommandBuffer(
	const CommandPool* pPool,
	CommandBuffer** ppCommandBuffer,
	uint32_t                 resourceDescriptorCount,
	uint32_t                 samplerDescriptorCount)
{
	ASSERT_NULL_ARG(ppCommandBuffer);

	internal::CommandBufferCreateInfo createInfo = {};
	createInfo.pPool = pPool;
	createInfo.resourceDescriptorCount = resourceDescriptorCount;
	createInfo.samplerDescriptorCount = samplerDescriptorCount;

	return createObject(createInfo, mCommandBuffers, ppCommandBuffer);
}

void RenderDevice::FreeCommandBuffer(const CommandBuffer* pCommandBuffer)
{
	ASSERT_NULL_ARG(pCommandBuffer);
	destroyObject(mCommandBuffers, pCommandBuffer);
}

Result RenderDevice::AllocateDescriptorSet(DescriptorPool* pPool, const DescriptorSetLayout* pLayout, DescriptorSet** ppSet)
{
	ASSERT_NULL_ARG(pPool);
	ASSERT_NULL_ARG(pLayout);
	ASSERT_NULL_ARG(ppSet);

	// Prevent allocation using layouts that are pushable
	if (pLayout->IsPushable()) {
		return ERROR_GRFX_OPERATION_NOT_PERMITTED;
	}

	internal::DescriptorSetCreateInfo createInfo = {};
	createInfo.pPool = pPool;
	createInfo.pLayout = pLayout;

	return createObject(createInfo, mDescriptorSets, ppSet);
}

void RenderDevice::FreeDescriptorSet(const DescriptorSet* pSet)
{
	ASSERT_NULL_ARG(pSet);
	destroyObject(mDescriptorSets, pSet);
}

Result RenderDevice::allocateObject(Buffer** ppObject)
{
	Buffer* pObject = new Buffer();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(CommandBuffer** ppObject)
{
	CommandBuffer* pObject = new CommandBuffer();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(CommandPool** ppObject)
{
	CommandPool* pObject = new CommandPool();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(ComputePipeline** ppObject)
{
	ComputePipeline* pObject = new ComputePipeline();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(DepthStencilView** ppObject)
{
	DepthStencilView* pObject = new DepthStencilView();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(DescriptorPool** ppObject)
{
	DescriptorPool* pObject = new DescriptorPool();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(DescriptorSet** ppObject)
{
	DescriptorSet* pObject = new DescriptorSet();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(DescriptorSetLayout** ppObject)
{
	DescriptorSetLayout* pObject = new DescriptorSetLayout();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Fence** ppObject)
{
	Fence* pObject = new Fence();
	if (IsNull(pObject)) return ERROR_ALLOCATION_FAILED;
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(GraphicsPipeline** ppObject)
{
	GraphicsPipeline* pObject = new GraphicsPipeline();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Image** ppObject)
{
	Image* pObject = new Image();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(PipelineInterface** ppObject)
{
	PipelineInterface* pObject = new PipelineInterface();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Queue** ppObject)
{
	Queue* pObject = new Queue();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Query** ppObject)
{
	Query* pObject = new Query();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(RenderPass** ppObject)
{
	RenderPass* pObject = new RenderPass();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(RenderTargetView** ppObject)
{
	RenderTargetView* pObject = new RenderTargetView();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(SampledImageView** ppObject)
{
	SampledImageView* pObject = new SampledImageView();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Sampler** ppObject)
{
	Sampler* pObject = new Sampler();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(SamplerYcbcrConversion** ppObject)
{
	SamplerYcbcrConversion* pObject = new SamplerYcbcrConversion();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Semaphore** ppObject)
{
	Semaphore* pObject = new Semaphore();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(ShaderModule** ppObject)
{
	ShaderModule* pObject = new ShaderModule();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(ShaderProgram** ppObject)
{
	return ERROR_ALLOCATION_FAILED;
}

Result RenderDevice::allocateObject(ShadingRatePattern** ppObject)
{
	ShadingRatePattern* pObject = new ShadingRatePattern();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(StorageImageView** ppObject)
{
	StorageImageView* pObject = new StorageImageView();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(DrawPass** ppObject)
{
	DrawPass* pObject = new DrawPass();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(FullscreenQuad** ppObject)
{
	FullscreenQuad* pObject = new FullscreenQuad();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Mesh** ppObject)
{
	Mesh* pObject = new Mesh();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(TextDraw** ppObject)
{
	TextDraw* pObject = new TextDraw();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(Texture** ppObject)
{
	Texture* pObject = new Texture();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::allocateObject(TextureFont** ppObject)
{
	TextureFont* pObject = new TextureFont();
	if (IsNull(pObject)) {
		return ERROR_ALLOCATION_FAILED;
	}
	*ppObject = pObject;
	return SUCCESS;
}

Result RenderDevice::createGraphicsQueue(Queue** ppQueue)
{
	auto createInfo = internal::QueueCreateInfo{ .commandType = COMMAND_TYPE_GRAPHICS };
	ASSERT_NULL_ARG(ppQueue);
	return createObject(createInfo, mGraphicsQueues, ppQueue);
}

Result RenderDevice::createComputeQueue(Queue** ppQueue)
{
	auto createInfo = internal::QueueCreateInfo{ .commandType = COMMAND_TYPE_COMPUTE };
	ASSERT_NULL_ARG(ppQueue);
	return createObject(createInfo, mComputeQueues, ppQueue);
}

Result RenderDevice::createTransferQueue(Queue** ppQueue)
{
	auto createInfo = internal::QueueCreateInfo{ .commandType = COMMAND_TYPE_TRANSFER };
	ASSERT_NULL_ARG(ppQueue);
	return createObject(createInfo, mTransferQueues, ppQueue);
}

template<typename ObjectT, typename CreateInfoT, typename ContainerT>
inline Result RenderDevice::createObject(const CreateInfoT& createInfo, ContainerT& container, ObjectT** ppObject)
{
	// Allocate object
	ObjectT* pObject = nullptr;
	Result   ppxres = allocateObject(&pObject);
	if (Failed(ppxres)) return ppxres;

	// Set parent
	pObject->setParent(this);
	// Create internal objects
	ppxres = pObject->create(createInfo);
	if (Failed(ppxres)) 
	{
		pObject->destroy();
		delete pObject;
		pObject = nullptr;
		return ppxres;
	}
	// Store
	container.push_back(ObjPtr<ObjectT>(pObject));
	// Assign
	*ppObject = pObject;
	// Success
	return SUCCESS;
}

template<typename ObjectT, typename ContainerT>
void RenderDevice::destroyObject(ContainerT& container, const ObjectT* pObject)
{
	// Make sure object is in container
	auto it = std::find_if(
		std::begin(container),
		std::end(container),
		[pObject](const ObjPtr<ObjectT>& elem) -> bool {
			bool res = (elem == pObject);
			return res; });
	if (it == std::end(container)) return;

	// Copy pointer
	ObjPtr<ObjectT> object = *it;
	// Remove object pointer from container
	RemoveElement(object, container);
	// Destroy internal objects
	object->destroy();
	// Delete allocation
	ObjectT* ptr = object.Get();
	delete ptr;
}

template<typename ObjectT>
void RenderDevice::destroyAllObjects(std::vector<ObjPtr<ObjectT>>& container)
{
	size_t n = container.size();
	for (size_t i = 0; i < n; ++i)
	{
		// Get object pointer
		ObjPtr<ObjectT> object = container[i];
		// Destroy internal objects
		object->destroy();
		// Delete allocation
		ObjectT* ptr = object.Get();
		delete ptr;
	}
	// Clear container
	container.clear();
}

std::optional<std::filesystem::path> RenderDevice::getShaderPathSuffix(const std::filesystem::path& baseName)
{
	return (std::filesystem::path("spv") / baseName).concat(".spv");
}

//===================================================================
// OLD
//===================================================================

VulkanFencePtr RenderDevice::CreateFence(const FenceCreateInfo& createInfo)
{
	VulkanFencePtr res = std::make_shared<VulkanFence>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

VulkanSemaphorePtr RenderDevice::CreateSemaphore(const SemaphoreCreateInfo& createInfo)
{
	VulkanSemaphorePtr res = std::make_shared<VulkanSemaphore>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

#pragma endregion

} // namespace vkr