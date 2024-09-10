#pragma once

#include "RenderCore.h"

#pragma region VulkanFence

struct FenceCreateInfo final
{
	bool signaled = false;
};

class Fence final : public DeviceObject<FenceCreateInfo>
{
	friend class RenderDevice;
public:
	Result Wait(uint64_t timeout = UINT64_MAX);
	Result Reset();

	Result WaitAndReset(uint64_t timeout = UINT64_MAX);

	VkFencePtr GetVkFence() const { return m_fence; }

private:
	Result createApiObjects(const FenceCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkFencePtr m_fence;
};

#pragma endregion

#pragma region VulkanSemaphore

struct SemaphoreCreateInfo final
{
	SemaphoreType semaphoreType = SEMAPHORE_TYPE_BINARY;
	uint64_t      initialValue = 0; // Timeline semaphore only
};

class Semaphore final : public DeviceObject<SemaphoreCreateInfo>
{
	friend class RenderDevice;
public:
	VkSemaphorePtr GetVkSemaphore() const { return m_semaphore; }
	SemaphoreType  GetSemaphoreType() const { return m_createInfo.semaphoreType; }
	bool           IsBinary() const { return m_createInfo.semaphoreType == SEMAPHORE_TYPE_BINARY; }
	bool           IsTimeline() const { return m_createInfo.semaphoreType == SEMAPHORE_TYPE_TIMELINE; }

	// Timeline semaphore wait
	Result Wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;

	// Timeline semaphore signal
	// WARNING: Signaling a value less than what's already been signaled can cause a block or a race condition.
	Result Signal(uint64_t value) const;

	// Returns current timeline semaphore value
	uint64_t GetCounterValue() const;

private:
	Result createApiObjects(const SemaphoreCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	Result timelineWait(uint64_t value, uint64_t timeout) const;
	Result timelineSignal(uint64_t value) const;
	uint64_t timelineCounterValue() const;

	// Why are we storing timeline semaphore signaled values?
	// Direct3D allows fence objects to signal a value if the value is equal to or greater than what's already been signaled.
	// Vulkan does not:
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreSignalInfo.html#VUID-VkSemaphoreSignalInfo-value-03258
	// This is unfortunate, because there are cases where an application may need to signal a value that is equal to what's been signaled.
	// Even though it's possible to get the current value, add 1 to it, and then signal it - this can create a different problem where a value is signaled too soon and a write-after-read hazard possibly gets introduced.
	mutable uint64_t m_timelineSignaledValue = 0;
	VkSemaphorePtr m_semaphore;
};

#pragma endregion

#pragma region Query

#define GRFX_PIPELINE_STATISTIC_NUM_ENTRIES 11

union PipelineStatistics
{
	struct
	{
		uint64_t IAVertices;    // Input Assembly Vertices
		uint64_t IAPrimitives;  // Input Assembly Primitives
		uint64_t VSInvocations; // Vertex Shader Invocations
		uint64_t GSInvocations; // Geometry Shader Invocations
		uint64_t GSPrimitives;  // Geometry Shader Primitives
		uint64_t CInvocations;  // Clipping Invocations
		uint64_t CPrimitives;   // Clipping Primitives
		uint64_t PSInvocations; // Pixel Shader Invocations
		uint64_t HSInvocations; // Hull Shader Invocations
		uint64_t DSInvocations; // Domain Shader Invocations
		uint64_t CSInvocations; // Compute Shader Invocations
	};
	uint64_t Statistics[GRFX_PIPELINE_STATISTIC_NUM_ENTRIES] = { 0 };
};

struct QueryCreateInfo
{
	QueryType type = QUERY_TYPE_UNDEFINED;
	uint32_t        count = 0;
};

class Query final : public DeviceObject<QueryCreateInfo>
{
	friend class RenderDevice;
public:
	QueryType GetType() const { return m_createInfo.type; }
	uint32_t  GetCount() const { return m_createInfo.count; }

	VkQueryPoolPtr GetVkQueryPool() const { return mQueryPool; }
	uint32_t       GetQueryTypeSize() const { return GetQueryTypeSize(mType, mMultiplier); }
	VkBufferPtr    GetReadBackBuffer() const;

	void   Reset(uint32_t firstQuery, uint32_t queryCount);
	Result GetData(void* pDstData, uint64_t dstDataSize);

private:
	Result create(const QueryCreateInfo& pCreateInfo) final;
	Result createApiObjects(const QueryCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;
	uint32_t       GetQueryTypeSize(VkQueryType type, uint32_t multiplier) const;
	VkQueryType    GetQueryType() const { return mType; }

	VkQueryPoolPtr  mQueryPool;
	VkQueryType     mType = VK_QUERY_TYPE_MAX_ENUM;
	BufferPtr mBuffer;
	uint32_t        mMultiplier = 1;
};

#pragma endregion

#pragma region Buffer

struct BufferCreateInfo final
{
	uint64_t         size = 0;
	uint32_t         structuredElementStride = 0; // HLSL StructuredBuffer<> only
	BufferUsageFlags usageFlags = 0;
	MemoryUsage      memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ResourceState    initialState = RESOURCE_STATE_GENERAL;
	Ownership        ownership = OWNERSHIP_REFERENCE;
};

class Buffer final : public DeviceObject<BufferCreateInfo>
{
	friend class RenderDevice;
public:
	uint64_t                GetSize() const { return m_createInfo.size; }
	uint32_t                GetStructuredElementStride() const { return m_createInfo.structuredElementStride; }
	const BufferUsageFlags& GetUsageFlags() const { return m_createInfo.usageFlags; }
	VkBufferPtr             GetVkBuffer() const { return m_buffer; }

	Result MapMemory(uint64_t offset, void** ppMappedAddress);
	void   UnmapMemory();

	Result CopyFromSource(uint32_t dataSize, const void* pData);
	Result CopyToDest(uint32_t dataSize, void* pData);

private:
	Result create(const BufferCreateInfo& createInfo) final;
	Result createApiObjects(const BufferCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkBufferPtr m_buffer;
	VmaAllocationPtr m_allocation;
	VmaAllocationInfo m_allocationInfo = {};
};

struct IndexBufferView final
{
	IndexBufferView() {}
	IndexBufferView(const Buffer* pBuffer_, IndexType indexType_, uint64_t offset_ = 0, uint64_t size_ = WHOLE_SIZE) : pBuffer(pBuffer_), indexType(indexType_), offset(offset_), size(size_) {}

	const Buffer* pBuffer = nullptr;
	IndexType     indexType = INDEX_TYPE_UINT16;
	uint64_t      offset = 0;
	uint64_t      size = WHOLE_SIZE; // [D3D12 - REQUIRED] Size in bytes of view
};

struct VertexBufferView final
{
	VertexBufferView() {}
	VertexBufferView(const Buffer* pBuffer_, uint32_t stride_, uint64_t offset_ = 0, uint64_t size_ = 0) : pBuffer(pBuffer_), stride(stride_), offset(offset_), size(size_) {}

	const Buffer* pBuffer = nullptr;
	uint32_t      stride = 0; // [D3D12 - REQUIRED] Stride in bytes of vertex entry
	uint64_t      offset = 0;
	uint64_t      size = WHOLE_SIZE; // [D3D12 - REQUIRED] Size in bytes of view
};

#pragma endregion

#pragma region Image

struct ImageCreateInfo final
{
	ImageType              type = IMAGE_TYPE_2D;
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               depth = 0;
	Format                 format = FORMAT_UNDEFINED;
	SampleCount            sampleCount = SAMPLE_COUNT_1;
	uint32_t               mipLevelCount = 1;
	uint32_t               arrayLayerCount = 1;
	ImageUsageFlags        usageFlags = ImageUsageFlags::SampledImage();
	MemoryUsage            memoryUsage = MEMORY_USAGE_GPU_ONLY;   // D3D12 will fail on any other memory usage
	ResourceState          initialState = RESOURCE_STATE_GENERAL; // This may not be the best choice
	RenderTargetClearValue RTVClearValue = { 0, 0, 0, 0 };        // Optimized RTV clear value
	DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };        // Optimized DSV clear value
	void*                  pApiObject = nullptr;                  // [OPTIONAL] For external images such as swapchain images
	Ownership              ownership = OWNERSHIP_REFERENCE;
	bool                   concurrentMultiQueueUsage = false;
	ImageCreateFlags       createFlags = {};

	// Returns a create info for sampled image
	static ImageCreateInfo SampledImage2D(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1,
		MemoryUsage memoryUsage = MEMORY_USAGE_GPU_ONLY);

	// Returns a create info for sampled image and depth stencil target
	static ImageCreateInfo DepthStencilTarget(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1);

	// Returns a create info for sampled image and render target
	static ImageCreateInfo RenderTarget2D(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1);
};

class Image final : public DeviceObject<ImageCreateInfo>
{
	friend class RenderDevice;
public:
	ImageType                     GetType() const { return m_createInfo.type; }
	uint32_t                      GetWidth() const { return m_createInfo.width; }
	uint32_t                      GetHeight() const { return m_createInfo.height; }
	uint32_t                      GetDepth() const { return m_createInfo.depth; }
	Format                        GetFormat() const { return m_createInfo.format; }
	SampleCount                   GetSampleCount() const { return m_createInfo.sampleCount; }
	uint32_t                      GetMipLevelCount() const { return m_createInfo.mipLevelCount; }
	uint32_t                      GetArrayLayerCount() const { return m_createInfo.arrayLayerCount; }
	const ImageUsageFlags&        GetUsageFlags() const { return m_createInfo.usageFlags; }
	MemoryUsage                   GetMemoryUsage() const { return m_createInfo.memoryUsage; }
	ResourceState                 GetInitialState() const { return m_createInfo.initialState; }
	const RenderTargetClearValue& GetRTVClearValue() const { return m_createInfo.RTVClearValue; }
	const DepthStencilClearValue& GetDSVClearValue() const { return m_createInfo.DSVClearValue; }
	bool                          GetConcurrentMultiQueueUsageEnabled() const { return m_createInfo.concurrentMultiQueueUsage; }
	ImageCreateFlags              GetCreateFlags() const { return m_createInfo.createFlags; }

	VkImagePtr                    GetVkImage() const { return mImage; }
	VkFormat                      GetVkFormat() const { return mVkFormat; }
	VkImageAspectFlags            GetVkImageAspectFlags() const { return mImageAspect; }

	// Convenience functions
	ImageViewType GuessImageViewType(bool isCube = false) const;

	Result MapMemory(uint64_t offset, void** ppMappedAddress);
	void   UnmapMemory();

private:
	Result create(const ImageCreateInfo& pCreateInfo) final;
	Result createApiObjects(const ImageCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() override;

	VkImagePtr         mImage;
	VmaAllocationPtr   mAllocation;
	VmaAllocationInfo  mAllocationInfo = {};
	VkFormat           mVkFormat = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags mImageAspect = InvalidValue<VkImageAspectFlags>();
};

namespace internal
{
	class ImageResourceView final
	{
	public:
		ImageResourceView(VkImageViewPtr vkImageView, VkImageLayout layout)
			: mImageView(vkImageView), mImageLayout(layout) {}

		VkImageViewPtr GetVkImageView() const { return mImageView; }
		VkImageLayout  GetVkImageLayout() const { return mImageLayout; }

	private:
		VkImageViewPtr mImageView;
		VkImageLayout  mImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};
} // namespace internal

// This class exists to genericize descriptor updates for Vulkan's 'image' based resources.
class ImageView
{
public:
	ImageView() {}
	virtual ~ImageView() {}

	const internal::ImageResourceView* GetResourceView() const { return m_resourceView.get(); }

protected:
	void setResourceView(std::unique_ptr<internal::ImageResourceView>&& view)
	{
		m_resourceView = std::move(view);
	}

private:
	std::unique_ptr<internal::ImageResourceView> m_resourceView;
};

struct SamplerCreateInfo final
{
	Filter                  magFilter = FILTER_NEAREST;
	Filter                  minFilter = FILTER_NEAREST;
	SamplerMipmapMode       mipmapMode = SAMPLER_MIPMAP_MODE_NEAREST;
	SamplerAddressMode      addressModeU = SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerAddressMode      addressModeV = SAMPLER_ADDRESS_MODE_REPEAT;
	SamplerAddressMode      addressModeW = SAMPLER_ADDRESS_MODE_REPEAT;
	float                   mipLodBias = 0.0f;
	bool                    anisotropyEnable = false;
	float                   maxAnisotropy = 0.0f;
	bool                    compareEnable = false;
	CompareOp               compareOp = COMPARE_OP_NEVER;
	float                   minLod = 0.0f;
	float                   maxLod = 1.0f;
	BorderColor             borderColor = BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	SamplerYcbcrConversion* pYcbcrConversion = nullptr; // Leave null if not required.
	Ownership               ownership = OWNERSHIP_REFERENCE;
	SamplerCreateFlags      createFlags = {};
};

class Sampler final : public DeviceObject<SamplerCreateInfo>
{
public:
	VkSamplerPtr GetVkSampler() const { return mSampler; }

private:
	Result createApiObjects(const SamplerCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkSamplerPtr mSampler;
};

struct DepthStencilViewCreateInfo final
{
	Image* pImage = nullptr;
	ImageViewType     imageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
	Format            format = FORMAT_UNDEFINED;
	uint32_t          mipLevel = 0;
	uint32_t          mipLevelCount = 0;
	uint32_t          arrayLayer = 0;
	uint32_t          arrayLayerCount = 0;
	ComponentMapping  components = {};
	AttachmentLoadOp  depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp depthStoreOp = ATTACHMENT_STORE_OP_STORE;
	AttachmentLoadOp  stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
	Ownership         ownership = OWNERSHIP_REFERENCE;

	static DepthStencilViewCreateInfo GuessFromImage(Image* pImage);
};

class DepthStencilView final : public DeviceObject<DepthStencilViewCreateInfo>, public ImageView
{
public:
	ImagePtr          GetImage() const { return m_createInfo.pImage; }
	Format            GetFormat() const { return m_createInfo.format; }
	SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t          GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t          GetArrayLayer() const { return m_createInfo.arrayLayer; }
	AttachmentLoadOp  GetDepthLoadOp() const { return m_createInfo.depthLoadOp; }
	AttachmentStoreOp GetDepthStoreOp() const { return m_createInfo.depthStoreOp; }
	AttachmentLoadOp  GetStencilLoadOp() const { return m_createInfo.stencilLoadOp; }
	AttachmentStoreOp GetStencilStoreOp() const { return m_createInfo.stencilStoreOp; }

	VkImageViewPtr GetVkImageView() const { return mImageView; }

private:
	Result createApiObjects(const DepthStencilViewCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkImageViewPtr mImageView;
};

struct RenderTargetViewCreateInfo
{
	Image*            pImage = nullptr;
	ImageViewType     imageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
	Format            format = FORMAT_UNDEFINED;
	uint32_t          mipLevel = 0;
	uint32_t          mipLevelCount = 0;
	uint32_t          arrayLayer = 0;
	uint32_t          arrayLayerCount = 0;
	ComponentMapping  components = {};
	AttachmentLoadOp  loadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp storeOp = ATTACHMENT_STORE_OP_STORE;
	Ownership         ownership = OWNERSHIP_REFERENCE;

	static RenderTargetViewCreateInfo GuessFromImage(Image* pImage);
};

class RenderTargetView final : public DeviceObject<RenderTargetViewCreateInfo>, public ImageView
{
public:
	ImagePtr          GetImage() const { return m_createInfo.pImage; }
	Format            GetFormat() const { return m_createInfo.format; }
	SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t          GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t         GetArrayLayer() const { return m_createInfo.arrayLayer; }
	AttachmentLoadOp  GetLoadOp() const { return m_createInfo.loadOp; }
	AttachmentStoreOp GetStoreOp() const { return m_createInfo.storeOp; }

	VkImageViewPtr GetVkImageView() const { return mImageView; }

private:
	Result createApiObjects(const RenderTargetViewCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkImageViewPtr mImageView;
};

struct SampledImageViewCreateInfo
{
	Image*                  pImage = nullptr;
	ImageViewType           imageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
	Format                  format = FORMAT_UNDEFINED;
	SampleCount             sampleCount = SAMPLE_COUNT_1;
	uint32_t                mipLevel = 0;
	uint32_t                mipLevelCount = 0;
	uint32_t                arrayLayer = 0;
	uint32_t                arrayLayerCount = 0;
	ComponentMapping        components = {};
	SamplerYcbcrConversion* pYcbcrConversion = nullptr; // Leave null if not required.
	Ownership               ownership = OWNERSHIP_REFERENCE;

	static SampledImageViewCreateInfo GuessFromImage(Image* pImage);
};

class SampledImageView final : public DeviceObject<SampledImageViewCreateInfo>, public ImageView
{
public:
	ImagePtr                GetImage() const { return m_createInfo.pImage; }
	ImageViewType           GetImageViewType() const { return m_createInfo.imageViewType; }
	Format                  GetFormat() const { return m_createInfo.format; }
	SampleCount             GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t                GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t                GetMipLevelCount() const { return m_createInfo.mipLevelCount; }
	uint32_t                GetArrayLayer() const { return m_createInfo.arrayLayer; }
	uint32_t                GetArrayLayerCount() const { return m_createInfo.arrayLayerCount; }
	const ComponentMapping& GetComponents() const { return m_createInfo.components; }

	VkImageViewPtr GetVkImageView() const { return mImageView; }

private:
	Result createApiObjects(const SampledImageViewCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkImageViewPtr mImageView;
};

// SamplerYcbcrConversionCreateInfo defines a color model conversion for a texture, sampler, or sampled image.
struct SamplerYcbcrConversionCreateInfo final
{
	Format               format = FORMAT_UNDEFINED;
	YcbcrModelConversion ycbcrModel = YCBCR_MODEL_CONVERSION_RGB_IDENTITY;
	YcbcrRange           ycbcrRange = YCBCR_RANGE_ITU_FULL;
	ComponentMapping     components = {};
	ChromaLocation       xChromaOffset = CHROMA_LOCATION_COSITED_EVEN;
	ChromaLocation       yChromaOffset = CHROMA_LOCATION_COSITED_EVEN;
	Filter               filter = FILTER_LINEAR;
	bool                 forceExplicitReconstruction = false;
};

class SamplerYcbcrConversion final : public DeviceObject<SamplerYcbcrConversionCreateInfo>
{
public:
	VkSamplerYcbcrConversionPtr GetVkSamplerYcbcrConversion() const { return mSamplerYcbcrConversion; }

private:
	Result createApiObjects(const SamplerYcbcrConversionCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkSamplerYcbcrConversionPtr mSamplerYcbcrConversion;
};

struct StorageImageViewCreateInfo
{
	Image* pImage = nullptr;
	ImageViewType    imageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
	Format           format = FORMAT_UNDEFINED;
	uint32_t         mipLevel = 0;
	uint32_t         mipLevelCount = 0;
	uint32_t         arrayLayer = 0;
	uint32_t         arrayLayerCount = 0;
	ComponentMapping components = {};
	Ownership        ownership = OWNERSHIP_REFERENCE;

	static StorageImageViewCreateInfo GuessFromImage(Image* pImage);
};

class StorageImageView final : public DeviceObject<StorageImageViewCreateInfo>, public ImageView
{
public:
	ImagePtr      GetImage() const { return m_createInfo.pImage; }
	ImageViewType GetImageViewType() const { return m_createInfo.imageViewType; }
	Format        GetFormat() const { return m_createInfo.format; }
	SampleCount   GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t      GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t      GetMipLevelCount() const { return m_createInfo.mipLevelCount; }
	uint32_t      GetArrayLayer() const { return m_createInfo.arrayLayer; }
	uint32_t      GetArrayLayerCount() const { return m_createInfo.arrayLayerCount; }

	VkImageViewPtr GetVkImageView() const { return mImageView; }

private:
	Result createApiObjects(const StorageImageViewCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkImageViewPtr mImageView;
};

#pragma endregion

#pragma region Texture

struct TextureCreateInfo final
{
	Image*                  pImage = nullptr;
	ImageType               imageType = IMAGE_TYPE_2D;
	uint32_t                width = 0;
	uint32_t                height = 0;
	uint32_t                depth = 0;
	Format                  imageFormat = FORMAT_UNDEFINED;
	SampleCount             sampleCount = SAMPLE_COUNT_1;
	uint32_t                mipLevelCount = 1;
	uint32_t                arrayLayerCount = 1;
	ImageUsageFlags         usageFlags = ImageUsageFlags::SampledImage();
	MemoryUsage             memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ResourceState           initialState = RESOURCE_STATE_GENERAL;            // This may not be the best choice
	RenderTargetClearValue  RTVClearValue = { 0, 0, 0, 0 };                   // Optimized RTV clear value
	DepthStencilClearValue  DSVClearValue = { 1.0f, 0xFF };                   // Optimized DSV clear value
	ImageViewType           sampledImageViewType = IMAGE_VIEW_TYPE_UNDEFINED; // Guesses from image if UNDEFINED
	Format                  sampledImageViewFormat = FORMAT_UNDEFINED;        // Guesses from image if UNDEFINED
	SamplerYcbcrConversion* pSampledImageYcbcrConversion = nullptr;           // Leave null if not Ycbcr, or not using sampled image.
	Format                  renderTargetViewFormat = FORMAT_UNDEFINED;         // Guesses from image if UNDEFINED
	Format                  depthStencilViewFormat = FORMAT_UNDEFINED;         // Guesses from image if UNDEFINED
	Format                  storageImageViewFormat = FORMAT_UNDEFINED;         // Guesses from image if UNDEFINED
	Ownership               ownership = OWNERSHIP_REFERENCE;
	bool                    concurrentMultiQueueUsage = false;
	ImageCreateFlags        imageCreateFlags = {};
};

class Texture : public DeviceObject<TextureCreateInfo>
{
	friend class RenderDevice;
public:
	ImageType              GetImageType() const;
	uint32_t               GetWidth() const;
	uint32_t               GetHeight() const;
	uint32_t               GetDepth() const;
	Format                 GetImageFormat() const;
	SampleCount            GetSampleCount() const;
	uint32_t               GetMipLevelCount() const;
	uint32_t               GetArrayLayerCount() const;
	const ImageUsageFlags& GetUsageFlags() const;
	MemoryUsage            GetMemoryUsage() const;

	Format GetSampledImageViewFormat() const;
	Format GetRenderTargetViewFormat() const;
	Format GetDepthStencilViewFormat() const;
	Format GetStorageImageViewFormat() const;

	ImagePtr            GetImage() const { return m_image; }
	SampledImageViewPtr GetSampledImageView() const { return m_sampledImageView; }
	RenderTargetViewPtr GetRenderTargetView() const { return m_renderTargetView; }
	DepthStencilViewPtr GetDepthStencilView() const { return m_depthStencilView; }
	StorageImageViewPtr GetStorageImageView() const { return m_storageImageView; }

private:
	Result create(const TextureCreateInfo& pCreateInfo) final;
	Result createApiObjects(const TextureCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	ImagePtr            m_image;
	SampledImageViewPtr m_sampledImageView;
	RenderTargetViewPtr m_renderTargetView;
	DepthStencilViewPtr m_depthStencilView;
	StorageImageViewPtr m_storageImageView;
};


#pragma endregion

#pragma region RenderPass

// Use this if the RTVs and/or the DSV exists.
struct RenderPassCreateInfo final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               arrayLayerCount = 1;
	uint32_t               renderTargetCount = 0;
	MultiViewState         multiViewState = {};
	RenderTargetView*      pRenderTargetViews[MAX_RENDER_TARGETS] = {};
	DepthStencilView*      pDepthStencilView = nullptr;
	ResourceState          depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	Ownership              ownership = OWNERSHIP_REFERENCE;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this
	// RenderPass must use the same shading rate mode
	// (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr pShadingRatePattern = nullptr;

	void SetAllRenderTargetClearValue(const RenderTargetClearValue& value);
};

// Use this version if the format(s) are know but images and views need creation.
// RTVs, DSV, and backing images will be created using the criteria provided in this struct.
struct RenderPassCreateInfo2 final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               arrayLayerCount = 1;
	MultiViewState         multiViewState = {};
	SampleCount            sampleCount = SAMPLE_COUNT_1;
	uint32_t               renderTargetCount = 0;
	Format                 renderTargetFormats[MAX_RENDER_TARGETS] = {};
	Format                 depthStencilFormat = FORMAT_UNDEFINED;
	ImageUsageFlags        renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
	ImageUsageFlags        depthStencilUsageFlags = {};
	RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	AttachmentLoadOp       renderTargetLoadOps[MAX_RENDER_TARGETS] = { ATTACHMENT_LOAD_OP_LOAD };
	AttachmentStoreOp      renderTargetStoreOps[MAX_RENDER_TARGETS] = { ATTACHMENT_STORE_OP_STORE };
	AttachmentLoadOp       depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp      depthStoreOp = ATTACHMENT_STORE_OP_STORE;
	AttachmentLoadOp       stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp      stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
	ResourceState          renderTargetInitialStates[MAX_RENDER_TARGETS] = { RESOURCE_STATE_UNDEFINED };
	ResourceState          depthStencilInitialState = RESOURCE_STATE_UNDEFINED;
	Ownership              ownership = OWNERSHIP_REFERENCE;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this RenderPass must use the same shading rate mode
	// (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr pShadingRatePattern = nullptr;

	void SetAllRenderTargetUsageFlags(const ImageUsageFlags& flags);
	void SetAllRenderTargetClearValue(const RenderTargetClearValue& value);
	void SetAllRenderTargetLoadOp(AttachmentLoadOp op);
	void SetAllRenderTargetStoreOp(AttachmentStoreOp op);
	void SetAllRenderTargetToClear();
};

// Use this if the images exists but views need creation.
struct RenderPassCreateInfo3 final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               renderTargetCount = 0;
	uint32_t               arrayLayerCount = 1;
	MultiViewState         multiViewState = {};
	Image*                 pRenderTargetImages[MAX_RENDER_TARGETS] = {};
	Image*                 pDepthStencilImage = nullptr;
	ResourceState          depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	AttachmentLoadOp       renderTargetLoadOps[MAX_RENDER_TARGETS] = { ATTACHMENT_LOAD_OP_LOAD };
	AttachmentStoreOp      renderTargetStoreOps[MAX_RENDER_TARGETS] = { ATTACHMENT_STORE_OP_STORE };
	AttachmentLoadOp       depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp      depthStoreOp = ATTACHMENT_STORE_OP_STORE;
	AttachmentLoadOp       stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	AttachmentStoreOp      stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
	Ownership              ownership = OWNERSHIP_REFERENCE;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this RenderPass must use the same shading rate mode
	// (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr pShadingRatePattern = nullptr;

	void SetAllRenderTargetClearValue(const RenderTargetClearValue& value);
	void SetAllRenderTargetLoadOp(AttachmentLoadOp op);
	void SetAllRenderTargetStoreOp(AttachmentStoreOp op);
	void SetAllRenderTargetToClear();
};

namespace internal
{
	struct RenderPassCreateInfo
	{
		enum CreateInfoVersion
		{
			CREATE_INFO_VERSION_UNDEFINED = 0,
			CREATE_INFO_VERSION_1 = 1,
			CREATE_INFO_VERSION_2 = 2,
			CREATE_INFO_VERSION_3 = 3,
		};

		Ownership           ownership = OWNERSHIP_REFERENCE;
		CreateInfoVersion   version = CREATE_INFO_VERSION_UNDEFINED;
		uint32_t            width = 0;
		uint32_t            height = 0;
		uint32_t            renderTargetCount = 0;
		uint32_t            arrayLayerCount = 1;
		MultiViewState      multiViewState = {};
		ResourceState       depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		ShadingRatePattern* pShadingRatePattern = nullptr;

		// Data unique to RenderPassCreateInfo
		struct
		{
			RenderTargetView* pRenderTargetViews[MAX_RENDER_TARGETS] = {};
			DepthStencilView* pDepthStencilView = nullptr;
		} V1;

		// Data unique to RenderPassCreateInfo2
		struct
		{
			SampleCount     sampleCount = SAMPLE_COUNT_1;
			Format          renderTargetFormats[MAX_RENDER_TARGETS] = {};
			Format          depthStencilFormat = FORMAT_UNDEFINED;
			ImageUsageFlags renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
			ImageUsageFlags depthStencilUsageFlags = {};
			ResourceState   renderTargetInitialStates[MAX_RENDER_TARGETS] = { RESOURCE_STATE_UNDEFINED };
			ResourceState   depthStencilInitialState = RESOURCE_STATE_UNDEFINED;
		} V2;

		// Data unique to RenderPassCreateInfo3
		struct
		{
			Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
			Image* pDepthStencilImage = nullptr;
		} V3;

		// Clear values
		RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		DepthStencilClearValue depthStencilClearValue = {};

		// Load/store ops
		AttachmentLoadOp  renderTargetLoadOps[MAX_RENDER_TARGETS] = { ATTACHMENT_LOAD_OP_LOAD };
		AttachmentStoreOp renderTargetStoreOps[MAX_RENDER_TARGETS] = { ATTACHMENT_STORE_OP_STORE };
		AttachmentLoadOp  depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		AttachmentStoreOp depthStoreOp = ATTACHMENT_STORE_OP_STORE;
		AttachmentLoadOp  stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		AttachmentStoreOp stencilStoreOp = ATTACHMENT_STORE_OP_STORE;

		RenderPassCreateInfo() {}
		RenderPassCreateInfo(const ::RenderPassCreateInfo& obj);
		RenderPassCreateInfo(const ::RenderPassCreateInfo2& obj);
		RenderPassCreateInfo(const ::RenderPassCreateInfo3& obj);
	};

}

class RenderPass final: public DeviceObject<internal::RenderPassCreateInfo>
{
	friend class RenderDevice;
public:
	const Rect& GetRenderArea() const { return mRenderArea; }
	const Rect& GetScissor() const { return mRenderArea; }
	const Viewport& GetViewport() const { return mViewport; }

	uint32_t GetRenderTargetCount() const { return m_createInfo.renderTargetCount; }
	bool     HasDepthStencil() const { return mDepthStencilImage ? true : false; }

	Result GetRenderTargetView(uint32_t index, RenderTargetView** ppView) const;
	Result GetDepthStencilView(DepthStencilView** ppView) const;

	Result GetRenderTargetImage(uint32_t index, Image** ppImage) const;
	Result GetDepthStencilImage(Image** ppImage) const;

	// This only applies to RenderPass objects created using RenderPassCreateInfo2.
	// These functions will set 'isExternal' to true resulting in these objects NOT getting destroyed when the encapsulating RenderPass object is destroyed.
	// Calling these fuctions on RenderPass objects created using using RenderPassCreateInfo will still return a valid object if the index or DSV object exists.
	Result DisownRenderTargetView(uint32_t index, RenderTargetView** ppView);
	Result DisownDepthStencilView(DepthStencilView** ppView);
	Result DisownRenderTargetImage(uint32_t index, Image** ppImage);
	Result DisownDepthStencilImage(Image** ppImage);

	// Convenience functions returns empty ptr if index is out of range or DSV object does not exist.
	RenderTargetViewPtr GetRenderTargetView(uint32_t index) const;
	DepthStencilViewPtr GetDepthStencilView() const;
	ImagePtr            GetRenderTargetImage(uint32_t index) const;
	ImagePtr            GetDepthStencilImage() const;

	// Returns index of pImage otherwise returns UINT32_MAX
	uint32_t GetRenderTargetImageIndex(const Image* pImage) const;

	// Returns true if render targets or depth stencil contains ATTACHMENT_LOAD_OP_CLEAR
	bool HasLoadOpClear() const { return mHasLoadOpClear; }

	VkRenderPassPtr  GetVkRenderPass() const { return mRenderPass; }
	VkFramebufferPtr GetVkFramebuffer() const { return mFramebuffer; }

private:
	Result create(const internal::RenderPassCreateInfo& pCreateInfo) final;
	Result createApiObjects(const internal::RenderPassCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;
	void   destroy() final;

	Result createImagesAndViewsV1(const internal::RenderPassCreateInfo& pCreateInfo);
	Result createImagesAndViewsV2(const internal::RenderPassCreateInfo& pCreateInfo);
	Result createImagesAndViewsV3(const internal::RenderPassCreateInfo& pCreateInfo);

	Result createRenderPass(const internal::RenderPassCreateInfo& pCreateInfo);
	Result createFramebuffer(const internal::RenderPassCreateInfo& pCreateInfo);

	VkRenderPassPtr  mRenderPass;
	VkFramebufferPtr mFramebuffer;

	Rect                             mRenderArea = {};
	Viewport                         mViewport = {};
	std::vector<RenderTargetViewPtr> mRenderTargetViews;
	DepthStencilViewPtr              mDepthStencilView;
	std::vector<ImagePtr>            mRenderTargetImages;
	ImagePtr                         mDepthStencilImage;
	bool                             mHasLoadOpClear = false;
};

VkResult CreateTransientRenderPass(
	RenderDevice* device,
	uint32_t              renderTargetCount,
	const VkFormat* pRenderTargetFormats,
	VkFormat              depthStencilFormat,
	VkSampleCountFlagBits sampleCount,
	uint32_t              viewMask,
	uint32_t              correlationMask,
	VkRenderPass* pRenderPass,
	ShadingRateMode shadingRateMode = SHADING_RATE_NONE);

#pragma endregion

#pragma region DrawPass

struct RenderPassBeginInfo;

// Use this version if the format(s) are known but images need creation.
// Backing images will be created using the criteria provided in this struct.
struct DrawPassCreateInfo
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	SampleCount            sampleCount = SAMPLE_COUNT_1;
	uint32_t               renderTargetCount = 0;
	Format                 renderTargetFormats[MAX_RENDER_TARGETS] = {};
	Format                 depthStencilFormat = FORMAT_UNDEFINED;
	ImageUsageFlags        renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
	ImageUsageFlags        depthStencilUsageFlags = {};
	ResourceState          renderTargetInitialStates[MAX_RENDER_TARGETS] = { RESOURCE_STATE_RENDER_TARGET };
	ResourceState          depthStencilInitialState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	ShadingRatePattern*    pShadingRatePattern = nullptr;
	ImageCreateFlags       imageCreateFlags = {};
};

// Use this version if the images exists.
struct DrawPassCreateInfo2
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               renderTargetCount = 0;
	Image*                 pRenderTargetImages[MAX_RENDER_TARGETS] = {};
	Image*                 pDepthStencilImage = nullptr;
	ResourceState          depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	ShadingRatePattern*    pShadingRatePattern = nullptr;
};

// Use this version if the textures exists.
struct DrawPassCreateInfo3
{
	uint32_t            width = 0;
	uint32_t            height = 0;
	uint32_t            renderTargetCount = 0;
	Texture*            pRenderTargetTextures[MAX_RENDER_TARGETS] = {};
	Texture*            pDepthStencilTexture = nullptr;
	ResourceState       depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
	ShadingRatePattern* pShadingRatePattern = nullptr;
};

namespace internal
{

	struct DrawPassCreateInfo
	{
		enum CreateInfoVersion
		{
			CREATE_INFO_VERSION_UNDEFINED = 0,
			CREATE_INFO_VERSION_1 = 1,
			CREATE_INFO_VERSION_2 = 2,
			CREATE_INFO_VERSION_3 = 3,
		};

		CreateInfoVersion   version = CREATE_INFO_VERSION_UNDEFINED;
		uint32_t            width = 0;
		uint32_t            height = 0;
		uint32_t            renderTargetCount = 0;
		ResourceState       depthStencilState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
		ShadingRatePattern* pShadingRatePattern = nullptr;

		// Data unique to DrawPassCreateInfo1
		struct
		{
			SampleCount      sampleCount = SAMPLE_COUNT_1;
			Format           renderTargetFormats[MAX_RENDER_TARGETS] = {};
			Format           depthStencilFormat = FORMAT_UNDEFINED;
			ImageUsageFlags  renderTargetUsageFlags[MAX_RENDER_TARGETS] = {};
			ImageUsageFlags  depthStencilUsageFlags = {};
			ResourceState    renderTargetInitialStates[MAX_RENDER_TARGETS] = { RESOURCE_STATE_RENDER_TARGET };
			ResourceState    depthStencilInitialState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
			ImageCreateFlags imageCreateFlags = {};
		} V1;

		// Data unique to DrawPassCreateInfo2
		struct
		{
			Image* pRenderTargetImages[MAX_RENDER_TARGETS] = {};
			Image* pDepthStencilImage = nullptr;
		} V2;

		// Data unique to DrawPassCreateInfo3
		struct
		{
			Texture* pRenderTargetTextures[MAX_RENDER_TARGETS] = {};
			Texture* pDepthStencilTexture = nullptr;
		} V3;

		// Clear values
		RenderTargetClearValue renderTargetClearValues[MAX_RENDER_TARGETS] = {};
		DepthStencilClearValue depthStencilClearValue = {};

		DrawPassCreateInfo() {}
		DrawPassCreateInfo(const ::DrawPassCreateInfo& obj);
		DrawPassCreateInfo(const ::DrawPassCreateInfo2& obj);
		DrawPassCreateInfo(const ::DrawPassCreateInfo3& obj);
	};

}

class DrawPass final : public DeviceObject<internal::DrawPassCreateInfo>
{
public:
	uint32_t              GetWidth() const { return m_createInfo.width; }
	uint32_t              GetHeight() const { return m_createInfo.height; }
	const Rect& GetRenderArea() const;
	const Rect& GetScissor() const;
	const Viewport& GetViewport() const;

	uint32_t       GetRenderTargetCount() const { return m_createInfo.renderTargetCount; }
	Result         GetRenderTargetTexture(uint32_t index, Texture** ppRenderTarget) const;
	Texture* GetRenderTargetTexture(uint32_t index) const;
	bool           HasDepthStencil() const { return mDepthStencilTexture ? true : false; }
	Result         GetDepthStencilTexture(Texture** ppDepthStencil) const;
	Texture* GetDepthStencilTexture() const;

	void PrepareRenderPassBeginInfo(const DrawPassClearFlags& clearFlags, RenderPassBeginInfo* pBeginInfo) const;

private:
	Result createApiObjects(const internal::DrawPassCreateInfo& pCreateInfo) final;
	void destroyApiObjects() final;

	Result CreateTexturesV1(const internal::DrawPassCreateInfo& pCreateInfo);
	Result CreateTexturesV2(const internal::DrawPassCreateInfo& pCreateInfo);
	Result CreateTexturesV3(const internal::DrawPassCreateInfo& pCreateInfo);

	Rect                    mRenderArea = {};
	std::vector<TexturePtr> mRenderTargetTextures;
	TexturePtr              mDepthStencilTexture;

	struct Pass
	{
		uint32_t            clearMask = 0;
		RenderPassPtr renderPass;
	};

	std::vector<Pass> mPasses;
};

#pragma endregion

#pragma region Descriptor

struct DescriptorBinding final
{
	uint32_t                binding = VALUE_IGNORED;
	DescriptorType          type = DESCRIPTOR_TYPE_UNDEFINED;
	uint32_t                arrayCount = 1; // WARNING: Not VkDescriptorSetLayoutBinding::descriptorCount
	ShaderStageBits         shaderVisiblity = SHADER_STAGE_ALL; // Single value not set of flags (see note above)
	DescriptorBindingFlags  flags;
	std::vector<SamplerPtr> immutableSamplers;

	DescriptorBinding() {}

	DescriptorBinding(
		uint32_t               binding_,
		DescriptorType         type_,
		uint32_t               arrayCount_ = 1,
		ShaderStageBits        shaderVisiblity_ = SHADER_STAGE_ALL,
		DescriptorBindingFlags flags_ = 0)
		: binding(binding_),
		type(type_),
		arrayCount(arrayCount_),
		shaderVisiblity(shaderVisiblity_),
		flags(flags_) {}
};

struct WriteDescriptor
{
	uint32_t               binding = VALUE_IGNORED;
	uint32_t               arrayIndex = 0;
	DescriptorType         type = DESCRIPTOR_TYPE_UNDEFINED;
	uint64_t               bufferOffset = 0;
	uint64_t               bufferRange = 0;
	uint32_t               structuredElementCount = 0;
	const Buffer* pBuffer = nullptr;
	const ImageView* pImageView = nullptr;
	const Sampler* pSampler = nullptr;
};

struct DescriptorPoolCreateInfo
{
	uint32_t sampler = 0;
	uint32_t combinedImageSampler = 0;
	uint32_t sampledImage = 0;
	uint32_t storageImage = 0;
	uint32_t uniformTexelBuffer = 0;
	uint32_t storageTexelBuffer = 0;
	uint32_t uniformBuffer = 0;
	uint32_t rawStorageBuffer = 0;
	uint32_t structuredBuffer = 0;
	uint32_t uniformBufferDynamic = 0;
	uint32_t storageBufferDynamic = 0;
	uint32_t inputAttachment = 0;
};

class DescriptorPool final : public DeviceObject<DescriptorPoolCreateInfo>
{
public:
	VkDescriptorPoolPtr GetVkDescriptorPool() const { return mDescriptorPool; }

private:
	Result createApiObjects(const DescriptorPoolCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkDescriptorPoolPtr mDescriptorPool;
};

namespace internal
{
	struct DescriptorSetCreateInfo
	{
		DescriptorPool* pPool = nullptr;
		const DescriptorSetLayout* pLayout = nullptr;
	};
}

class DescriptorSet final : public DeviceObject<internal::DescriptorSetCreateInfo>
{
public:
	DescriptorPoolPtr          GetPool() const { return m_createInfo.pPool; }
	const DescriptorSetLayout* GetLayout() const { return m_createInfo.pLayout; }

	Result UpdateDescriptors(uint32_t writeCount, const WriteDescriptor* pWrites);

	Result UpdateSampler(
		uint32_t             binding,
		uint32_t             arrayIndex,
		const Sampler* pSampler);

	Result UpdateSampledImage(
		uint32_t                      binding,
		uint32_t                      arrayIndex,
		const SampledImageView* pImageView);

	Result UpdateSampledImage(
		uint32_t             binding,
		uint32_t             arrayIndex,
		const Texture* pTexture);

	Result UpdateStorageImage(
		uint32_t             binding,
		uint32_t             arrayIndex,
		const Texture* pTexture);

	Result UpdateUniformBuffer(
		uint32_t            binding,
		uint32_t            arrayIndex,
		const Buffer* pBuffer,
		uint64_t            offset = 0,
		uint64_t            range = WHOLE_SIZE);

	VkDescriptorSetPtr GetVkDescriptorSet() const { return mDescriptorSet; }

private:
	Result createApiObjects(const internal::DescriptorSetCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkDescriptorSetPtr  mDescriptorSet;
	VkDescriptorPoolPtr mDescriptorPool;

	// Reduce memory allocations during update process
	std::vector<VkWriteDescriptorSet>   mWriteStore;
	std::vector<VkDescriptorImageInfo>  mImageInfoStore;
	std::vector<VkBufferView>           mTexelBufferStore;
	std::vector<VkDescriptorBufferInfo> mBufferInfoStore;
	uint32_t                            mWriteCount = 0;
	uint32_t                            mImageCount = 0;
	uint32_t                            mTexelBufferCount = 0;
	uint32_t                            mBufferCount = 0;
};

struct DescriptorSetLayoutCreateInfo
{
	DescriptorSetLayoutFlags       flags;
	std::vector<DescriptorBinding> bindings;
};

class DescriptorSetLayout final : public DeviceObject<DescriptorSetLayoutCreateInfo>
{
	friend class RenderDevice;
public:
	bool IsPushable() const { return m_createInfo.flags.bits.pushable; }

	const std::vector<DescriptorBinding>& GetBindings() const { return m_createInfo.bindings; }

	VkDescriptorSetLayoutPtr GetVkDescriptorSetLayout() const { return mDescriptorSetLayout; }

private:
	Result create(const DescriptorSetLayoutCreateInfo& pCreateInfo) final;

	Result createApiObjects(const DescriptorSetLayoutCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;
	Result validateDescriptorBindingFlags(const DescriptorBindingFlags& flags) const;

	VkDescriptorSetLayoutPtr mDescriptorSetLayout;
};

#pragma endregion

#pragma region Shader

struct ShaderModuleCreateInfo final
{
	uint32_t    size = 0;
	const char* pCode = nullptr;
};

class ShaderModule final : public DeviceObject<ShaderModuleCreateInfo>
{
public:
	VkShaderModulePtr GetVkShaderModule() const { return mShaderModule; }

private:
	Result createApiObjects(const ShaderModuleCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkShaderModulePtr mShaderModule;
};

#pragma endregion

#pragma region ShadingRate

// A supported shading rate supported by the device.
struct SupportedShadingRate final
{
	// Bit mask of supported sample counts.
	uint32_t sampleCountMask;

	// Size of the shading rate fragment size.
	Extent2D fragmentSize;
};

// Information about GPU support for shading rate features.
struct ShadingRateCapabilities final
{
	// The shading rate mode supported by this device.
	ShadingRateMode supportedShadingRateMode = SHADING_RATE_NONE;

	struct
	{
		bool supportsNonSubsampledImages;

		// Minimum/maximum size of the region of the render target
		// corresponding to a single pixel in the FDM attachment.
		// This is *not* the minimum/maximum fragment density.
		Extent2D minTexelSize;
		Extent2D maxTexelSize;
	} fdm;

	struct
	{
		// Minimum/maximum size of the region of the render target
		// corresponding to a single pixel in the VRS attachment.
		// This is *not* the shading rate itself.
		Extent2D minTexelSize;
		Extent2D maxTexelSize;

		// List of supported shading rates.
		std::vector<SupportedShadingRate> supportedRates;
	} vrs;
};

// Encodes fragment densities/sizes into the format needed for a ShadingRatePattern.
class ShadingRateEncoder
{
public:
	virtual ~ShadingRateEncoder() = default;

	// Encode a pair of fragment density values.
	//
	// Fragment density values are a ratio over 255, e.g. 255 means shade every
	// pixel, and 128 means shade every other pixel.
	virtual uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const = 0;

	// Encode a pair of fragment size values.
	//
	// The fragmentWidth/fragmentHeight values are in pixels.
	virtual uint32_t EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const = 0;
};

namespace internal
{
	// Encodes fragment sizes/densities for FDM.
	class FDMShadingRateEncoder final : public ShadingRateEncoder
	{
	public:
		uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const final;
		uint32_t EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const final;

	private:
		static uint32_t encodeFragmentDensityImpl(uint8_t xDensity, uint8_t yDensity);
	};

	// Encodes fragment sizes/densities for VRS.
	class VRSShadingRateEncoder final : public ShadingRateEncoder
	{
	public:
		void Initialize(SampleCount sampleCount, const ShadingRateCapabilities& capabilities);

		uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const final;
		uint32_t EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const final;

	private:
		// Maximum encoded value of a shading rate.
		static constexpr size_t kMaxEncodedShadingRate = (2 << 2) | 2;

		uint32_t        encodeFragmentSizeImpl(uint8_t xDensity, uint8_t yDensity) const;
		static uint32_t rawEncode(uint8_t width, uint8_t height);

		// Maps a requested shading rate to a supported shading rate.
		// The fragment width/height of the supported shading rate will be no larger than the fragment width/height of the requested shading rate.
		// Ties are broken lexicographically, e.g. if 2x2, 1x4 and 4x1 are supported, then 2x4 will be mapped to 2x2 but 4x2 will map to 4x1.
		std::array<uint8_t, kMaxEncodedShadingRate + 1> mMapRateToSupported;
	};
} // namespace internal

struct ShadingRatePatternCreateInfo final
{
	// The size of the framebuffer image that will be used with the created ShadingRatePattern.
	Extent2D framebufferSize;

	// The size of the region of the framebuffer image that will correspond to a single pixel in the ShadingRatePattern image.
	Extent2D texelSize;

	// The shading rate mode (FDM or VRS).
	ShadingRateMode shadingRateMode = SHADING_RATE_NONE;

	// The sample count of the render targets using this shading rate pattern.
	SampleCount sampleCount;
};

// An image representing fragment sizes/densities that can be used in a render pass to control the shading rate.
class ShadingRatePattern final : public DeviceObject<ShadingRatePatternCreateInfo>
{
public:
	// The shading rate mode (FDM or VRS).
	ShadingRateMode GetShadingRateMode() const { return mShadingRateMode; }

	// The image contaning encoded fragment sizes/densities.
	ImagePtr GetAttachmentImage() const { return mAttachmentImage; }

	// The width/height of the image contaning encoded fragment sizes/densities.
	uint32_t GetAttachmentWidth() const { return mAttachmentImage->GetWidth(); }
	uint32_t GetAttachmentHeight() const { return mAttachmentImage->GetHeight(); }

	// The width/height of the region of the render target image corresponding
	// to a single pixel in the image containing fragment sizes/densities.
	uint32_t GetTexelWidth() const { return mTexelSize.width; }
	uint32_t GetTexelHeight() const { return mTexelSize.height; }

	// The sample count of the render targets using this shading rate pattern.
	SampleCount GetSampleCount() const { return mSampleCount; }

	// Create a bitmap suitable for uploading fragment density/size to this pattern.
	std::unique_ptr<Bitmap> CreateBitmap() const;

	// Load fragment density/size from a bitmap of encoded values.
	Result LoadFromBitmap(Bitmap* bitmap);

	// Get the pixel format of a bitmap that can store the fragment density/size data.
	Bitmap::Format GetBitmapFormat() const;

	// Get an encoder that can encode fragment density/size values for this pattern.
	const ShadingRateEncoder* GetShadingRateEncoder() const;

	VkImageViewPtr GetAttachmentImageView() const
	{
		return mAttachmentView;
	}

	// Creates a modified version of the render pass create info which supports the required shading rate mode.
	// The shared_ptr also manages the memory of all referenced pointers and arrays in the VkRenderPassCreateInfo2 struct.
	std::shared_ptr<const VkRenderPassCreateInfo2>        GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci);
	std::shared_ptr<const VkRenderPassCreateInfo2>        GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo2& vkci);
	static std::shared_ptr<const VkRenderPassCreateInfo2> GetModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode, const VkRenderPassCreateInfo& vkci);
	static std::shared_ptr<const VkRenderPassCreateInfo2> GetModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode, const VkRenderPassCreateInfo2& vkci);

private:
	// Handles modification of VkRenderPassCreateInfo/VkRenderPassCreateInfo2 to add support for a ShadingRatePattern.
	// The ModifiedRenderPassCreateInfo object handles the lifetimes of the pointers and arrays referenced in the modified VkRenderPassCreateInfo2.
	class ModifiedRenderPassCreateInfo : public std::enable_shared_from_this<ModifiedRenderPassCreateInfo>
	{
	public:
		virtual ~ModifiedRenderPassCreateInfo() = default;

		// Initializes the modified VkRenderPassCreateInfo2, based on the
		// values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2,
		// with appropriate modifications for the shading rate implementation.
		ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo& vkci);
		ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo2& vkci);

		// Returns the modified VkRenderPassCreateInfo2.
		//
		// The returned pointer, as well as pointers and arrays inside the
		// VkRenderPassCreateInfo2 struct, point to memory owned by this
		/// ModifiedRenderPassCreateInfo object, and so cannot be used after
		// this object is destroyed.
		std::shared_ptr<const VkRenderPassCreateInfo2> Get()
		{
			return std::shared_ptr<const VkRenderPassCreateInfo2>(shared_from_this(), &mVkRenderPassCreateInfo2);
		}

	protected:
		// Initializes the internal VkRenderPassCreateInfo2, based on the values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2.
		// All arrays are copied to internal vectors, and the internal
		// VkRenderPassCreateInfo2 references the data in these vectors, rather than the poitners in the input VkRenderPassCreateInfo.
		void LoadVkRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci);
		void LoadVkRenderPassCreateInfo2(const VkRenderPassCreateInfo2& vkci);

		// Modifies the internal VkRenderPassCreateInfo2 to enable the shading rate implementation.
		virtual void UpdateRenderPassForShadingRateImplementation() = 0;

		VkRenderPassCreateInfo2               mVkRenderPassCreateInfo2 = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2 };
		std::vector<VkAttachmentDescription2> mAttachments;
		std::vector<VkSubpassDescription2>    mSubpasses;
		struct SubpassAttachments
		{
			std::vector<VkAttachmentReference2> inputAttachments;
			std::vector<VkAttachmentReference2> colorAttachments;
			std::vector<VkAttachmentReference2> resolveAttachments;
			VkAttachmentReference2              depthStencilAttachment;
			std::vector<uint32_t>               preserveAttachments;
		};
		std::vector<SubpassAttachments>   mSubpassAttachments;
		std::vector<VkSubpassDependency2> mDependencies;
	};

	// Creates a ModifiedRenderPassCreateInfo that will modify
	// VkRenderPassCreateInfo/VkRenderPassCreateInfo2  to support the given ShadingRateMode on the given device.
	static std::shared_ptr<ModifiedRenderPassCreateInfo> CreateModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode);

	// Creates a ModifiedRenderPassCreateInfo that will modify
	// VkRenderPassCreateInfo/VkRenderPassCreateInfo2 to support this ShadingRatePattern.
	std::shared_ptr<ModifiedRenderPassCreateInfo> CreateModifiedRenderPassCreateInfo() const
	{
		return CreateModifiedRenderPassCreateInfo(GetDevice(), GetShadingRateMode());
	}

	// Handles modification of VkRenderPassCreateInfo(2) to add support for FDM.
	class FDMModifiedRenderPassCreateInfo : public ModifiedRenderPassCreateInfo
	{
	protected:
		void UpdateRenderPassForShadingRateImplementation() override;

	private:
		VkRenderPassFragmentDensityMapCreateInfoEXT mFdmInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT };
	};

	// Handles modification of VkRenderPassCreateInfo(2) to add support for VRS.
	class VRSModifiedRenderPassCreateInfo : public ModifiedRenderPassCreateInfo
	{
	public:
		VRSModifiedRenderPassCreateInfo(const ShadingRateCapabilities& capabilities)
			: mCapabilities(capabilities) {}

	protected:
		void UpdateRenderPassForShadingRateImplementation() override;

	private:
		ShadingRateCapabilities                mCapabilities;
		VkFragmentShadingRateAttachmentInfoKHR mVrsAttachmentInfo = { VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR };
		VkAttachmentReference2                 mVrsAttachmentRef = { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR };
	};

	Result createApiObjects(const ShadingRatePatternCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	std::unique_ptr<ShadingRateEncoder> mShadingRateEncoder;
	VkImageViewPtr                      mAttachmentView;

	ShadingRateMode mShadingRateMode;
	ImagePtr        mAttachmentImage;
	Extent2D        mTexelSize;
	SampleCount     mSampleCount;
};

#pragma endregion

#pragma region Shading Rate Util

// These set of functions create a bitmap.The bitmap can be used to produce a ShadingRatePattern that in tun can be used in a render pass creation to specify a non - uniform shading pattern.

// A uniform bitmap that has cells of fragmentWidth and fragementHeight size
void FillShadingRateUniformFragmentSize(ShadingRatePatternPtr pattern, uint32_t fragmentWidth, uint32_t fragmentHeight, Bitmap* bitmap);
// A uniform bit map that has cells of xDensity and yDensity size
void FillShadingRateUniformFragmentDensity(ShadingRatePatternPtr pattern, uint32_t xDensity, uint32_t yDensity, Bitmap* bitmap);
// A map with cells of radial scale size
void FillShadingRateRadial(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);
// A map with cells produced by anisotropic filter of scale size
void FillShadingRateAnisotropic(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap);

#pragma endregion

#pragma region Mesh

struct MeshVertexAttribute
{
	Format format = FORMAT_UNDEFINED;

	// Use 0 to have stride calculated from format
	uint32_t stride = 0;

	// Not used for mesh/vertex buffer creation.
	// Gets calculated during creation for queries afterwards.
	uint32_t offset = 0;

	// [OPTIONAL] Useful for debugging.
	VertexSemantic vertexSemantic = VERTEX_SEMANTIC_UNDEFINED;
};

struct MeshVertexBufferDescription
{
	uint32_t                  attributeCount = 0;
	MeshVertexAttribute attributes[MAX_VERTEX_BINDINGS] = {};

	// Use 0 to have stride calculated from attributes
	uint32_t stride = 0;

	VertexInputRate vertexInputRate = VERTEX_INPUT_RATE_VERTEX;
};

//! @struct MeshCreateInfo
//!
//! Usage Notes:
//!   - Index and vertex data configuration needs to make sense
//!       - If \b indexCount is 0 then \b vertexCount cannot be 0
//!   - To create a mesh without an index buffer, \b indexType must be INDEX_TYPE_UNDEFINED
//!   - If \b vertexCount is 0 then no vertex buffers will be created
//!       - This means vertex buffer information will be ignored
//!   - Active elements in \b vertexBuffers cannot have an \b attributeCount of 0
//!
struct MeshCreateInfo
{
	IndexType                   indexType = INDEX_TYPE_UNDEFINED;
	uint32_t                          indexCount = 0;
	uint32_t                          vertexCount = 0;
	uint32_t                          vertexBufferCount = 0;
	MeshVertexBufferDescription vertexBuffers[MAX_VERTEX_BINDINGS] = {};
	MemoryUsage                 memoryUsage = MEMORY_USAGE_GPU_ONLY;

	MeshCreateInfo() {}
	MeshCreateInfo(const Geometry& geometry);
};

//! @class Mesh
//!
//! The \b Mesh class is a straight forward geometry container for the GPU.
//! A \b Mesh instance consists of vertex data and an optional index buffer.
//! The vertex data is stored in on or more vertex buffers. Each vertex buffer
//! can store data for one or more attributes. The index data is stored in an
//! index buffer.
//!
//! A \b Mesh instance does not store vertex binding information. Even if the
//! create info is derived from a Geometry instance. This design is
//! intentional since it enables calling applications to map vertex attributes
//! and vertex buffers to how it sees fit. For convenience, the function
//! \b Mesh::GetDerivedVertexBindings() returns vertex bindings derived from
//! a \Mesh instance's vertex buffer descriptions.
//!
class Mesh : public DeviceObject<MeshCreateInfo>
{
public:
	Mesh() {}
	virtual ~Mesh() {}

	IndexType GetIndexType() const { return m_createInfo.indexType; }
	uint32_t        GetIndexCount() const { return m_createInfo.indexCount; }
	BufferPtr GetIndexBuffer() const { return mIndexBuffer; }

	uint32_t                                 GetVertexCount() const { return m_createInfo.vertexCount; }
	uint32_t                                 GetVertexBufferCount() const { return CountU32(mVertexBuffers); }
	BufferPtr                          GetVertexBuffer(uint32_t index) const;
	const MeshVertexBufferDescription* GetVertexBufferDescription(uint32_t index) const;

	//! Returns derived vertex bindings based on the vertex buffer description
	const std::vector<VertexBinding>& GetDerivedVertexBindings() const { return mDerivedVertexBindings; }

private:
	Result createApiObjects(const MeshCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	BufferPtr                                                            mIndexBuffer;
	std::vector<std::pair<BufferPtr, MeshVertexBufferDescription>> mVertexBuffers;
	std::vector<VertexBinding>                                           mDerivedVertexBindings;
};

#pragma endregion

#pragma region Pipeline

struct ShaderStageInfo
{
	const ShaderModule* pModule = nullptr;
	std::string entryPoint = "";
};

struct ComputePipelineCreateInfo
{
	ShaderStageInfo          CS = {};
	const PipelineInterface* pPipelineInterface = nullptr;
};

class ComputePipeline final : public DeviceObject<ComputePipelineCreateInfo>
{
	friend class RenderDevice;
public:

	VkPipelinePtr GetVkPipeline() const { return mPipeline; }

private:
	Result create(const ComputePipelineCreateInfo& pCreateInfo) final;
	Result createApiObjects(const ComputePipelineCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkPipelinePtr mPipeline;
};

struct VertexInputState
{
	uint32_t            bindingCount = 0;
	VertexBinding bindings[MAX_VERTEX_BINDINGS] = {};
};

struct InputAssemblyState
{
	PrimitiveTopology topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	bool                    primitiveRestartEnable = false;
};

struct TessellationState
{
	uint32_t                       patchControlPoints = 0;
	TessellationDomainOrigin domainOrigin = TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
};

struct RasterState
{
	bool              depthClampEnable = false;
	bool              rasterizeDiscardEnable = false;
	PolygonMode polygonMode = POLYGON_MODE_FILL;
	CullMode    cullMode = CULL_MODE_NONE;
	FrontFace   frontFace = FRONT_FACE_CCW;
	bool              depthBiasEnable = false;
	float             depthBiasConstantFactor = 0.0f;
	float             depthBiasClamp = 0.0f;
	float             depthBiasSlopeFactor = 0.0f;
	bool              depthClipEnable = false;
	SampleCount rasterizationSamples = SAMPLE_COUNT_1;
};

struct MultisampleState
{
	bool alphaToCoverageEnable = false;
};

struct StencilOpState
{
	StencilOp failOp = STENCIL_OP_KEEP;
	StencilOp passOp = STENCIL_OP_KEEP;
	StencilOp depthFailOp = STENCIL_OP_KEEP;
	CompareOp compareOp = COMPARE_OP_NEVER;
	uint32_t        compareMask = 0;
	uint32_t        writeMask = 0;
	uint32_t        reference = 0;
};

struct DepthStencilState
{
	bool                 depthTestEnable = true;
	bool                 depthWriteEnable = true;
	CompareOp      depthCompareOp = COMPARE_OP_LESS;
	bool                 depthBoundsTestEnable = false;
	float                minDepthBounds = 0.0f;
	float                maxDepthBounds = 1.0f;
	bool                 stencilTestEnable = false;
	StencilOpState front = {};
	StencilOpState back = {};
};

struct BlendAttachmentState
{
	bool                      blendEnable = false;
	BlendFactor         srcColorBlendFactor = BLEND_FACTOR_ONE;
	BlendFactor         dstColorBlendFactor = BLEND_FACTOR_ZERO;
	BlendOp             colorBlendOp = BLEND_OP_ADD;
	BlendFactor         srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	BlendFactor         dstAlphaBlendFactor = BLEND_FACTOR_ZERO;
	BlendOp             alphaBlendOp = BLEND_OP_ADD;
	ColorComponentFlags colorWriteMask = ColorComponentFlags::RGBA();

	// These are best guesses based on random formulas off of the internet.
	// Correct later when authorative literature is found.
	//
	static BlendAttachmentState BlendModeAdditive();
	static BlendAttachmentState BlendModeAlpha();
	static BlendAttachmentState BlendModeOver();
	static BlendAttachmentState BlendModeUnder();
	static BlendAttachmentState BlendModePremultAlpha();
};

struct ColorBlendState
{
	bool                       logicOpEnable = false;
	LogicOp              logicOp = LOGIC_OP_CLEAR;
	uint32_t                   blendAttachmentCount = 0;
	BlendAttachmentState blendAttachments[MAX_RENDER_TARGETS] = {};
	float                      blendConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct OutputState
{
	uint32_t     renderTargetCount = 0;
	Format renderTargetFormats[MAX_RENDER_TARGETS] = { FORMAT_UNDEFINED };
	Format depthStencilFormat = FORMAT_UNDEFINED;
};

struct GraphicsPipelineCreateInfo
{
	ShaderStageInfo          VS = {};
	ShaderStageInfo          HS = {};
	ShaderStageInfo          DS = {};
	ShaderStageInfo          GS = {};
	ShaderStageInfo          PS = {};
	VertexInputState         vertexInputState = {};
	InputAssemblyState       inputAssemblyState = {};
	TessellationState        tessellationState = {};
	RasterState              rasterState = {};
	MultisampleState         multisampleState = {};
	DepthStencilState        depthStencilState = {};
	ColorBlendState          colorBlendState = {};
	OutputState              outputState = {};
	ShadingRateMode          shadingRateMode = SHADING_RATE_NONE;
	MultiViewState           multiViewState = {};
	const PipelineInterface* pPipelineInterface = nullptr;
	bool                           dynamicRenderPass = false;
};

struct GraphicsPipelineCreateInfo2
{
	ShaderStageInfo          VS = {};
	ShaderStageInfo          PS = {};
	VertexInputState         vertexInputState = {};
	PrimitiveTopology        topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PolygonMode              polygonMode = POLYGON_MODE_FILL;
	CullMode                 cullMode = CULL_MODE_NONE;
	FrontFace                frontFace = FRONT_FACE_CCW;
	bool                           depthReadEnable = true;
	bool                           depthWriteEnable = true;
	CompareOp                depthCompareOp = COMPARE_OP_LESS;
	BlendMode                blendModes[MAX_RENDER_TARGETS] = { BLEND_MODE_NONE };
	OutputState              outputState = {};
	ShadingRateMode          shadingRateMode = SHADING_RATE_NONE;
	MultiViewState           multiViewState = {};
	const PipelineInterface* pPipelineInterface = nullptr;
	bool                           dynamicRenderPass = false;
};

namespace internal {

	void FillOutGraphicsPipelineCreateInfo(
		const GraphicsPipelineCreateInfo2& pSrcCreateInfo,
		GraphicsPipelineCreateInfo* pDstCreateInfo);

} // namespace internal

class GraphicsPipeline final: public DeviceObject<GraphicsPipelineCreateInfo>
{
	friend class RenderDevice;
public:
	VkPipelinePtr GetVkPipeline() const { return mPipeline; }

private:
	Result create(const GraphicsPipelineCreateInfo& pCreateInfo) final;
	Result createApiObjects(const GraphicsPipelineCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	Result initializeShaderStages(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
		VkGraphicsPipelineCreateInfo& vkCreateInfo);
	Result initializeVertexInput(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		std::vector<VkVertexInputAttributeDescription>& attribues,
		std::vector<VkVertexInputBindingDescription>& bindings,
		VkPipelineVertexInputStateCreateInfo& stateCreateInfo);
	Result initializeInputAssembly(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineInputAssemblyStateCreateInfo& stateCreateInfo);
	Result initializeTessellation(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineTessellationDomainOriginStateCreateInfoKHR& domainOriginStateCreateInfo,
		VkPipelineTessellationStateCreateInfo& stateCreateInfo);
	Result initializeViewports(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineViewportStateCreateInfo& stateCreateInfo);
	Result initializeRasterization(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthClipStateCreateInfo,
		VkPipelineRasterizationStateCreateInfo& stateCreateInfo);
	Result initializeMultisample(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineMultisampleStateCreateInfo& stateCreateInfo);
	Result initializeDepthStencil(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		VkPipelineDepthStencilStateCreateInfo& stateCreateInfo);
	Result initializeColorBlend(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		std::vector<VkPipelineColorBlendAttachmentState>& attachments,
		VkPipelineColorBlendStateCreateInfo& stateCreateInfo);
	Result initializeDynamicState(
		const GraphicsPipelineCreateInfo& pCreateInfo,
		std::vector<VkDynamicState>& dynamicStates,
		VkPipelineDynamicStateCreateInfo& stateCreateInfo);

	VkPipelinePtr mPipeline;
};

struct PipelineInterfaceCreateInfo
{
	uint32_t setCount = 0;
	struct
	{
		uint32_t                         set = VALUE_IGNORED; // Set number
		const DescriptorSetLayout* pLayout = nullptr;           // Set layout
	} sets[MAX_BOUND_DESCRIPTOR_SETS] = {};

	// VK: Push constants
	// DX: Root constants
	//
	// Push/root constants are measured in DWORDs (uint32_t) aka 32-bit values.
	//
	// The binding and set for push constants CAN NOT overlap with a binding
	// AND set in sets (the struct immediately above this one). It's okay for
	// push constants to be in an existing set at binding that is not used
	// by an entry in the set layout.
	//
	struct
	{
		uint32_t              count = 0;                 // Measured in DWORDs, must be less than or equal to MAX_PUSH_CONSTANTS
		uint32_t              binding = VALUE_IGNORED; // D3D12 only, ignored by Vulkan
		uint32_t              set = VALUE_IGNORED; // D3D12 only, ignored by Vulkan
		ShaderStageBits shaderVisiblity = SHADER_STAGE_ALL;
	} pushConstants;
};

class PipelineInterface final : public DeviceObject<PipelineInterfaceCreateInfo>
{
	friend class RenderDevice;
public:
	bool                         HasConsecutiveSetNumbers() const { return mHasConsecutiveSetNumbers; }
	const std::vector<uint32_t>& GetSetNumbers() const { return mSetNumbers; }

	const DescriptorSetLayout* GetSetLayout(uint32_t setNumber) const;

	VkPipelineLayoutPtr GetVkPipelineLayout() const { return mPipelineLayout; }

	VkShaderStageFlags GetPushConstantShaderStageFlags() const { return mPushConstantShaderStageFlags; }

private:
	Result create(const PipelineInterfaceCreateInfo& pCreateInfo) final;
	Result createApiObjects(const PipelineInterfaceCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkPipelineLayoutPtr mPipelineLayout;
	VkShaderStageFlags  mPushConstantShaderStageFlags = 0;
	bool                  mHasConsecutiveSetNumbers = false;
	std::vector<uint32_t> mSetNumbers = {};
};

#pragma endregion

#pragma region Command

struct BufferToBufferCopyInfo
{
	uint64_t size = 0;

	struct srcBuffer
	{
		uint64_t offset = 0;
	} srcBuffer;

	struct
	{
		uint32_t offset = 0;
	} dstBuffer;
};

struct BufferToImageCopyInfo
{
	struct
	{
		uint32_t imageWidth = 0; // [pixels]
		uint32_t imageHeight = 0; // [pixels]
		uint32_t imageRowStride = 0; // [bytes]
		uint64_t footprintOffset = 0; // [bytes]
		uint32_t footprintWidth = 0; // [pixels]
		uint32_t footprintHeight = 0; // [pixels]
		uint32_t footprintDepth = 0; // [pixels]
	} srcBuffer;

	struct
	{
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0; // Must be 0 for 3D images
		uint32_t arrayLayerCount = 0; // Must be 1 for 3D images
		uint32_t x = 0; // [pixels]
		uint32_t y = 0; // [pixels]
		uint32_t z = 0; // [pixels]
		uint32_t width = 0; // [pixels]
		uint32_t height = 0; // [pixels]
		uint32_t depth = 0; // [pixels]
	} dstImage;
};

struct ImageToBufferCopyInfo
{
	struct
	{
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0; // Must be 0 for 3D images
		uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} offset;
	} srcImage;

	struct
	{
		uint32_t x = 0; // [pixels]
		uint32_t y = 0; // [pixels]
		uint32_t z = 0; // [pixels]
	} extent;
};

struct ImageToBufferOutputPitch
{
	uint32_t rowPitch = 0;
};

struct ImageToImageCopyInfo
{
	struct
	{
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0; // Must be 0 for 3D images
		uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} offset;
	} srcImage;

	struct
	{
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0; // Must be 0 for 3D images
		uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} offset;
	} dstImage;

	struct
	{
		uint32_t x = 0; // [pixels]
		uint32_t y = 0; // [pixels]
		uint32_t z = 0; // [pixels]
	} extent;
};

struct ImageBlitInfo
{
	struct ImgInfo
	{
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0; // Must be 0 for 3D images
		uint32_t arrayLayerCount = 1; // Must be 1 for 3D images
		struct
		{
			uint32_t x = 0; // [pixels]
			uint32_t y = 0; // [pixels]
			uint32_t z = 0; // [pixels]
		} offsets[2];
	};

	ImgInfo srcImage;
	ImgInfo dstImage;

	Filter filter = FILTER_LINEAR;
};

struct RenderPassBeginInfo
{
	//
	// The value of RTVClearCount cannot be less than the number
	// of RTVs in pRenderPass.
	//
	const RenderPass* pRenderPass = nullptr;
	Rect                   renderArea = {};
	uint32_t                     RTVClearCount = 0;
	RenderTargetClearValue RTVClearValues[MAX_RENDER_TARGETS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };
};

// RenderingInfo is used to start dynamic render passes.
struct RenderingInfo
{
	BeginRenderingFlags flags;
	Rect                renderArea = {};

	uint32_t                renderTargetCount = 0;
	RenderTargetView* pRenderTargetViews[MAX_RENDER_TARGETS] = {};
	DepthStencilView* pDepthStencilView = nullptr;

	RenderTargetClearValue RTVClearValues[MAX_RENDER_TARGETS] = { 0.0f, 0.0f, 0.0f, 0.0f };
	DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };
};

struct CommandPoolCreateInfo
{
	const Queue* pQueue = nullptr;
};

class CommandPool final : public DeviceObject<CommandPoolCreateInfo>
{
public:
	CommandType GetCommandType() const;
	VkCommandPoolPtr GetVkCommandPool() const { return mCommandPool; }
	
private:
	Result createApiObjects(const CommandPoolCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	VkCommandPoolPtr mCommandPool;
};

namespace internal {

	//! @struct CommandBufferCreateInfo
	//!
	//! For D3D12 every command buffer will have two GPU visible descriptor heaps:
	//!   - one for CBVSRVUAV descriptors
	//!   - one for Sampler descriptors
	//!
	//! Both of heaps are set when the command buffer begins.
	//!
	//! Each time that BindGraphicsDescriptorSets or BindComputeDescriptorSets
	//! is called, the contents of each descriptor set's CBVSRVAUV and Sampler heaps
	//! will be copied into the command buffer's respective heap.
	//!
	//! The offsets from used in the copies will be saved and used to set the
	//! root descriptor tables.
	//!
	//! 'resourceDescriptorCount' and 'samplerDescriptorCount' tells the
	//! D3D12 command buffer how large CBVSRVUAV and Sampler heaps should be.
	//!
	//! 'samplerDescriptorCount' cannot exceed MAX_SAMPLER_DESCRIPTORS.
	//!
	//! Vulkan does not use 'samplerDescriptorCount' or 'samplerDescriptorCount'.
	//!
	struct CommandBufferCreateInfo
	{
		const CommandPool* pPool = nullptr;
		uint32_t                 resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT;
		uint32_t                 samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT;
	};

} // namespace internal

class CommandBuffer final : public DeviceObject<internal::CommandBufferCreateInfo>
{
public:
	CommandType GetCommandType() const { return m_createInfo.pPool->GetCommandType(); }
	VkCommandBufferPtr GetVkCommandBuffer() const { return mCommandBuffer; }

	Result Begin();
	Result End();

	void BeginRenderPass(const RenderPassBeginInfo* pBeginInfo);
	void EndRenderPass();

	void BeginRendering(const RenderingInfo* pRenderingInfo);
	void EndRendering();

	const RenderPass* GetCurrentRenderPass() const { return mCurrentRenderPass; }

	// Clear functions must be called between BeginRenderPass and EndRenderPass.
	// Arg for pImage must be an image in the current render pass.
	// TODO: add support for calling inside a dynamic render pass (i.e., BeginRendering and EndRendering).
	void ClearRenderTarget(
		Image* pImage,
		const RenderTargetClearValue& clearValue);
	void ClearDepthStencil(
		Image* pImage,
		const DepthStencilClearValue& clearValue,
		uint32_t                            clearFlags);

	//! @fn TransitionImageLayout
	//! Vulkan requires a queue ownership transfer if a resource
	//! is used by queues in different queue families:
	//!  - Use \b pSrcQueue to specify a queue in the source queue family
	//!  - Use \b pDstQueue to specify a queue in the destination queue family
	//!  - If \b pSrcQueue and \b pDstQueue belong to the same queue family then the queue ownership transfer won't happen.
	//! D3D12 ignores both \b pSrcQueue and \b pDstQueue since they're not relevant.
	void TransitionImageLayout(
		const Image* pImage,
		uint32_t            mipLevel,
		uint32_t            mipLevelCount,
		uint32_t            arrayLayer,
		uint32_t            arrayLayerCount,
		ResourceState beforeState,
		ResourceState afterState,
		const Queue* pSrcQueue = nullptr,
		const Queue* pDstQueue = nullptr);

	void BufferResourceBarrier(
		const Buffer* pBuffer,
		ResourceState beforeState,
		ResourceState afterState,
		const Queue* pSrcQueue = nullptr,
		const Queue* pDstQueue = nullptr);

	void SetViewports(
		uint32_t              viewportCount,
		const Viewport* pViewports);

	void SetScissors(
		uint32_t          scissorCount,
		const Rect* pScissors);

	void BindGraphicsDescriptorSets(
		const PipelineInterface* pInterface,
		uint32_t                          setCount,
		const DescriptorSet* const* ppSets);

	//
	// Parameters count and dstOffset are measured in DWORDs (uint32_t) aka 32-bit values.
	// To set the first 4 32-bit values, use count = 4, dstOffset = 0.
	// To set the 16 DWORDs starting at offset 8, use count=16, dstOffset = 8.
	//
	// VK: pValues is subjected to Vulkan packing rules. BigWheels compiles HLSL
	//     shaders with -fvk-use-dx-layout on. This makes the packing rules match
	//     that of D3D12. However, if a shader is compiled without that flag or
	//     with a different compiler or source language. The contents pointed to
	//     by pValues must respect the packing rules in effect.
	//
	void PushGraphicsConstants(
		const PipelineInterface* pInterface,
		uint32_t                       count,
		const void* pValues,
		uint32_t                       dstOffset = 0);

	void PushGraphicsUniformBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushGraphicsStructuredBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushGraphicsStorageBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushGraphicsSampledImage(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const SampledImageView* pView);

	void PushGraphicsStorageImage(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const StorageImageView* pView);

	void PushGraphicsSampler(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const Sampler* pSampler);

	void BindGraphicsPipeline(const GraphicsPipeline* pPipeline);

	void BindComputeDescriptorSets(
		const PipelineInterface* pInterface,
		uint32_t                          setCount,
		const DescriptorSet* const* ppSets);

	void PushComputeConstants(
		const PipelineInterface* pInterface,
		uint32_t                       count,
		const void* pValues,
		uint32_t                       dstOffset = 0);

	void PushComputeUniformBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushComputeStructuredBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushComputeStorageBuffer(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer);

	void PushComputeSampledImage(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const SampledImageView* pView);

	void PushComputeStorageImage(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const StorageImageView* pView);

	void PushComputeSampler(
		const PipelineInterface* pInterface,
		uint32_t                       binding,
		uint32_t                       set,
		const Sampler* pSampler);

	void BindComputePipeline(const ComputePipeline* pPipeline);

	void BindIndexBuffer(const IndexBufferView* pView);

	void BindVertexBuffers(
		uint32_t                      viewCount,
		const VertexBufferView* pViews);

	void Draw(
		uint32_t vertexCount,
		uint32_t instanceCount = 1,
		uint32_t firstVertex = 0,
		uint32_t firstInstance = 0);

	void DrawIndexed(
		uint32_t indexCount,
		uint32_t instanceCount = 1,
		uint32_t firstIndex = 0,
		int32_t  vertexOffset = 0,
		uint32_t firstInstance = 0);

	void Dispatch(
		uint32_t groupCountX,
		uint32_t groupCountY,
		uint32_t groupCountZ);

	void CopyBufferToBuffer(
		const BufferToBufferCopyInfo* pCopyInfo,
		Buffer* pSrcBuffer,
		Buffer* pDstBuffer);

	void CopyBufferToImage(
		const std::vector<BufferToImageCopyInfo>& pCopyInfos,
		Buffer* pSrcBuffer,
		Image* pDstImage);

	void CopyBufferToImage(
		const BufferToImageCopyInfo* pCopyInfo,
		Buffer* pSrcBuffer,
		Image* pDstImage);

	//! @brief Copies an image to a buffer.
	//! @param pCopyInfo The specifications of the image region to copy.
	//! @param pSrcImage The source image.
	//! @param pDstBuffer The destination buffer.
	//! @return The image row pitch as written to the destination buffer.
	ImageToBufferOutputPitch CopyImageToBuffer(
		const ImageToBufferCopyInfo* pCopyInfo,
		Image* pSrcImage,
		Buffer* pDstBuffer);

	void CopyImageToImage(
		const ImageToImageCopyInfo* pCopyInfo,
		Image* pSrcImage,
		Image* pDstImage);

	void BlitImage(
		const ImageBlitInfo* pCopyInfo,
		Image* pSrcImage,
		Image* pDstImage);

	void BeginQuery(
		const Query* pQuery,
		uint32_t           queryIndex);

	void EndQuery(
		const Query* pQuery,
		uint32_t           queryIndex);

	void WriteTimestamp(
		const Query* pQuery,
		PipelineStage pipelineStage,
		uint32_t            queryIndex);

	void ResolveQueryData(
		Query* pQuery,
		uint32_t     startIndex,
		uint32_t     numQueries);

	void BeginRenderPass(const RenderPass* pRenderPass);

	void BeginRenderPass(
		const DrawPass* pDrawPass,
		const DrawPassClearFlags& clearFlags = DRAW_PASS_CLEAR_FLAG_CLEAR_ALL);

	void TransitionImageLayout(
		const Texture* pTexture,
		uint32_t             mipLevel,
		uint32_t             mipLevelCount,
		uint32_t             arrayLayer,
		uint32_t             arrayLayerCount,
		ResourceState  beforeState,
		ResourceState  afterState,
		const Queue* pSrcQueue = nullptr,
		const Queue* pDstQueue = nullptr);

	void TransitionImageLayout(
		RenderPass* pRenderPass,
		ResourceState renderTargetBeforeState,
		ResourceState renderTargetAfterState,
		ResourceState depthStencilTargetBeforeState,
		ResourceState depthStencilTargetAfterState);

	void TransitionImageLayout(
		DrawPass* pDrawPass,
		ResourceState renderTargetBeforeState,
		ResourceState renderTargetAfterState,
		ResourceState depthStencilTargetBeforeState,
		ResourceState depthStencilTargetAfterState);

	void SetViewports(const Viewport& viewport);

	void SetScissors(const Rect& scissor);

	void BindIndexBuffer(const Buffer* pBuffer, IndexType indexType, uint64_t offset = 0);

	void BindIndexBuffer(const Mesh* pMesh, uint64_t offset = 0);

	void BindVertexBuffers(
		uint32_t                   bufferCount,
		const Buffer* const* buffers,
		const uint32_t* pStrides,
		const uint64_t* pOffsets = nullptr);

	void BindVertexBuffers(const Mesh* pMesh, const uint64_t* pOffsets = nullptr);

	//
	// NOTE: If you're running into an issue where VS2019 is incorrectly
	//       resolving call to this function to the Draw(vertexCount, ...)
	//       function above - it might be that the last parameter isn't
	//       explicitly the double pointer type. Possibly due to some casting.
	//
	//       For example this might give VS2019 some grief:
	//          DescriptorSetPtr set;
	//          Draw(quad, 1, set);
	//
	//       Use this instead:
	//          DescriptorSetPtr set;
	//          Draw(quad, 1, &set);
	//
	void Draw(const FullscreenQuad* pQuad, uint32_t setCount, const DescriptorSet* const* ppSets);

private:
	Result createApiObjects(const internal::CommandBufferCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

	struct DynamicRenderPassInfo
	{
		Rect                             mRenderArea = {};
		std::vector<RenderTargetViewPtr> mRenderTargetViews = {};
		DepthStencilViewPtr              mDepthStencilView = nullptr;
	};

	// Returns true when inside a render pass (dynamic or regular)
	bool HasActiveRenderPass() const;

	bool                  mDynamicRenderPassActive = false;
	DynamicRenderPassInfo mDynamicRenderPassInfo = {};

	void BeginRenderPassImpl(const RenderPassBeginInfo* pBeginInfo);
	void EndRenderPassImpl();

	void BeginRenderingImpl(const RenderingInfo* pRenderingInfo);
	void EndRenderingImpl();

	void PushDescriptorImpl(
		CommandType              pipelineBindPoint,
		const PipelineInterface* pInterface,
		DescriptorType           descriptorType,
		uint32_t                       binding,
		uint32_t                       set,
		uint32_t                       bufferOffset,
		const Buffer* pBuffer,
		const SampledImageView* pSampledImageView,
		const StorageImageView* pStorageImageView,
		const Sampler* pSampler);

	void BindDescriptorSets(
		VkPipelineBindPoint               bindPoint,
		const PipelineInterface* pInterface,
		uint32_t                          setCount,
		const DescriptorSet* const* ppSets);

	void PushConstants(
		const PipelineInterface* pInterface,
		uint32_t                       count,
		const void* pValues,
		uint32_t                       dstOffset);

	const RenderPass* mCurrentRenderPass = nullptr;
	VkCommandBufferPtr mCommandBuffer;
};

#pragma endregion

#pragma region Queue

struct SubmitInfo
{
	uint32_t                          commandBufferCount = 0;
	const CommandBuffer* const* ppCommandBuffers = nullptr;
	uint32_t                          waitSemaphoreCount = 0;
	const Semaphore* const* ppWaitSemaphores = nullptr;
	std::vector<uint64_t>             waitValues = {}; // Use 0 if index is binary semaphore
	uint32_t                          signalSemaphoreCount = 0;
	Semaphore** ppSignalSemaphores = nullptr;
	std::vector<uint64_t>             signalValues = {}; // Use 0 if index is binary smeaphore
	Fence* pFence = nullptr;
};

namespace internal
{
	struct QueueCreateInfo
	{
		CommandType commandType = COMMAND_TYPE_UNDEFINED;
		uint32_t          queueFamilyIndex = VALUE_IGNORED; // Vulkan
		uint32_t          queueIndex = VALUE_IGNORED; // Vulkan
		void* pApiObject = nullptr;           // D3D12
	};
}

class Queue final : public DeviceObject<internal::QueueCreateInfo>
{
public:
	CommandType GetCommandType() const { return m_createInfo.commandType; }
	VkQueuePtr GetVkQueue() const { return mQueue; }
	uint32_t GetQueueFamilyIndex() const { return m_createInfo.queueFamilyIndex; }

	Result WaitIdle();

	Result Submit(const SubmitInfo* pSubmitInfo);

	// Timeline semaphore functions
	Result QueueWait(Semaphore* pSemaphore, uint64_t value);
	Result QueueSignal(Semaphore* pSemaphore, uint64_t value);

	// GPU timestamp frequency counter in ticks per second
	Result GetTimestampFrequency(uint64_t* pFrequency) const;

	Result CreateCommandBuffer(
		CommandBuffer** ppCommandBuffer,
		uint32_t              resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT,
		uint32_t              samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT);
	void DestroyCommandBuffer(const CommandBuffer* pCommandBuffer);

	// In place copy of buffer to buffer
	Result CopyBufferToBuffer(
		const BufferToBufferCopyInfo* pCopyInfo,
		Buffer* pSrcBuffer,
		Buffer* pDstBuffer,
		ResourceState                 stateBefore,
		ResourceState                 stateAfter);

	// In place copy of buffer to image
	Result CopyBufferToImage(
		const std::vector<BufferToImageCopyInfo>& pCopyInfos,
		Buffer* pSrcBuffer,
		Image* pDstImage,
		uint32_t                                        mipLevel,
		uint32_t                                        mipLevelCount,
		uint32_t                                        arrayLayer,
		uint32_t                                        arrayLayerCount,
		ResourceState                             stateBefore,
		ResourceState                             stateAfter);

	VkResult TransitionImageLayout(
		VkImage              image,
		VkImageAspectFlags   aspectMask,
		uint32_t             baseMipLevel,
		uint32_t             levelCount,
		uint32_t             baseArrayLayer,
		uint32_t             layerCount,
		VkImageLayout        oldLayout,
		VkImageLayout        newLayout,
		VkPipelineStageFlags newPipelineStage);

private:
	Result createApiObjects(const internal::QueueCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;


	struct CommandSet
	{
		CommandPoolPtr   commandPool;
		CommandBufferPtr commandBuffer;
	};
	std::vector<CommandSet> mCommandSets;
	std::mutex              mCommandSetMutex;

	VkQueuePtr       mQueue;
	VkCommandPoolPtr mTransientPool;
	std::mutex       mQueueMutex;
	std::mutex       mCommandPoolMutex;
};

#pragma endregion

#pragma region FullscreenQuad

/*

// Shaders for use with FullscreenQuad helper class should look something like the example shader below.
// Reference:
//   https://www.slideshare.net/DevCentralAMD/vertex-shader-tricks-bill-bilodeau

Texture2D    Tex0     : register(t0);
SamplerState Sampler0 : register(s1);

struct VSOutput
{
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(uint id : SV_VertexID)
{
	VSOutput result;

	// Clip space position
	result.Position.x = (float)(id / 2) * 4.0 - 1.0;
	result.Position.y = (float)(id % 2) * 4.0 - 1.0;
	result.Position.z = 0.0;
	result.Position.w = 1.0;

	// Texture coordinates
	result.TexCoord.x = (float)(id / 2) * 2.0;
	result.TexCoord.y = 1.0 - (float)(id % 2) * 2.0;

	return result;
}

float4 psmain(VSOutput input) : SV_TARGET
{
	return Tex0.Sample(Sampler0, input.TexCoord);
}
*/

struct FullscreenQuadCreateInfo
{
	ShaderModule* VS = nullptr;
	ShaderModule* PS = nullptr;

	uint32_t setCount = 0;
	struct
	{
		uint32_t                   set = VALUE_IGNORED;
		DescriptorSetLayout* pLayout;
	} sets[MAX_BOUND_DESCRIPTOR_SETS] = {};

	uint32_t     renderTargetCount = 0;
	Format renderTargetFormats[MAX_RENDER_TARGETS] = { FORMAT_UNDEFINED };
	Format depthStencilFormat = FORMAT_UNDEFINED;
};

class FullscreenQuad : public DeviceObject<FullscreenQuadCreateInfo>
{
public:
	FullscreenQuad() {}
	virtual ~FullscreenQuad() {}

	PipelineInterfacePtr GetPipelineInterface() const { return mPipelineInterface; }
	GraphicsPipelinePtr  GetPipeline() const { return mPipeline; }

protected:
	Result createApiObjects(const FullscreenQuadCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

private:
	PipelineInterfacePtr mPipelineInterface;
	GraphicsPipelinePtr  mPipeline;
};

#pragma endregion

#pragma region Scope

class ScopeDestroyer
{
public:
	ScopeDestroyer(RenderDevice* pDevice);
	~ScopeDestroyer();

	Result AddObject(Image* pObject);
	Result AddObject(Buffer* pObject);
	Result AddObject(Mesh* pObject);
	Result AddObject(Texture* pObject);
	Result AddObject(Sampler* pObject);
	Result AddObject(SampledImageView* pObject);
	Result AddObject(Queue* pParent, CommandBuffer* pObject);

	// Releases all objects without destroying them
	void ReleaseAll();

private:
	RenderDevice* mDevice;
	std::vector<ImagePtr>                              mImages;
	std::vector<BufferPtr>                             mBuffers;
	std::vector<MeshPtr>                               mMeshes;
	std::vector<TexturePtr>                            mTextures;
	std::vector<SamplerPtr>                            mSamplers;
	std::vector<SampledImageViewPtr>                   mSampledImageViews;
	std::vector<std::pair<QueuePtr, CommandBufferPtr>> mTransientCommandBuffers;
};

#pragma endregion

#pragma region Text Draw

struct TextureFontUVRect
{
	float u0 = 0;
	float v0 = 0;
	float u1 = 0;
	float v1 = 0;
};

struct TextureFontGlyphMetrics
{
	uint32_t          codepoint = 0;
	GlyphMetrics      glyphMetrics = {};
	float2            size = {};
	TextureFontUVRect uvRect = {};
};

struct TextureFontCreateInfo
{
	Font   font;
	float       size = 16.0f;
	std::string characters = ""; // Default characters if empty
};

class TextureFont : public DeviceObject<TextureFontCreateInfo>
{
public:
	TextureFont() {}
	virtual ~TextureFont() {}

	static std::string GetDefaultCharacters();

	const Font& GetFont() const { return m_createInfo.font; }
	float            GetSize() const { return m_createInfo.size; }
	std::string      GetCharacters() const { return m_createInfo.characters; }
	TexturePtr GetTexture() const { return mTexture; }

	float                                GetAscent() const { return mFontMetrics.ascent; }
	float                                GetDescent() const { return mFontMetrics.descent; }
	float                                GetLineGap() const { return mFontMetrics.lineGap; }
	const TextureFontGlyphMetrics* GetGlyphMetrics(uint32_t codepoint) const;

protected:
	Result createApiObjects(const TextureFontCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

private:
	FontMetrics                                mFontMetrics;
	std::vector<TextureFontGlyphMetrics> mGlyphMetrics;
	TexturePtr                           mTexture;
};

// -------------------------------------------------------------------------------------------------

struct TextDrawCreateInfo
{
	TextureFont* pFont = nullptr;
	uint32_t              maxTextLength = 4096;
	ShaderStageInfo VS = {}; // Use basic/shaders/TextDraw.hlsl (vsmain) for now
	ShaderStageInfo PS = {}; // Use basic/shaders/TextDraw.hlsl (psmain) for now
	BlendMode       blendMode = BLEND_MODE_PREMULT_ALPHA;
	Format          renderTargetFormat = FORMAT_UNDEFINED;
	Format          depthStencilFormat = FORMAT_UNDEFINED;
};

class TextDraw
	: public DeviceObject<TextDrawCreateInfo>
{
public:
	TextDraw() {}
	virtual ~TextDraw() {}

	void Clear();

	void AddString(
		const float2& position,
		const std::string& string,
		float              tabSpacing,  // Tab size, 0.5f = 0.5x space, 1.0 = 1x space, 2.0 = 2x space, etc
		float              lineSpacing, // Line spacing (ascent - descent + line gap), 0.5f = 0.5x line space, 1.0 = 1x line space, 2.0 = 2x line space, etc
		const float3& color,
		float              opacity);

	void AddString(
		const float2& position,
		const std::string& string,
		const float3& color = float3(1, 1, 1),
		float              opacity = 1.0f);

	// Use this if text is static
	Result UploadToGpu(Queue* pQueue);

	// Use this if text is dynamic
	void UploadToGpu(CommandBuffer* pCommandBuffer);

	void PrepareDraw(const float4x4& MVP, CommandBuffer* pCommandBuffer);
	void Draw(CommandBuffer* pCommandBuffer);

protected:
	Result createApiObjects(const TextDrawCreateInfo& pCreateInfo) final;
	void   destroyApiObjects() final;

private:
	uint32_t                     mTextLength = 0;
	BufferPtr              mCpuIndexBuffer;
	BufferPtr              mCpuVertexBuffer;
	BufferPtr              mGpuIndexBuffer;
	BufferPtr              mGpuVertexBuffer;
	IndexBufferView        mIndexBufferView = {};
	VertexBufferView       mVertexBufferView = {};
	BufferPtr              mCpuConstantBuffer;
	BufferPtr              mGpuConstantBuffer;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	DescriptorSetPtr       mDescriptorSet;
	PipelineInterfacePtr   mPipelineInterface;
	GraphicsPipelinePtr    mPipeline;
};

#pragma endregion


//===================================================================
// OLD
//===================================================================

#pragma region VulkanFence

class VulkanFence final
{
	friend class RenderDevice;
public:
	VulkanFence(RenderDevice& device, const FenceCreateInfo& createInfo);
	~VulkanFence();

	bool IsValid() const { return m_fence != nullptr; }

	bool Wait(uint64_t timeout = UINT64_MAX);
	bool Reset();
	bool WaitAndReset(uint64_t timeout = UINT64_MAX);

	[[nodiscard]] VkResult Status() const;

	VkFence Get() { return m_fence; }

private:
	RenderDevice& m_device;
	VkFence m_fence{ nullptr };
};
using VulkanFencePtr = std::shared_ptr<VulkanFence>;

#pragma endregion

#pragma region VulkanSemaphore

class VulkanSemaphore final
{
	friend class RenderDevice;
public:
	VulkanSemaphore(RenderDevice& device, const SemaphoreCreateInfo& createInfo);
	~VulkanSemaphore();

	bool IsValid() const { return m_semaphore != nullptr; }

	VkSemaphore Get() { return m_semaphore; }
	SemaphoreType GetSemaphoreType() const { return m_semaphoreType; }
	bool IsBinary() const { return m_semaphoreType == SEMAPHORE_TYPE_BINARY; }
	bool IsTimeline() const { return m_semaphoreType == SEMAPHORE_TYPE_TIMELINE; }
	uint64_t GetCounterValue() const; // Returns current timeline semaphore value

	bool Wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
	bool Signal(uint64_t value) const;

private:
	bool timelineWait(uint64_t value, uint64_t timeout) const;
	bool timelineSignal(uint64_t value) const;
	uint64_t timelineCounterValue() const;

	RenderDevice&    m_device;
	mutable uint64_t m_timelineSignaledValue = 0;
	VkSemaphore      m_semaphore{ nullptr };
	SemaphoreType    m_semaphoreType{ SEMAPHORE_TYPE_BINARY };
};
using VulkanSemaphorePtr = std::shared_ptr<VulkanSemaphore>;

#pragma endregion

#pragma region VulkanImage

class VulkanImage final
{
	friend class RenderDevice;
public:
	VulkanImage(RenderDevice& device, const ImageCreateInfo& createInfo);
	~VulkanImage();

	// Convenience functions
	ImageViewType GuessImageViewType(bool isCube = false) const;

	bool MapMemory(uint64_t offset, void** ppMappedAddress);
	void UnmapMemory();

	bool IsValid() const { return m_image != nullptr; }

	VkImage                       Get() { return m_image; }
	VkFormat                      GetVkFormat() const { return m_vkFormat; }
	VkImageAspectFlags            GetVkImageAspectFlags() const { return m_imageAspect; }

	ImageType                     GetType() const { return m_type; }
	uint32_t                      GetWidth() const { return m_width; }
	uint32_t                      GetHeight() const { return m_height; }
	uint32_t                      GetDepth() const { return m_depth; }
	Format                        GetFormat() const { return m_format; }
	SampleCount                   GetSampleCount() const { return m_sampleCount; }
	uint32_t                      GetMipLevelCount() const { return m_mipLevelCount; }
	uint32_t                      GetArrayLayerCount() const { return m_arrayLayerCount; }
	const ImageUsageFlags&        GetUsageFlags() const { return m_usageFlags; }
	MemoryUsage                   GetMemoryUsage() const { return m_memoryUsage; }
	ResourceState                 GetInitialState() const { return m_initialState; }
	const RenderTargetClearValue& GetRTVClearValue() const { return m_RTVClearValue; }
	const DepthStencilClearValue& GetDSVClearValue() const { return m_DSVClearValue; }
	bool                          GetConcurrentMultiQueueUsageEnabled() const { return m_concurrentMultiQueueUsage; }
	ImageCreateFlags              GetCreateFlags() const { return m_createFlags; }

private:
	RenderDevice&          m_device;

	ImageType              m_type = IMAGE_TYPE_2D;
	uint32_t               m_width = 0;
	uint32_t               m_height = 0;
	uint32_t               m_depth = 0;
	Format                 m_format = FORMAT_UNDEFINED;
	SampleCount            m_sampleCount = SAMPLE_COUNT_1;
	uint32_t               m_mipLevelCount = 1;
	uint32_t               m_arrayLayerCount = 1;
	ImageUsageFlags        m_usageFlags = ImageUsageFlags::SampledImage();
	MemoryUsage            m_memoryUsage = MEMORY_USAGE_GPU_ONLY;   // D3D12 will fail on any other memory usage
	ResourceState          m_initialState = RESOURCE_STATE_GENERAL; // This may not be the best choice
	RenderTargetClearValue m_RTVClearValue = { 0, 0, 0, 0 };        // Optimized RTV clear value
	DepthStencilClearValue m_DSVClearValue = { 1.0f, 0xFF };        // Optimized DSV clear value
	void*                  m_pApiObject = nullptr;                  // [OPTIONAL] For external images such as swapchain images
	bool                   m_concurrentMultiQueueUsage = false;
	ImageCreateFlags       m_createFlags = {};

	VkImage                m_image{ nullptr };
	VmaAllocation          m_allocation{ nullptr };
	VmaAllocationInfo      m_allocationInfo = {};
	VkFormat               m_vkFormat = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags     m_imageAspect = InvalidValue<VkImageAspectFlags>();
};
using VulkanImagePtr = std::shared_ptr<VulkanImage>;

#pragma endregion

#pragma region VulkanBuffer

class VulkanBuffer final
{

};
using VulkanBufferPtr = std::shared_ptr<VulkanBuffer>;

#pragma endregion

#pragma region VulkanCommandPool

class VulkanCommandPool final
{
public:
	CommandType GetCommandType() const;
};
using VulkanCommandPoolPtr = std::shared_ptr<VulkanCommandPool>;

#pragma endregion

#pragma region VulkanCommandBuffer

struct CommandBufferCreateInfo final
{
	VulkanCommandPoolPtr pool = nullptr;
	uint32_t resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT;
	uint32_t samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT; // TODO: не используется - удалить
};

class VulkanCommandBuffer final
{
public:
	
};
using VulkanCommandBufferPtr = std::shared_ptr<VulkanCommandBuffer>;

#pragma endregion

