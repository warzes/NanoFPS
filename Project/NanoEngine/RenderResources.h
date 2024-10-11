#pragma once

#include "RenderCore.h"

namespace vkr {

//=============================================================================
#pragma region [ Fence ]

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

//=============================================================================
#pragma region [ Semaphore ]

struct SemaphoreCreateInfo final
{
	SemaphoreType semaphoreType = SemaphoreType::Binary;
	uint64_t      initialValue = 0; // Timeline semaphore only
};

class Semaphore final : public DeviceObject<SemaphoreCreateInfo>
{
	friend class RenderDevice;
public:
	VkSemaphorePtr GetVkSemaphore() const { return m_semaphore; }
	SemaphoreType  GetSemaphoreType() const { return m_createInfo.semaphoreType; }
	bool           IsBinary() const { return m_createInfo.semaphoreType == SemaphoreType::Binary; }
	bool           IsTimeline() const { return m_createInfo.semaphoreType == SemaphoreType::Timeline; }

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

//=============================================================================
#pragma region [ Query ]

struct PipelineStatistics final
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

struct QueryCreateInfo final
{
	QueryType type = QueryType::Undefined;
	uint32_t count = 0;
};

class Query final : public DeviceObject<QueryCreateInfo>
{
	friend class RenderDevice;
public:
	QueryType GetType() const { return m_createInfo.type; }
	uint32_t GetCount() const { return m_createInfo.count; }

	VkQueryPoolPtr GetVkQueryPool() const { return m_queryPool; }
	uint32_t GetQueryTypeSize() const { return getQueryTypeSize(m_type, m_multiplier); }
	VkBufferPtr GetReadBackBuffer() const;

	void Reset(uint32_t firstQuery, uint32_t queryCount);
	Result GetData(void* dstData, uint64_t dstDataSize);

private:
	Result createApiObjects(const QueryCreateInfo& createInfo) final;
	void destroyApiObjects() final;
	uint32_t getQueryTypeSize(VkQueryType type, uint32_t multiplier) const;
	VkQueryType getQueryType() const { return m_type; }

	VkQueryPoolPtr  m_queryPool;
	VkQueryType     m_type = VK_QUERY_TYPE_MAX_ENUM;
	BufferPtr       m_buffer;
	uint32_t        m_multiplier = 1;
};

#pragma endregion

//=============================================================================
#pragma region [ Buffer ]

struct BufferCreateInfo final
{
	uint64_t         size = 0;
	uint32_t         structuredElementStride = 0; // HLSL StructuredBuffer<> only
	BufferUsageFlags usageFlags = 0;
	MemoryUsage      memoryUsage = MemoryUsage::GPUOnly;
	ResourceState    initialState = ResourceState::General;
	Ownership        ownership = Ownership::Reference;
};

class Buffer final : public DeviceObject<BufferCreateInfo>
{
	friend class RenderDevice;
public:
	uint64_t                GetSize() const { return m_createInfo.size; }
	uint32_t                GetStructuredElementStride() const { return m_createInfo.structuredElementStride; }
	const BufferUsageFlags& GetUsageFlags() const { return m_createInfo.usageFlags; }
	VkBufferPtr             GetVkBuffer() const { return m_buffer; }

	Result MapMemory(uint64_t offset, void** mappedAddress);
	void   UnmapMemory();

	Result CopyFromSource(uint32_t dataSize, const void* srcData);
	Result CopyToDest(uint32_t dataSize, void* destData);

private:
	Result createApiObjects(const BufferCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkBufferPtr       m_buffer;
	VmaAllocationPtr  m_allocation;
	VmaAllocationInfo m_allocationInfo = {};
};

struct IndexBufferView final
{
	IndexBufferView() {}
	IndexBufferView(const Buffer* pBuffer_, IndexType indexType_, uint64_t offset_ = 0, uint64_t size_ = WHOLE_SIZE) : pBuffer(pBuffer_), indexType(indexType_), offset(offset_), size(size_) {}

	const Buffer* pBuffer = nullptr;
	IndexType     indexType = IndexType::Uint16;
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

//=============================================================================
#pragma region [ Image ]

struct ImageCreateInfo final
{
	ImageType              type = ImageType::Image2D;
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               depth = 0;
	Format                 format = Format::Undefined;
	SampleCount            sampleCount = SampleCount::Sample1;
	uint32_t               mipLevelCount = 1;
	uint32_t               arrayLayerCount = 1;
	ImageUsageFlags        usageFlags = ImageUsageFlags::SampledImage();
	MemoryUsage            memoryUsage = MemoryUsage::GPUOnly;    // D3D12 will fail on any other memory usage
	ResourceState          initialState = ResourceState::General; // This may not be the best choice
	float4                 RTVClearValue = { 0, 0, 0, 0 };        // Optimized RTV clear value
	DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };        // Optimized DSV clear value
	void*                  ApiObject = nullptr;                   // [OPTIONAL] For external images such as swapchain images
	Ownership              ownership = Ownership::Reference;
	bool                   concurrentMultiQueueUsage = false;
	ImageCreateFlags       createFlags = {};

	// Returns a create info for sampled image
	static ImageCreateInfo SampledImage2D(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SampleCount::Sample1,
		MemoryUsage memoryUsage = MemoryUsage::GPUOnly);

	// Returns a create info for sampled image and depth stencil target
	static ImageCreateInfo DepthStencilTarget(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SampleCount::Sample1);

	// Returns a create info for sampled image and render target
	static ImageCreateInfo RenderTarget2D(
		uint32_t    width,
		uint32_t    height,
		Format      format,
		SampleCount sampleCount = SampleCount::Sample1);
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
	const float4&                 GetRTVClearValue() const { return m_createInfo.RTVClearValue; }
	const DepthStencilClearValue& GetDSVClearValue() const { return m_createInfo.DSVClearValue; }
	bool                          GetConcurrentMultiQueueUsageEnabled() const { return m_createInfo.concurrentMultiQueueUsage; }
	ImageCreateFlags              GetCreateFlags() const { return m_createInfo.createFlags; }

	VkImagePtr                    GetVkImage() const { return m_image; }
	VkFormat                      GetVkFormat() const { return m_vkFormat; }
	VkImageAspectFlags            GetVkImageAspectFlags() const { return m_imageAspect; }

	// Convenience functions
	ImageViewType GuessImageViewType(bool isCube = false) const;

	Result MapMemory(uint64_t offset, void** mappedAddress);
	void   UnmapMemory();

private:
	Result createApiObjects(const ImageCreateInfo& createInfo) final;
	void   destroyApiObjects() final;

	VkImagePtr         m_image;
	VmaAllocationPtr   m_allocation;
	VmaAllocationInfo  m_allocationInfo = {};
	VkFormat           m_vkFormat = VK_FORMAT_UNDEFINED;
	VkImageAspectFlags m_imageAspect = InvalidValue<VkImageAspectFlags>();
};

namespace internal
{
	class ImageResourceView final
	{
	public:
		ImageResourceView(VkImageViewPtr vkImageView, VkImageLayout layout) : m_imageView(vkImageView), m_imageLayout(layout) {}

		VkImageViewPtr GetVkImageView() const { return m_imageView; }
		VkImageLayout  GetVkImageLayout() const { return m_imageLayout; }

	private:
		VkImageViewPtr m_imageView;
		VkImageLayout  m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	};
} // namespace internal

// This class exists to genericize descriptor updates for Vulkan's 'image' based resources.
class ImageView
{
public:
	ImageView() = default;
	virtual ~ImageView() = default;

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
	Filter                  magFilter = Filter::Nearest;
	Filter                  minFilter = Filter::Nearest;
	SamplerMipmapMode       mipmapMode = SamplerMipmapMode::Nearest;
	SamplerAddressMode      addressModeU = SamplerAddressMode::Repeat;
	SamplerAddressMode      addressModeV = SamplerAddressMode::Repeat;
	SamplerAddressMode      addressModeW = SamplerAddressMode::Repeat;
	SamplerReductionMode    reductionMode = SamplerReductionMode::Standard;
	float                   mipLodBias = 0.0f;
	bool                    anisotropyEnable = false;
	float                   maxAnisotropy = 0.0f;
	bool                    compareEnable = false;
	CompareOp               compareOp = CompareOp::Never;
	float                   minLod = 0.0f;
	float                   maxLod = 1.0f;
	BorderColor             borderColor = BorderColor::FloatTransparentBlack;
	SamplerYcbcrConversion* YcbcrConversion = nullptr; // Leave null if not required.
	Ownership               ownership = Ownership::Reference;
	SamplerCreateFlags      createFlags = {};
};

class Sampler final : public DeviceObject<SamplerCreateInfo>
{
public:
	VkSamplerPtr GetVkSampler() const { return m_sampler; }

private:
	Result createApiObjects(const SamplerCreateInfo& createInfo) final;
	void   destroyApiObjects() final;

	VkSamplerPtr m_sampler;
};

struct DepthStencilViewCreateInfo final
{
	Image*            image = nullptr;
	ImageViewType     imageViewType = ImageViewType::Undefined;
	Format            format = Format::Undefined;
	uint32_t          mipLevel = 0;
	uint32_t          mipLevelCount = 0;
	uint32_t          arrayLayer = 0;
	uint32_t          arrayLayerCount = 0;
	ComponentMapping  components = {};
	AttachmentLoadOp  depthLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp depthStoreOp = AttachmentStoreOp::Store;
	AttachmentLoadOp  stencilLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::Store;
	Ownership         ownership = Ownership::Reference;

	static DepthStencilViewCreateInfo GuessFromImage(Image* image);
};

class DepthStencilView final : public DeviceObject<DepthStencilViewCreateInfo>, public ImageView
{
public:
	ImagePtr          GetImage() const { return m_createInfo.image; }
	Format            GetFormat() const { return m_createInfo.format; }
	SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t          GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t          GetArrayLayer() const { return m_createInfo.arrayLayer; }
	AttachmentLoadOp  GetDepthLoadOp() const { return m_createInfo.depthLoadOp; }
	AttachmentStoreOp GetDepthStoreOp() const { return m_createInfo.depthStoreOp; }
	AttachmentLoadOp  GetStencilLoadOp() const { return m_createInfo.stencilLoadOp; }
	AttachmentStoreOp GetStencilStoreOp() const { return m_createInfo.stencilStoreOp; }
	VkImageViewPtr    GetVkImageView() const { return m_imageView; }

private:
	Result createApiObjects(const DepthStencilViewCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkImageViewPtr m_imageView;
};

struct RenderTargetViewCreateInfo final
{
	Image*            image = nullptr;
	ImageViewType     imageViewType = ImageViewType::Undefined;
	Format            format = Format::Undefined;
	uint32_t          mipLevel = 0;
	uint32_t          mipLevelCount = 0;
	uint32_t          arrayLayer = 0;
	uint32_t          arrayLayerCount = 0;
	ComponentMapping  components = {};
	AttachmentLoadOp  loadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
	Ownership         ownership = Ownership::Reference;

	static RenderTargetViewCreateInfo GuessFromImage(Image* image);
};

class RenderTargetView final : public DeviceObject<RenderTargetViewCreateInfo>, public ImageView
{
public:
	ImagePtr          GetImage() const { return m_createInfo.image; }
	Format            GetFormat() const { return m_createInfo.format; }
	SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t          GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t          GetArrayLayer() const { return m_createInfo.arrayLayer; }
	AttachmentLoadOp  GetLoadOp() const { return m_createInfo.loadOp; }
	AttachmentStoreOp GetStoreOp() const { return m_createInfo.storeOp; }
	VkImageViewPtr    GetVkImageView() const { return m_imageView; }

private:
	Result createApiObjects(const RenderTargetViewCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkImageViewPtr m_imageView;
};

struct SampledImageViewCreateInfo final
{
	Image*                  image = nullptr;
	ImageViewType           imageViewType = ImageViewType::Undefined;
	Format                  format = Format::Undefined;
	SampleCount             sampleCount = SampleCount::Sample1;
	uint32_t                mipLevel = 0;
	uint32_t                mipLevelCount = 0;
	uint32_t                arrayLayer = 0;
	uint32_t                arrayLayerCount = 0;
	ComponentMapping        components = {};
	SamplerYcbcrConversion* ycbcrConversion = nullptr; // Leave null if not required.
	Ownership               ownership = Ownership::Reference;

	static SampledImageViewCreateInfo GuessFromImage(Image* image);
};

class SampledImageView final : public DeviceObject<SampledImageViewCreateInfo>, public ImageView
{
public:
	ImagePtr                GetImage() const { return m_createInfo.image; }
	ImageViewType           GetImageViewType() const { return m_createInfo.imageViewType; }
	Format                  GetFormat() const { return m_createInfo.format; }
	SampleCount             GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t                GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t                GetMipLevelCount() const { return m_createInfo.mipLevelCount; }
	uint32_t                GetArrayLayer() const { return m_createInfo.arrayLayer; }
	uint32_t                GetArrayLayerCount() const { return m_createInfo.arrayLayerCount; }
	const ComponentMapping& GetComponents() const { return m_createInfo.components; }
	VkImageViewPtr          GetVkImageView() const { return m_imageView; }

private:
	Result createApiObjects(const SampledImageViewCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkImageViewPtr m_imageView;
};

// SamplerYcbcrConversionCreateInfo defines a color model conversion for a texture, sampler, or sampled image.
struct SamplerYcbcrConversionCreateInfo final
{
	Format               format = Format::Undefined;
	YcbcrModelConversion ycbcrModel = YcbcrModelConversion::RGBIdentity;
	YcbcrRange           ycbcrRange = YcbcrRange::ITU_FULL;
	ComponentMapping     components = {};
	ChromaLocation       xChromaOffset = ChromaLocation::CositedEven;
	ChromaLocation       yChromaOffset = ChromaLocation::CositedEven;
	Filter               filter = Filter::Linear;
	bool                 forceExplicitReconstruction = false;
};

class SamplerYcbcrConversion final : public DeviceObject<SamplerYcbcrConversionCreateInfo>
{
public:
	VkSamplerYcbcrConversionPtr GetVkSamplerYcbcrConversion() const { return m_samplerYcbcrConversion; }

private:
	Result createApiObjects(const SamplerYcbcrConversionCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkSamplerYcbcrConversionPtr m_samplerYcbcrConversion;
};

struct StorageImageViewCreateInfo final
{
	Image*           image = nullptr;
	ImageViewType    imageViewType = ImageViewType::Undefined;
	Format           format = Format::Undefined;
	uint32_t         mipLevel = 0;
	uint32_t         mipLevelCount = 0;
	uint32_t         arrayLayer = 0;
	uint32_t         arrayLayerCount = 0;
	ComponentMapping components = {};
	Ownership        ownership = Ownership::Reference;

	static StorageImageViewCreateInfo GuessFromImage(Image* image);
};

class StorageImageView final : public DeviceObject<StorageImageViewCreateInfo>, public ImageView
{
public:
	ImagePtr       GetImage() const { return m_createInfo.image; }
	ImageViewType  GetImageViewType() const { return m_createInfo.imageViewType; }
	Format         GetFormat() const { return m_createInfo.format; }
	SampleCount    GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t       GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t       GetMipLevelCount() const { return m_createInfo.mipLevelCount; }
	uint32_t       GetArrayLayer() const { return m_createInfo.arrayLayer; }
	uint32_t       GetArrayLayerCount() const { return m_createInfo.arrayLayerCount; }
	VkImageViewPtr GetVkImageView() const { return m_imageView; }

private:
	Result createApiObjects(const StorageImageViewCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkImageViewPtr m_imageView;
};

#pragma endregion

//=============================================================================
#pragma region [ Texture ]

struct TextureCreateInfo final
{
	Image*                  image = nullptr;
	ImageType               imageType = ImageType::Image2D;
	uint32_t                width = 0;
	uint32_t                height = 0;
	uint32_t                depth = 0;
	Format                  imageFormat = Format::Undefined;
	SampleCount             sampleCount = SampleCount::Sample1;
	uint32_t                mipLevelCount = 1;
	uint32_t                arrayLayerCount = 1;
	ImageUsageFlags         usageFlags = ImageUsageFlags::SampledImage();
	MemoryUsage             memoryUsage = MemoryUsage::GPUOnly;
	ResourceState           initialState = ResourceState::General;           // This may not be the best choice
	float4                  RTVClearValue = { 0, 0, 0, 0 };                  // Optimized RTV clear value
	DepthStencilClearValue  DSVClearValue = { 1.0f, 0xFF };                  // Optimized DSV clear value
	ImageViewType           sampledImageViewType = ImageViewType::Undefined; // Guesses from image if UNDEFINED
	Format                  sampledImageViewFormat = Format::Undefined;      // Guesses from image if UNDEFINED
	SamplerYcbcrConversion* sampledImageYcbcrConversion = nullptr;           // Leave null if not Ycbcr, or not using sampled image.
	Format                  renderTargetViewFormat = Format::Undefined;      // Guesses from image if UNDEFINED
	Format                  depthStencilViewFormat = Format::Undefined;      // Guesses from image if UNDEFINED
	Format                  storageImageViewFormat = Format::Undefined;      // Guesses from image if UNDEFINED
	Ownership               ownership = Ownership::Reference;
	bool                    concurrentMultiQueueUsage = false;
	ImageCreateFlags        imageCreateFlags = {};
};

class Texture final : public DeviceObject<TextureCreateInfo>
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
	Result createApiObjects(const TextureCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	ImagePtr            m_image;
	SampledImageViewPtr m_sampledImageView;
	RenderTargetViewPtr m_renderTargetView;
	DepthStencilViewPtr m_depthStencilView;
	StorageImageViewPtr m_storageImageView;
};

#pragma endregion

//=============================================================================
#pragma region [ RenderPass ]

// Use this if the RTVs and/or the DSV exists.
struct RenderPassCreateInfo final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               arrayLayerCount = 1;
	uint32_t               renderTargetCount = 0;
	MultiViewState         multiViewState = {};
	RenderTargetView*      renderTargetViews[MaxRenderTargets] = {};
	DepthStencilView*      depthStencilView = nullptr;
	ResourceState          depthStencilState = ResourceState::DepthStencilWrite;
	float4                 renderTargetClearValues[MaxRenderTargets] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	Ownership              ownership = Ownership::Reference;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this RenderPass must use the same shading rate mode
	// (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr  shadingRatePattern = nullptr;

	void SetAllRenderTargetClearValue(const float4& value);
};

// Use this version if the format(s) are know but images and views need creation.
// RTVs, DSV, and backing images will be created using the criteria provided in this struct.
struct RenderPassCreateInfo2 final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               arrayLayerCount = 1;
	MultiViewState         multiViewState = {};
	SampleCount            sampleCount = SampleCount::Sample1;
	uint32_t               renderTargetCount = 0;
	Format                 renderTargetFormats[MaxRenderTargets] = {};
	Format                 depthStencilFormat = Format::Undefined;
	ImageUsageFlags        renderTargetUsageFlags[MaxRenderTargets] = {};
	ImageUsageFlags        depthStencilUsageFlags = {};
	float4                 renderTargetClearValues[MaxRenderTargets] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	AttachmentLoadOp       renderTargetLoadOps[MaxRenderTargets] = { AttachmentLoadOp::Load };
	AttachmentStoreOp      renderTargetStoreOps[MaxRenderTargets] = { AttachmentStoreOp::Store };
	AttachmentLoadOp       depthLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp      depthStoreOp = AttachmentStoreOp::Store;
	AttachmentLoadOp       stencilLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp      stencilStoreOp = AttachmentStoreOp::Store;
	ResourceState          renderTargetInitialStates[MaxRenderTargets] = { ResourceState::Undefined };
	ResourceState          depthStencilInitialState = ResourceState::Undefined;
	Ownership              ownership = Ownership::Reference;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this RenderPass must use the same shading rate mode (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr  shadingRatePattern = nullptr;

	void SetAllRenderTargetUsageFlags(const ImageUsageFlags& flags);
	void SetAllRenderTargetClearValue(const float4& value);
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
	Image*                 renderTargetImages[MaxRenderTargets] = {};
	Image*                 depthStencilImage = nullptr;
	ResourceState          depthStencilState = ResourceState::DepthStencilWrite;
	float4                 renderTargetClearValues[MaxRenderTargets] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	AttachmentLoadOp       renderTargetLoadOps[MaxRenderTargets] = { AttachmentLoadOp::Load };
	AttachmentStoreOp      renderTargetStoreOps[MaxRenderTargets] = { AttachmentStoreOp::Store };
	AttachmentLoadOp       depthLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp      depthStoreOp = AttachmentStoreOp::Store;
	AttachmentLoadOp       stencilLoadOp = AttachmentLoadOp::Load;
	AttachmentStoreOp      stencilStoreOp = AttachmentStoreOp::Store;
	Ownership              ownership = Ownership::Reference;

	// If `pShadingRatePattern` is not null, then the pipeline targeting this RenderPass must use the same shading rate mode (`GraphicsPipelineCreateInfo.shadingRateMode`).
	ShadingRatePatternPtr  shadingRatePattern = nullptr;

	void SetAllRenderTargetClearValue(const float4& value);
	void SetAllRenderTargetLoadOp(AttachmentLoadOp op);
	void SetAllRenderTargetStoreOp(AttachmentStoreOp op);
	void SetAllRenderTargetToClear();
};

namespace internal
{
	struct RenderPassCreateInfo final
	{
		enum CreateInfoVersion
		{
			CREATE_INFO_VERSION_UNDEFINED = 0,
			CREATE_INFO_VERSION_1 = 1,
			CREATE_INFO_VERSION_2 = 2,
			CREATE_INFO_VERSION_3 = 3,
		};

		Ownership           ownership = Ownership::Reference;
		CreateInfoVersion   version = CREATE_INFO_VERSION_UNDEFINED;
		uint32_t            width = 0;
		uint32_t            height = 0;
		uint32_t            renderTargetCount = 0;
		uint32_t            arrayLayerCount = 1;
		MultiViewState      multiViewState = {};
		ResourceState       depthStencilState = ResourceState::DepthStencilWrite;
		ShadingRatePattern* shadingRatePattern = nullptr;

		// Data unique to RenderPassCreateInfo
		struct
		{
			RenderTargetView* renderTargetViews[MaxRenderTargets] = {};
			DepthStencilView* depthStencilView = nullptr;
		} V1;

		// Data unique to RenderPassCreateInfo2
		struct
		{
			SampleCount     sampleCount = SampleCount::Sample1;
			Format          renderTargetFormats[MaxRenderTargets] = {};
			Format          depthStencilFormat = Format::Undefined;
			ImageUsageFlags renderTargetUsageFlags[MaxRenderTargets] = {};
			ImageUsageFlags depthStencilUsageFlags = {};
			ResourceState   renderTargetInitialStates[MaxRenderTargets] = { ResourceState::Undefined };
			ResourceState   depthStencilInitialState = ResourceState::Undefined;
		} V2;

		// Data unique to RenderPassCreateInfo3
		struct
		{
			Image* renderTargetImages[MaxRenderTargets] = {};
			Image* depthStencilImage = nullptr;
		} V3;

		// Clear values
		float4                 renderTargetClearValues[MaxRenderTargets] = {};
		DepthStencilClearValue depthStencilClearValue = {};

		// Load/store ops
		AttachmentLoadOp  renderTargetLoadOps[MaxRenderTargets] = { AttachmentLoadOp::Load };
		AttachmentStoreOp renderTargetStoreOps[MaxRenderTargets] = { AttachmentStoreOp::Store };
		AttachmentLoadOp  depthLoadOp = AttachmentLoadOp::Load;
		AttachmentStoreOp depthStoreOp = AttachmentStoreOp::Store;
		AttachmentLoadOp  stencilLoadOp = AttachmentLoadOp::Load;
		AttachmentStoreOp stencilStoreOp = AttachmentStoreOp::Store;

		RenderPassCreateInfo() = default;
		RenderPassCreateInfo(const vkr::RenderPassCreateInfo& obj);
		RenderPassCreateInfo(const vkr::RenderPassCreateInfo2& obj);
		RenderPassCreateInfo(const vkr::RenderPassCreateInfo3& obj);
	};
}

class RenderPass final: public DeviceObject<internal::RenderPassCreateInfo>
{
	friend class RenderDevice;
public:
	const Rect&     GetRenderArea() const { return m_renderArea; }
	const Rect&     GetScissor() const { return m_renderArea; }
	const Viewport& GetViewport() const { return m_viewport; }

	uint32_t GetRenderTargetCount() const { return m_createInfo.renderTargetCount; }
	bool     HasDepthStencil() const { return m_depthStencilImage ? true : false; }

	Result GetRenderTargetView(uint32_t index, RenderTargetView** view) const;
	Result GetDepthStencilView(DepthStencilView** view) const;

	Result GetRenderTargetImage(uint32_t index, Image** image) const;
	Result GetDepthStencilImage(Image** image) const;

	// This only applies to RenderPass objects created using RenderPassCreateInfo2.
	// These functions will set 'isExternal' to true resulting in these objects NOT getting destroyed when the encapsulating RenderPass object is destroyed.
	// Calling these fuctions on RenderPass objects created using using RenderPassCreateInfo will still return a valid object if the index or DSV object exists.
	Result DisownRenderTargetView(uint32_t index, RenderTargetView** view);
	Result DisownDepthStencilView(DepthStencilView** view);
	Result DisownRenderTargetImage(uint32_t index, Image** image);
	Result DisownDepthStencilImage(Image** image);

	// Convenience functions returns empty ptr if index is out of range or DSV object does not exist.
	RenderTargetViewPtr GetRenderTargetView(uint32_t index) const;
	DepthStencilViewPtr GetDepthStencilView() const;
	ImagePtr            GetRenderTargetImage(uint32_t index) const;
	ImagePtr            GetDepthStencilImage() const;

	// Returns index of image otherwise returns UINT32_MAX
	uint32_t GetRenderTargetImageIndex(const Image* image) const;

	// Returns true if render targets or depth stencil contains AttachmentLoadOp::Clear
	bool HasLoadOpClear() const { return m_hasLoadOpClear; }

	VkRenderPassPtr  GetVkRenderPass() const { return m_renderPass; }
	VkFramebufferPtr GetVkFramebuffer() const { return m_framebuffer; }

private:
	Result createApiObjects(const internal::RenderPassCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	Result createImagesAndViewsV1(const internal::RenderPassCreateInfo& createInfo);
	Result createImagesAndViewsV2(const internal::RenderPassCreateInfo& createInfo);
	Result createImagesAndViewsV3(const internal::RenderPassCreateInfo& createInfo);

	Result createRenderPass(const internal::RenderPassCreateInfo& createInfo);
	Result createFramebuffer(const internal::RenderPassCreateInfo& createInfo);

	VkRenderPassPtr                  m_renderPass;
	VkFramebufferPtr                 m_framebuffer;
	Rect                             m_renderArea = {};
	Viewport                         m_viewport = {};
	std::vector<RenderTargetViewPtr> m_renderTargetViews;
	DepthStencilViewPtr              m_depthStencilView;
	std::vector<ImagePtr>            m_renderTargetImages;
	ImagePtr                         m_depthStencilImage;
	bool                             m_hasLoadOpClear = false;
};

VkResult CreateTransientRenderPass(RenderDevice* device, uint32_t renderTargetCount, const VkFormat* pRenderTargetFormats, VkFormat depthStencilFormat, VkSampleCountFlagBits sampleCount, uint32_t viewMask, uint32_t correlationMask, VkRenderPass* renderPass, ShadingRateMode shadingRateMode = ShadingRateMode::None);

#pragma endregion

//=============================================================================
#pragma region [ DrawPass ]

struct RenderPassBeginInfo;

// Use this version if the format(s) are known but images need creation.
// Backing images will be created using the criteria provided in this struct.
struct DrawPassCreateInfo final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	SampleCount            sampleCount = SampleCount::Sample1;
	uint32_t               renderTargetCount = 0;
	Format                 renderTargetFormats[MaxRenderTargets] = {};
	Format                 depthStencilFormat = Format::Undefined;
	ImageUsageFlags        renderTargetUsageFlags[MaxRenderTargets] = {};
	ImageUsageFlags        depthStencilUsageFlags = {};
	ResourceState          renderTargetInitialStates[MaxRenderTargets] = { ResourceState::RenderTarget };
	ResourceState          depthStencilInitialState = ResourceState::DepthStencilWrite;
	float4                 renderTargetClearValues[MaxRenderTargets] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	ShadingRatePattern*    shadingRatePattern = nullptr;
	ImageCreateFlags       imageCreateFlags = {};
};

// Use this version if the images exists.
struct DrawPassCreateInfo2 final
{
	uint32_t               width = 0;
	uint32_t               height = 0;
	uint32_t               renderTargetCount = 0;
	Image*                 renderTargetImages[MaxRenderTargets] = {};
	Image*                 depthStencilImage = nullptr;
	ResourceState          depthStencilState = ResourceState::DepthStencilWrite;
	float4                 renderTargetClearValues[MaxRenderTargets] = {};
	DepthStencilClearValue depthStencilClearValue = {};
	ShadingRatePattern*    shadingRatePattern = nullptr;
};

// Use this version if the textures exists.
struct DrawPassCreateInfo3 final
{
	uint32_t            width = 0;
	uint32_t            height = 0;
	uint32_t            renderTargetCount = 0;
	Texture*            renderTargetTextures[MaxRenderTargets] = {};
	Texture*            depthStencilTexture = nullptr;
	ResourceState       depthStencilState = ResourceState::DepthStencilWrite;
	ShadingRatePattern* shadingRatePattern = nullptr;
};

namespace internal
{
	struct DrawPassCreateInfo final
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
		ResourceState       depthStencilState = ResourceState::DepthStencilWrite;
		ShadingRatePattern* shadingRatePattern = nullptr;

		// Data unique to DrawPassCreateInfo1
		struct
		{
			SampleCount      sampleCount = SampleCount::Sample1;
			Format           renderTargetFormats[MaxRenderTargets] = {};
			Format           depthStencilFormat = Format::Undefined;
			ImageUsageFlags  renderTargetUsageFlags[MaxRenderTargets] = {};
			ImageUsageFlags  depthStencilUsageFlags = {};
			ResourceState    renderTargetInitialStates[MaxRenderTargets] = { ResourceState::RenderTarget };
			ResourceState    depthStencilInitialState = ResourceState::DepthStencilWrite;
			ImageCreateFlags imageCreateFlags = {};
		} V1;

		// Data unique to DrawPassCreateInfo2
		struct
		{
			Image* renderTargetImages[MaxRenderTargets] = {};
			Image* depthStencilImage = nullptr;
		} V2;

		// Data unique to DrawPassCreateInfo3
		struct
		{
			Texture* renderTargetTextures[MaxRenderTargets] = {};
			Texture* depthStencilTexture = nullptr;
		} V3;

		// Clear values
		float4                 renderTargetClearValues[MaxRenderTargets] = {};
		DepthStencilClearValue depthStencilClearValue = {};

		DrawPassCreateInfo() = default;
		DrawPassCreateInfo(const vkr::DrawPassCreateInfo& obj);
		DrawPassCreateInfo(const vkr::DrawPassCreateInfo2& obj);
		DrawPassCreateInfo(const vkr::DrawPassCreateInfo3& obj);
	};
}

class DrawPass final : public DeviceObject<internal::DrawPassCreateInfo>
{
public:
	uint32_t        GetWidth() const { return m_createInfo.width; }
	uint32_t        GetHeight() const { return m_createInfo.height; }
	const Rect&     GetRenderArea() const;
	const Rect&     GetScissor() const;
	const Viewport& GetViewport() const;
	uint32_t        GetRenderTargetCount() const { return m_createInfo.renderTargetCount; }
	Result          GetRenderTargetTexture(uint32_t index, Texture** renderTarget) const;
	Texture*        GetRenderTargetTexture(uint32_t index) const;
	bool            HasDepthStencil() const { return m_depthStencilTexture ? true : false; }
	Result          GetDepthStencilTexture(Texture** depthStencil) const;
	Texture*        GetDepthStencilTexture() const;

	void PrepareRenderPassBeginInfo(const DrawPassClearFlags& clearFlags, RenderPassBeginInfo* beginInfo) const;

private:
	Result createApiObjects(const internal::DrawPassCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	Result createTexturesV1(const internal::DrawPassCreateInfo& createInfo);
	Result createTexturesV2(const internal::DrawPassCreateInfo& createInfo);
	Result createTexturesV3(const internal::DrawPassCreateInfo& createInfo);

	Rect                    m_renderArea = {};
	std::vector<TexturePtr> m_renderTargetTextures;
	TexturePtr              m_depthStencilTexture;

	struct Pass final
	{
		uint32_t clearMask = 0;
		RenderPassPtr renderPass;
	};
	std::vector<Pass> m_passes;
};

#pragma endregion

//=============================================================================
#pragma region [ Descriptor ]

struct DescriptorBinding final
{
	DescriptorBinding() = default;
	DescriptorBinding(
		uint32_t               binding_,
		DescriptorType         type_,
		uint32_t               arrayCount_ = 1,
		ShaderStageBits        shaderVisibility_ = SHADER_STAGE_ALL,
		DescriptorBindingFlags flags_ = 0)
		: binding(binding_)
		, type(type_)
		, arrayCount(arrayCount_)
		, shaderVisibility(shaderVisibility_)
		, flags(flags_) {
	}

	uint32_t                binding = VALUE_IGNORED;
	DescriptorType          type = DescriptorType::Undefined;
	uint32_t                arrayCount = 1; // WARNING: Not VkDescriptorSetLayoutBinding::descriptorCount
	ShaderStageBits         shaderVisibility = SHADER_STAGE_ALL; // Single value not set of flags (see note above)
	DescriptorBindingFlags  flags;
	std::vector<SamplerPtr> immutableSamplers;
};

struct WriteDescriptor final
{
	uint32_t         binding = VALUE_IGNORED;
	uint32_t         arrayIndex = 0;
	DescriptorType   type = DescriptorType::Undefined;
	uint64_t         bufferOffset = 0;
	uint64_t         bufferRange = 0;
	uint32_t         structuredElementCount = 0;
	const Buffer*    buffer = nullptr;
	const ImageView* imageView = nullptr;
	const Sampler*   sampler = nullptr;
};

struct DescriptorPoolCreateInfo final
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
	VkDescriptorPoolPtr GetVkDescriptorPool() const { return m_descriptorPool; }

private:
	Result createApiObjects(const DescriptorPoolCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkDescriptorPoolPtr m_descriptorPool;
};

namespace internal
{
	struct DescriptorSetCreateInfo final
	{
		DescriptorPool* pool = nullptr;
		const DescriptorSetLayout* layout = nullptr;
	};
}

class DescriptorSet final : public DeviceObject<internal::DescriptorSetCreateInfo>
{
public:
	DescriptorPoolPtr          GetPool() const { return m_createInfo.pool; }
	const DescriptorSetLayout* GetLayout() const { return m_createInfo.layout; }

	Result UpdateDescriptors(uint32_t writeCount, const WriteDescriptor* writes);
	Result UpdateSampler(uint32_t binding, uint32_t  arrayIndex, const Sampler* sampler);
	Result UpdateSampledImage(uint32_t binding, uint32_t arrayIndex, const SampledImageView* imageView);
	Result UpdateSampledImage(uint32_t binding, uint32_t arrayIndex, const Texture* texture);
	Result UpdateStorageImage(uint32_t binding, uint32_t arrayIndex, const Texture* texture);
	Result UpdateUniformBuffer(uint32_t binding, uint32_t arrayIndex, const Buffer* buffer, uint64_t offset = 0, uint64_t range = WHOLE_SIZE);

	VkDescriptorSetPtr GetVkDescriptorSet() const { return m_descriptorSet; }

private:
	Result createApiObjects(const internal::DescriptorSetCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkDescriptorSetPtr                  m_descriptorSet;
	VkDescriptorPoolPtr                 m_descriptorPool;

	// Reduce memory allocations during update process
	std::vector<VkWriteDescriptorSet>   m_writeStore;
	std::vector<VkDescriptorImageInfo>  m_imageInfoStore;
	std::vector<VkBufferView>           m_texelBufferStore;
	std::vector<VkDescriptorBufferInfo> m_bufferInfoStore;
	uint32_t                            m_writeCount = 0;
	uint32_t                            m_imageCount = 0;
	uint32_t                            m_texelBufferCount = 0;
	uint32_t                            m_bufferCount = 0;
};

struct DescriptorSetLayoutCreateInfo final
{
	DescriptorSetLayoutFlags flags;
	std::vector<DescriptorBinding> bindings;
};

class DescriptorSetLayout final : public DeviceObject<DescriptorSetLayoutCreateInfo>
{
	friend class RenderDevice;
public:
	bool IsPushable() const { return m_createInfo.flags.bits.pushable; }

	const std::vector<DescriptorBinding>& GetBindings() const { return m_createInfo.bindings; }

	VkDescriptorSetLayoutPtr GetVkDescriptorSetLayout() const { return m_descriptorSetLayout; }

private:
	Result createApiObjects(const DescriptorSetLayoutCreateInfo& createInfo) final;
	void destroyApiObjects() final;
	Result validateDescriptorBindingFlags(const DescriptorBindingFlags& flags) const;

	VkDescriptorSetLayoutPtr m_descriptorSetLayout;
};

#pragma endregion

//=============================================================================
#pragma region [ Shader ]

struct ShaderModuleCreateInfo final
{
	uint32_t size = 0;
	const char* code = nullptr;
};

class ShaderModule final : public DeviceObject<ShaderModuleCreateInfo>
{
public:
	VkShaderModulePtr GetVkShaderModule() const { return m_shaderModule; }

private:
	Result createApiObjects(const ShaderModuleCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkShaderModulePtr m_shaderModule;
};

#pragma endregion

//=============================================================================
#pragma region [ ShadingRate ]

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
	ShadingRateMode supportedShadingRateMode = ShadingRateMode::None;

	struct
	{
		bool supportsNonSubsampledImages;

		// Minimum/maximum size of the region of the render target corresponding to a single pixel in the FDM attachment.
		// This is *not* the minimum/maximum fragment density.
		Extent2D minTexelSize;
		Extent2D maxTexelSize;
	} fdm;

	struct
	{
		// Minimum/maximum size of the region of the render target corresponding to a single pixel in the VRS attachment.
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
	// Fragment density values are a ratio over 255, e.g. 255 means shade every pixel, and 128 means shade every other pixel.
	virtual uint32_t EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const = 0;

	// Encode a pair of fragment size values.
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

		uint32_t encodeFragmentSizeImpl(uint8_t xDensity, uint8_t yDensity) const;
		static uint32_t rawEncode(uint32_t width, uint32_t height);

		// Maps a requested shading rate to a supported shading rate.
		// The fragment width/height of the supported shading rate will be no larger than the fragment width/height of the requested shading rate.
		// Ties are broken lexicographically, e.g. if 2x2, 1x4 and 4x1 are supported, then 2x4 will be mapped to 2x2 but 4x2 will map to 4x1.
		std::array<uint8_t, kMaxEncodedShadingRate + 1> m_mapRateToSupported;
	};
} // namespace internal

struct ShadingRatePatternCreateInfo final
{
	// The size of the framebuffer image that will be used with the created ShadingRatePattern.
	Extent2D framebufferSize;

	// The size of the region of the framebuffer image that will correspond to a single pixel in the ShadingRatePattern image.
	Extent2D texelSize;

	// The shading rate mode (FDM or VRS).
	ShadingRateMode shadingRateMode = ShadingRateMode::None;

	// The sample count of the render targets using this shading rate pattern.
	SampleCount sampleCount;
};

// An image representing fragment sizes/densities that can be used in a render pass to control the shading rate.
class ShadingRatePattern final : public DeviceObject<ShadingRatePatternCreateInfo>
{
public:
	// The shading rate mode (FDM or VRS).
	ShadingRateMode GetShadingRateMode() const { return m_shadingRateMode; }

	// The image contaning encoded fragment sizes/densities.
	ImagePtr GetAttachmentImage() const { return m_attachmentImage; }

	// The width/height of the image contaning encoded fragment sizes/densities.
	uint32_t GetAttachmentWidth() const { return m_attachmentImage->GetWidth(); }
	uint32_t GetAttachmentHeight() const { return m_attachmentImage->GetHeight(); }

	// The width/height of the region of the render target image corresponding to a single pixel in the image containing fragment sizes/densities.
	uint32_t GetTexelWidth() const { return m_texelSize.width; }
	uint32_t GetTexelHeight() const { return m_texelSize.height; }

	// The sample count of the render targets using this shading rate pattern.
	SampleCount GetSampleCount() const { return m_sampleCount; }

	// Create a bitmap suitable for uploading fragment density/size to this pattern.
	std::unique_ptr<Bitmap> CreateBitmap() const;

	// Load fragment density/size from a bitmap of encoded values.
	Result LoadFromBitmap(Bitmap* bitmap);

	// Get the pixel format of a bitmap that can store the fragment density/size data.
	Bitmap::Format GetBitmapFormat() const;

	// Get an encoder that can encode fragment density/size values for this pattern.
	const ShadingRateEncoder* GetShadingRateEncoder() const;

	VkImageViewPtr GetAttachmentImageView() const { return m_attachmentView; }

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

		// Initializes the modified VkRenderPassCreateInfo2, based on the values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2, with appropriate modifications for the shading rate implementation.
		ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo& vkci);
		ModifiedRenderPassCreateInfo& Initialize(const VkRenderPassCreateInfo2& vkci);

		// Returns the modified VkRenderPassCreateInfo2.
		// The returned pointer, as well as pointers and arrays inside the VkRenderPassCreateInfo2 struct, point to memory owned by this ModifiedRenderPassCreateInfo object, and so cannot be used after this object is destroyed.
		std::shared_ptr<const VkRenderPassCreateInfo2> Get()
		{
			return std::shared_ptr<const VkRenderPassCreateInfo2>(shared_from_this(), &m_vkRenderPassCreateInfo2);
		}

	protected:
		// Initializes the internal VkRenderPassCreateInfo2, based on the values in the input VkRenderPassCreateInfo/VkRenderPassCreateInfo2.
		// All arrays are copied to internal vectors, and the internal VkRenderPassCreateInfo2 references the data in these vectors, rather than the poitners in the input VkRenderPassCreateInfo.
		void loadVkRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci);
		void loadVkRenderPassCreateInfo2(const VkRenderPassCreateInfo2& vkci);

		// Modifies the internal VkRenderPassCreateInfo2 to enable the shading rate implementation.
		virtual void updateRenderPassForShadingRateImplementation() = 0;

		VkRenderPassCreateInfo2               m_vkRenderPassCreateInfo2 = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2 };
		std::vector<VkAttachmentDescription2> m_attachments;
		std::vector<VkSubpassDescription2>    m_subpasses;
		struct SubpassAttachments final
		{
			std::vector<VkAttachmentReference2> inputAttachments;
			std::vector<VkAttachmentReference2> colorAttachments;
			std::vector<VkAttachmentReference2> resolveAttachments;
			VkAttachmentReference2              depthStencilAttachment;
			std::vector<uint32_t>               preserveAttachments;
		};
		std::vector<SubpassAttachments>   m_subpassAttachments;
		std::vector<VkSubpassDependency2> m_dependencies;
	};

	// Creates a ModifiedRenderPassCreateInfo that will modify
	// VkRenderPassCreateInfo/VkRenderPassCreateInfo2  to support the given ShadingRateMode on the given device.
	static std::shared_ptr<ModifiedRenderPassCreateInfo> createModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode);

	// Creates a ModifiedRenderPassCreateInfo that will modify
	// VkRenderPassCreateInfo/VkRenderPassCreateInfo2 to support this ShadingRatePattern.
	std::shared_ptr<ModifiedRenderPassCreateInfo> createModifiedRenderPassCreateInfo() const
	{
		return createModifiedRenderPassCreateInfo(GetDevice(), GetShadingRateMode());
	}

	// Handles modification of VkRenderPassCreateInfo(2) to add support for FDM.
	class FDMModifiedRenderPassCreateInfo final : public ModifiedRenderPassCreateInfo
	{
	private:
		void updateRenderPassForShadingRateImplementation() final;

		VkRenderPassFragmentDensityMapCreateInfoEXT m_fdmInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_FRAGMENT_DENSITY_MAP_CREATE_INFO_EXT };
	};

	// Handles modification of VkRenderPassCreateInfo(2) to add support for VRS.
	class VRSModifiedRenderPassCreateInfo final : public ModifiedRenderPassCreateInfo
	{
	public:
		VRSModifiedRenderPassCreateInfo(const ShadingRateCapabilities& capabilities)
			: m_capabilities(capabilities) {}

	private:
		void updateRenderPassForShadingRateImplementation() final;

		ShadingRateCapabilities                m_capabilities;
		VkFragmentShadingRateAttachmentInfoKHR m_vrsAttachmentInfo = { VK_STRUCTURE_TYPE_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR };
		VkAttachmentReference2                 m_vrsAttachmentRef = { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2_KHR };
	};

	Result createApiObjects(const ShadingRatePatternCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	std::unique_ptr<ShadingRateEncoder> m_shadingRateEncoder;
	VkImageViewPtr                      m_attachmentView;

	ShadingRateMode m_shadingRateMode;
	ImagePtr        m_attachmentImage;
	Extent2D        m_texelSize;
	SampleCount     m_sampleCount;
};

#pragma endregion

//=============================================================================
#pragma region [ Shading Rate Util ]

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

//=============================================================================
#pragma region [ Mesh ]

struct MeshVertexAttribute final
{
	Format format = Format::Undefined;

	// Use 0 to have stride calculated from format
	uint32_t stride = 0;

	// Not used for mesh/vertex buffer creation.
	// Gets calculated during creation for queries afterwards.
	uint32_t offset = 0;

	// [OPTIONAL] Useful for debugging.
	VertexSemantic vertexSemantic = VertexSemantic::Undefined;
};

struct MeshVertexBufferDescription final
{
	uint32_t attributeCount = 0;
	MeshVertexAttribute attributes[MaxVertexBindings] = {};

	// Use 0 to have stride calculated from attributes
	uint32_t stride = 0;

	VertexInputRate vertexInputRate = VertexInputRate::Vertex;
};

// Usage Notes:
//   - Index and vertex data configuration needs to make sense
//       - If \b indexCount is 0 then \b vertexCount cannot be 0
//   - To create a mesh without an index buffer, \b indexType must be IndexType::Undefined
//   - If \b vertexCount is 0 then no vertex buffers will be created
//       - This means vertex buffer information will be ignored
//   - Active elements in \b vertexBuffers cannot have an \b attributeCount of 0
struct MeshCreateInfo final
{
	MeshCreateInfo() = default;
	MeshCreateInfo(const Geometry& geometry);

	IndexType                   indexType = IndexType::Undefined;
	uint32_t                    indexCount = 0;
	uint32_t                    vertexCount = 0;
	uint32_t                    vertexBufferCount = 0;
	MeshVertexBufferDescription vertexBuffers[MaxVertexBindings] = {};
	MemoryUsage                 memoryUsage = MemoryUsage::GPUOnly;
};

// The \b Mesh class is a straight forward geometry container for the GPU.
// A \b Mesh instance consists of vertex data and an optional index buffer.
// The vertex data is stored in on or more vertex buffers. Each vertex buffer can store data for one or more attributes. The index data is stored in an index buffer.
// A \b Mesh instance does not store vertex binding information. Even if the create info is derived from a Geometry instance. This design is intentional since it enables calling applications to map vertex attributes and vertex buffers to how it sees fit. For convenience, the function \b Mesh::GetDerivedVertexBindings() returns vertex bindings derived from a \Mesh instance's vertex buffer descriptions.
class Mesh final : public DeviceObject<MeshCreateInfo>
{
public:
	IndexType GetIndexType() const { return m_createInfo.indexType; }
	uint32_t  GetIndexCount() const { return m_createInfo.indexCount; }
	BufferPtr GetIndexBuffer() const { return m_indexBuffer; }

	uint32_t                           GetVertexCount() const { return m_createInfo.vertexCount; }
	uint32_t                           GetVertexBufferCount() const { return CountU32(m_vertexBuffers); }
	BufferPtr                          GetVertexBuffer(uint32_t index) const;
	const MeshVertexBufferDescription* GetVertexBufferDescription(uint32_t index) const;

	//! Returns derived vertex bindings based on the vertex buffer description
	const std::vector<VertexBinding>& GetDerivedVertexBindings() const { return m_derivedVertexBindings; }

private:
	Result createApiObjects(const MeshCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	BufferPtr                                                      m_indexBuffer;
	std::vector<std::pair<BufferPtr, MeshVertexBufferDescription>> m_vertexBuffers;
	std::vector<VertexBinding>                                     m_derivedVertexBindings;
};

#pragma endregion

//=============================================================================
#pragma region [ Pipeline ]

struct ShaderStageInfo final
{
	const ShaderModule* module = nullptr;
	std::string entryPoint = "";
};

struct ComputePipelineCreateInfo final
{
	ShaderStageInfo CS = {};
	const PipelineInterface* pipelineInterface = nullptr;
};

class ComputePipeline final : public DeviceObject<ComputePipelineCreateInfo>
{
	friend class RenderDevice;
public:

	VkPipelinePtr GetVkPipeline() const { return m_pipeline; }

private:
	Result createApiObjects(const ComputePipelineCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkPipelinePtr m_pipeline;
};

struct VertexInputState final
{
	uint32_t bindingCount = 0;
	VertexBinding bindings[MaxVertexBindings] = {};
};

struct InputAssemblyState final
{
	PrimitiveTopology topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	bool primitiveRestartEnable = false;
};

struct TessellationState final
{
	uint32_t patchControlPoints = 0;
	TessellationDomainOrigin domainOrigin = TESSELLATION_DOMAIN_ORIGIN_UPPER_LEFT;
};

struct RasterState final
{
	bool depthClampEnable = false;
	bool rasterizeDiscardEnable = false;
	PolygonMode polygonMode = POLYGON_MODE_FILL;
	CullMode cullMode = CULL_MODE_NONE;
	FrontFace frontFace = FRONT_FACE_CCW;
	bool depthBiasEnable = false;
	float depthBiasConstantFactor = 0.0f;
	float depthBiasClamp = 0.0f;
	float depthBiasSlopeFactor = 0.0f;
	bool depthClipEnable = false;
	SampleCount rasterizationSamples = SampleCount::Sample1;
};

struct MultisampleState final
{
	bool alphaToCoverageEnable = false;
};

struct StencilOpState final
{
	StencilOp failOp = STENCIL_OP_KEEP;
	StencilOp passOp = STENCIL_OP_KEEP;
	StencilOp depthFailOp = STENCIL_OP_KEEP;
	CompareOp compareOp = CompareOp::Never;
	uint32_t compareMask = 0;
	uint32_t writeMask = 0;
	uint32_t reference = 0;
};

struct DepthStencilState final
{
	bool depthTestEnable = true;
	bool depthWriteEnable = true;
	CompareOp depthCompareOp = CompareOp::Less;
	bool depthBoundsTestEnable = false;
	float minDepthBounds = 0.0f;
	float maxDepthBounds = 1.0f;
	bool stencilTestEnable = false;
	StencilOpState front = {};
	StencilOpState back = {};
};

struct BlendAttachmentState final
{
	bool blendEnable = false;
	BlendFactor srcColorBlendFactor = BLEND_FACTOR_ONE;
	BlendFactor dstColorBlendFactor = BLEND_FACTOR_ZERO;
	BlendOp colorBlendOp = BLEND_OP_ADD;
	BlendFactor srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	BlendFactor dstAlphaBlendFactor = BLEND_FACTOR_ZERO;
	BlendOp alphaBlendOp = BLEND_OP_ADD;
	ColorComponentFlags colorWriteMask = ColorComponentFlags::RGBA();

	// These are best guesses based on random formulas off of the internet.
	// Correct later when authorative literature is found.
	static BlendAttachmentState BlendModeAdditive();
	static BlendAttachmentState BlendModeAlpha();
	static BlendAttachmentState BlendModeOver();
	static BlendAttachmentState BlendModeUnder();
	static BlendAttachmentState BlendModePremultAlpha();
};

struct ColorBlendState final
{
	bool logicOpEnable = false;
	LogicOp logicOp = LOGIC_OP_CLEAR;
	uint32_t blendAttachmentCount = 0;
	BlendAttachmentState blendAttachments[MaxRenderTargets] = {};
	float blendConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct OutputState final
{
	uint32_t renderTargetCount = 0;
	Format renderTargetFormats[MaxRenderTargets] = { Format::Undefined };
	Format depthStencilFormat = Format::Undefined;
};

struct GraphicsPipelineCreateInfo final
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
	ShadingRateMode          shadingRateMode = ShadingRateMode::None;
	MultiViewState           multiViewState = {};
	const PipelineInterface* pipelineInterface = nullptr;
	bool                     dynamicRenderPass = false;
};

struct GraphicsPipelineCreateInfo2 final
{
	ShaderStageInfo          VS = {};
	ShaderStageInfo          PS = {};
	VertexInputState         vertexInputState = {};
	PrimitiveTopology        topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	PolygonMode              polygonMode = POLYGON_MODE_FILL;
	CullMode                 cullMode = CULL_MODE_NONE;
	FrontFace                frontFace = FRONT_FACE_CCW;
	bool                     depthReadEnable = true;
	bool                     depthWriteEnable = true;
	CompareOp                depthCompareOp = CompareOp::Less;
	BlendMode                blendModes[MaxRenderTargets] = { BLEND_MODE_NONE };
	OutputState              outputState = {};
	ShadingRateMode          shadingRateMode = ShadingRateMode::None;
	MultiViewState           multiViewState = {};
	const PipelineInterface* pipelineInterface = nullptr;
	bool                     dynamicRenderPass = false;
};

namespace internal
{
	void FillOutGraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo2& srcCreateInfo, GraphicsPipelineCreateInfo* dstCreateInfo);
} // namespace internal

class GraphicsPipeline final: public DeviceObject<GraphicsPipelineCreateInfo>
{
	friend class RenderDevice;
public:
	VkPipelinePtr GetVkPipeline() const { return m_pipeline; }

private:
	Result createApiObjects(const GraphicsPipelineCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	Result initializeShaderStages(const GraphicsPipelineCreateInfo& pCreateInfo, std::vector<VkPipelineShaderStageCreateInfo>& shaderStages);
	Result initializeVertexInput(const GraphicsPipelineCreateInfo& pCreateInfo, std::vector<VkVertexInputAttributeDescription>& attribues, std::vector<VkVertexInputBindingDescription>& bindings, VkPipelineVertexInputStateCreateInfo& stateCreateInfo);
	Result initializeInputAssembly(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineInputAssemblyStateCreateInfo& stateCreateInfo);
	Result initializeTessellation(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineTessellationDomainOriginStateCreateInfoKHR& domainOriginStateCreateInfo, VkPipelineTessellationStateCreateInfo& stateCreateInfo);
	Result initializeViewports(VkPipelineViewportStateCreateInfo& stateCreateInfo);
	Result initializeRasterization(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthClipStateCreateInfo, VkPipelineRasterizationStateCreateInfo& stateCreateInfo);
	Result initializeMultisample(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineMultisampleStateCreateInfo& stateCreateInfo);
	Result initializeDepthStencil(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineDepthStencilStateCreateInfo& stateCreateInfo);
	Result initializeColorBlend(const GraphicsPipelineCreateInfo& pCreateInfo, std::vector<VkPipelineColorBlendAttachmentState>& attachments, VkPipelineColorBlendStateCreateInfo& stateCreateInfo);
	Result initializeDynamicState(std::vector<VkDynamicState>& dynamicStates, VkPipelineDynamicStateCreateInfo& stateCreateInfo);

	VkPipelinePtr m_pipeline;
};

struct PipelineInterfaceCreateInfo final
{
	uint32_t setCount = 0;
	struct
	{
		uint32_t set = VALUE_IGNORED; // Set number
		const DescriptorSetLayout* layout = nullptr; // Set layout
	} sets[MaxBoundDescriptorSets] = {};

	// VK: Push constants
	// DX: Root constants
	// Push/root constants are measured in DWORDs (uint32_t) aka 32-bit values.
	// The binding and set for push constants CAN NOT overlap with a binding AND set in sets (the struct immediately above this one). It's okay for push constants to be in an existing set at binding that is not used by an entry in the set layout.
	struct
	{
		uint32_t        count = 0;               // Measured in DWORDs, must be less than or equal to MAX_PUSH_CONSTANTS
		uint32_t        binding = VALUE_IGNORED; // D3D12 only, ignored by Vulkan
		uint32_t        set = VALUE_IGNORED;     // D3D12 only, ignored by Vulkan
		ShaderStageBits shaderVisiblity = SHADER_STAGE_ALL;
	} pushConstants;
};

class PipelineInterface final : public DeviceObject<PipelineInterfaceCreateInfo>
{
	friend class RenderDevice;
public:
	bool HasConsecutiveSetNumbers() const { return m_hasConsecutiveSetNumbers; }
	const std::vector<uint32_t>& GetSetNumbers() const { return m_setNumbers; }

	const DescriptorSetLayout* GetSetLayout(uint32_t setNumber) const;

	VkPipelineLayoutPtr GetVkPipelineLayout() const { return m_pipelineLayout; }

	VkShaderStageFlags GetPushConstantShaderStageFlags() const { return m_pushConstantShaderStageFlags; }

private:
	Result createApiObjects(const PipelineInterfaceCreateInfo& createInfo) final;
	void destroyApiObjects() final;

	VkPipelineLayoutPtr m_pipelineLayout;
	VkShaderStageFlags  m_pushConstantShaderStageFlags = 0;
	bool m_hasConsecutiveSetNumbers = false;
	std::vector<uint32_t> m_setNumbers = {};
};

#pragma endregion

//=============================================================================
#pragma region [ Command ]

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

	Filter filter = Filter::Linear;
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
	float4                 RTVClearValues[MaxRenderTargets] = { { 0.0f, 0.0f, 0.0f, 0.0f } };
	DepthStencilClearValue DSVClearValue = { 1.0f, 0xFF };
};

// RenderingInfo is used to start dynamic render passes.
struct RenderingInfo
{
	BeginRenderingFlags flags;
	Rect                renderArea = {};

	uint32_t                renderTargetCount = 0;
	RenderTargetView* pRenderTargetViews[MaxRenderTargets] = {};
	DepthStencilView* pDepthStencilView = nullptr;

	float4                 RTVClearValues[MaxRenderTargets] = { { 0.0f, 0.0f, 0.0f, 0.0f } };
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
	void destroyApiObjects() final;

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
		uint32_t                 resourceDescriptorCount = DefaultResourceDescriptorCount;
		uint32_t                 samplerDescriptorCount = DefaultSampleDescriptorCount;
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
	void ClearRenderTarget(Image* pImage, const float4& clearValue);
	void ClearDepthStencil(
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

//=============================================================================
#pragma region [ Queue ]

struct SubmitInfo final
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
	};
}

class Queue final : public DeviceObject<internal::QueueCreateInfo>
{
public:
	CommandType GetCommandType() const { return m_createInfo.commandType; }
	DeviceQueuePtr GetQueue() { return m_queue; }
	uint32_t GetQueueFamilyIndex() const { return m_queue->QueueFamily; }

	Result WaitIdle();

	Result Submit(const SubmitInfo* pSubmitInfo);

	// Timeline semaphore functions
	Result QueueWait(Semaphore* pSemaphore, uint64_t value);
	Result QueueSignal(Semaphore* pSemaphore, uint64_t value);

	// GPU timestamp frequency counter in ticks per second
	Result GetTimestampFrequency(uint64_t* pFrequency) const;

	Result CreateCommandBuffer(
		CommandBuffer** ppCommandBuffer,
		uint32_t              resourceDescriptorCount = DefaultResourceDescriptorCount,
		uint32_t              samplerDescriptorCount = DefaultSampleDescriptorCount);
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

	DeviceQueuePtr m_queue;
	VkCommandPoolPtr mTransientPool;
	std::mutex       mQueueMutex;
	std::mutex       mCommandPoolMutex;
};

#pragma endregion

//=============================================================================
#pragma region [ FullscreenQuad ]

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
	} sets[MaxBoundDescriptorSets] = {};

	uint32_t     renderTargetCount = 0;
	Format renderTargetFormats[MaxRenderTargets] = { Format::Undefined };
	Format depthStencilFormat = Format::Undefined;
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
	void destroyApiObjects() final;

private:
	PipelineInterfacePtr mPipelineInterface;
	GraphicsPipelinePtr  mPipeline;
};

#pragma endregion

//=============================================================================
#pragma region [ Scope ]

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

//=============================================================================
#pragma region [ Text Draw ]

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

struct TextDrawCreateInfo
{
	TextureFont* pFont = nullptr;
	uint32_t              maxTextLength = 4096;
	ShaderStageInfo VS = {}; // Use basic/shaders/TextDraw.hlsl (vsmain) for now
	ShaderStageInfo PS = {}; // Use basic/shaders/TextDraw.hlsl (psmain) for now
	BlendMode       blendMode = BLEND_MODE_PREMULT_ALPHA;
	Format          renderTargetFormat = Format::Undefined;
	Format          depthStencilFormat = Format::Undefined;
};

class TextDraw : public DeviceObject<TextDrawCreateInfo>
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

} // namespace vkr