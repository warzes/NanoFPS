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

	// Convenience functions
	ImageViewType GuessImageViewType(bool isCube = false) const;

	Result MapMemory(uint64_t offset, void** ppMappedAddress);
	void   UnmapMemory();

private:
	Result create(const ImageCreateInfo& pCreateInfo) final;
};

namespace internal
{
	class ImageResourceView
	{
	public:
		ImageResourceView() {}
		virtual ~ImageResourceView() {}
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

class Sampler : public DeviceObject<SamplerCreateInfo>
{
public:
	Sampler() {}
	virtual ~Sampler() {}
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

class RenderTargetView : public DeviceObject<RenderTargetViewCreateInfo>, public ImageView
{
public:
	ImagePtr          GetImage() const { return m_createInfo.pImage; }
	Format            GetFormat() const { return m_createInfo.format; }
	SampleCount       GetSampleCount() const { return GetImage()->GetSampleCount(); }
	uint32_t          GetMipLevel() const { return m_createInfo.mipLevel; }
	uint32_t         GetArrayLayer() const { return m_createInfo.arrayLayer; }
	AttachmentLoadOp  GetLoadOp() const { return m_createInfo.loadOp; }
	AttachmentStoreOp GetStoreOp() const { return m_createInfo.storeOp; }
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

class SamplerYcbcrConversion : public DeviceObject<SamplerYcbcrConversionCreateInfo>
{
public:
	SamplerYcbcrConversion() {}
	virtual ~SamplerYcbcrConversion() {}
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
		uint32_t          width,
		uint32_t          height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1,
		MemoryUsage memoryUsage = MEMORY_USAGE_GPU_ONLY);

	// Returns a create info for sampled image and depth stencil target
	static ImageCreateInfo DepthStencilTarget(
		uint32_t          width,
		uint32_t          height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1);

	// Returns a create info for sampled image and render target
	static ImageCreateInfo RenderTarget2D(
		uint32_t          width,
		uint32_t          height,
		Format      format,
		SampleCount sampleCount = SAMPLE_COUNT_1);
};

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

struct CommandPoolCreateInfo final
{
	DeviceQueuePtr queue{ nullptr };
};

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

