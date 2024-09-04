#pragma once

#include "RenderCore.h"

class VulkanQueue;
class VulkanPipelineInterface;
class VulkanDescriptorSet;


using VulkanPipelineInterfacePtr = std::shared_ptr<VulkanPipelineInterface>;

#pragma region VulkanFence

struct FenceCreateInfo final
{
	bool signaled = false;
};

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

struct SemaphoreCreateInfo final
{
	SemaphoreType semaphoreType = SEMAPHORE_TYPE_BINARY;
	uint64_t      initialValue = 0; // Timeline semaphore only
};

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

