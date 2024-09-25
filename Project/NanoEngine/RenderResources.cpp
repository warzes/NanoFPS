#include "stdafx.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderDevice.h"

namespace vkr {

#pragma region VulkanFence

Result Fence::Wait(uint64_t timeout)
{
	VkResult vkres = vkWaitForFences(GetDevice()->GetVkDevice(), 1, m_fence, VK_TRUE, timeout);
	if (vkres != VK_SUCCESS) return ERROR_API_FAILURE;
	return SUCCESS;
}

Result Fence::Reset()
{
	VkResult vkres = vkResetFences(GetDevice()->GetVkDevice(), 1, m_fence);
	if (vkres != VK_SUCCESS) return ERROR_API_FAILURE;
	return SUCCESS;
}

Result Fence::WaitAndReset(uint64_t timeout)
{
	Result ppxres = Wait(timeout);
	if (Failed(ppxres)) return ppxres;
	ppxres = Reset();
	if (Failed(ppxres)) return ppxres;
	return SUCCESS;
}

Result Fence::createApiObjects(const FenceCreateInfo& createInfo)
{
	VkFenceCreateInfo vkci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	vkci.flags = createInfo.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkResult vkres = vkCreateFence(GetDevice()->GetVkDevice(), &vkci, nullptr, &m_fence);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vkCreateFence failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Fence::destroyApiObjects()
{
	if (m_fence)
	{
		vkDestroyFence(GetDevice()->GetVkDevice(), m_fence, nullptr);
		m_fence.Reset();
	}
}

#pragma endregion

#pragma region VulkanSemaphore

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

Result Semaphore::Wait(uint64_t value, uint64_t timeout) const
{
	if (GetSemaphoreType() != SemaphoreType::Timeline)
	{
		Fatal(REQUIRES_TIMELINE_MSG);
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	auto ppxres = timelineWait(value, timeout);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Semaphore::Signal(uint64_t value) const
{
	if (GetSemaphoreType() != SemaphoreType::Timeline)
	{
		Fatal(REQUIRES_TIMELINE_MSG);
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	auto ppxres = timelineSignal(value);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

uint64_t Semaphore::GetCounterValue() const
{
	if (GetSemaphoreType() != SemaphoreType::Timeline)
	{
		Fatal(REQUIRES_TIMELINE_MSG);
		return UINT64_MAX;
	}

	uint64_t value = timelineCounterValue();
	return value;
}

Result Semaphore::createApiObjects(const SemaphoreCreateInfo& createInfo)
{
	VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	timelineCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue              = createInfo.initialValue;

	VkSemaphoreCreateInfo vkci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkci.pNext = (createInfo.semaphoreType == SemaphoreType::Timeline) ? &timelineCreateInfo : nullptr;

	VkResult vkres = vkCreateSemaphore(GetDevice()->GetVkDevice(), &vkci, nullptr, &m_semaphore);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vkCreateSemaphore failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Semaphore::destroyApiObjects()
{
	if (m_semaphore)
	{
		vkDestroySemaphore(GetDevice()->GetVkDevice(), m_semaphore, nullptr);
		m_semaphore.Reset();
	}
}

Result Semaphore::timelineWait(uint64_t value, uint64_t timeout) const
{
	VkSemaphore semaphoreHandle = m_semaphore;

	VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	waitInfo.semaphoreCount = 1;
	waitInfo.pSemaphores = &semaphoreHandle;
	waitInfo.pValues = &value;

	VkResult vkres = vkWaitSemaphores(GetDevice()->GetVkDevice(), &waitInfo, timeout);
	if (vkres != VK_SUCCESS) return ERROR_API_FAILURE;

	return SUCCESS;
}

Result Semaphore::timelineSignal(uint64_t value) const
{
	// See header for explanation
	if (value > m_timelineSignaledValue)
	{
		VkSemaphore semaphoreHandle = m_semaphore;

		VkSemaphoreSignalInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
		signalInfo.semaphore = m_semaphore;
		signalInfo.value = value;

		VkResult vkres = vkSignalSemaphore(GetDevice()->GetVkDevice(), &signalInfo);
		if (vkres != VK_SUCCESS) return ERROR_API_FAILURE;

		m_timelineSignaledValue = value;
	}

	return SUCCESS;
}

uint64_t Semaphore::timelineCounterValue() const
{
	uint64_t value = UINT64_MAX;
	VkResult vkres = vkGetSemaphoreCounterValue(GetDevice()->GetVkDevice(), m_semaphore, &value);
	// Not clear if value is written to upon failure so prefer safety.
	return (vkres == VK_SUCCESS) ? value : UINT64_MAX;
}

#pragma endregion

#pragma region Query

VkBufferPtr Query::GetReadBackBuffer() const
{
	return m_buffer->GetVkBuffer();
}

void Query::Reset(uint32_t firstQuery, uint32_t queryCount)
{
	vkResetQueryPool(GetDevice()->GetVkDevice(), m_queryPool, firstQuery, queryCount);
}

Result Query::GetData(void* dstData, uint64_t dstDataSize)
{
	void* pMappedAddress = 0;
	Result ppxres = m_buffer->MapMemory(0, &pMappedAddress);
	if (Failed(ppxres)) return ppxres;

	size_t copySize = std::min<size_t>(dstDataSize, m_buffer->GetSize());
	memcpy(dstData, pMappedAddress, copySize);

	m_buffer->UnmapMemory();

	return SUCCESS;
}

Result Query::createApiObjects(const QueryCreateInfo& createInfo)
{
	if (createInfo.type == QueryType::Undefined) return ERROR_GRFX_INVALID_QUERY_TYPE;
	if (createInfo.count == 0) return ERROR_GRFX_INVALID_QUERY_COUNT;

	VkQueryPoolCreateInfo vkci = { VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
	vkci.flags                 = 0;
	vkci.queryType             = ToVkEnum(createInfo.type);
	vkci.queryCount            = createInfo.count;
	vkci.pipelineStatistics    = 0;

	m_type = vkci.queryType;
	m_multiplier = 1;

	if (vkci.queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
	{
		vkci.pipelineStatistics = 
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT |
			VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT |
			VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
		std::bitset<32> flagbit(vkci.pipelineStatistics);
		m_multiplier = (uint32_t)flagbit.count();
	}

	VkResult vkres = vkCreateQueryPool(GetDevice()->GetVkDevice(), &vkci, nullptr, &m_queryPool);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vkCreateQueryPool failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	BufferCreateInfo bufferInfo        = {};
	bufferInfo.size                    = vkci.queryCount * getQueryTypeSize(m_type, m_multiplier);
	bufferInfo.structuredElementStride = getQueryTypeSize(m_type, m_multiplier);
	bufferInfo.usageFlags              = BUFFER_USAGE_TRANSFER_DST;
	bufferInfo.memoryUsage             = MemoryUsage::GPUToCPU;
	bufferInfo.initialState            = ResourceState::CopyDst;
	bufferInfo.ownership               = Ownership::Reference;

	// Create buffer
	Result ppxres = GetDevice()->CreateBuffer(bufferInfo, &m_buffer);
	if (Failed(ppxres))
	{
		Fatal("Create buffer in Query failed: " + ToString(vkres));
		return ppxres;
	}

	return SUCCESS;
}

void Query::destroyApiObjects()
{
	if (m_queryPool)
	{
		vkDestroyQueryPool(GetDevice()->GetVkDevice(), m_queryPool, nullptr);
		m_queryPool.Reset();
	}

	if (m_buffer) m_buffer.Reset();
}

uint32_t Query::getQueryTypeSize(VkQueryType type, uint32_t multiplier) const
{
	uint32_t result = 0;
	switch (type)
	{
	case VK_QUERY_TYPE_OCCLUSION:
	case VK_QUERY_TYPE_TIMESTAMP:
		// this need VK_QUERY_RESULT_64_BIT to be set
		result = (uint32_t)sizeof(uint64_t);
		break;
	case VK_QUERY_TYPE_PIPELINE_STATISTICS:
		// this need VK_QUERY_RESULT_64_BIT to be set
		result = (uint32_t)sizeof(uint64_t) * multiplier;
		break;
	default:
		Fatal("Unsupported query type");
		break;
	}
	return result;
}

#pragma endregion

#pragma region Buffer

Result Buffer::MapMemory(uint64_t offset, void** mappedAddress)
{
	if (IsNull(mappedAddress)) return ERROR_UNEXPECTED_NULL_ARGUMENT;

	VkResult vkres = vmaMapMemory(GetDevice()->GetVmaAllocator(), m_allocation, mappedAddress);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vmaMapMemory failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Buffer::UnmapMemory()
{
	vmaUnmapMemory(GetDevice()->GetVmaAllocator(), m_allocation);
}

Result Buffer::CopyFromSource(uint32_t dataSize, const void* srcData)
{
	if (dataSize > GetSize()) return ERROR_LIMIT_EXCEEDED;

	// Map
	void* bufferAddress = nullptr;
	Result ppxres = MapMemory(0, &bufferAddress);
	if (Failed(ppxres)) return ppxres;

	// Copy
	std::memcpy(bufferAddress, srcData, dataSize);

	// Unmap
	UnmapMemory();

	return SUCCESS;
}

Result Buffer::CopyToDest(uint32_t dataSize, void* destData)
{
	if (dataSize > GetSize()) return ERROR_LIMIT_EXCEEDED;

	// Map
	void* bufferAddress = nullptr;
	Result ppxres = MapMemory(0, &bufferAddress);
	if (Failed(ppxres))  return ppxres;

	// Copy
	std::memcpy(destData, bufferAddress, dataSize);

	// Unmap
	UnmapMemory();

	return SUCCESS;
}

Result Buffer::createApiObjects(const BufferCreateInfo& createInfo)
{
#ifndef DISABLE_MINIMUM_BUFFER_SIZE_CHECK
	// Constant/uniform buffers need to be at least CONSTANT_BUFFER_ALIGNMENT in size
	if (createInfo.usageFlags.bits.uniformBuffer && (createInfo.size < CONSTANT_BUFFER_ALIGNMENT))
	{
		Fatal("constant/uniform buffer sizes must be at least CONSTANT_BUFFER_ALIGNMENT (" + std::to_string(CONSTANT_BUFFER_ALIGNMENT) + ")");
		return ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
	}

	// Storage/structured buffers need to be at least STORAGE_BUFFER_ALIGNMENT in size
	if (createInfo.usageFlags.bits.uniformBuffer && (createInfo.size < STUCTURED_BUFFER_ALIGNMENT))
	{
		Fatal("storage/structured buffer sizes must be at least STUCTURED_BUFFER_ALIGNMENT (" + std::to_string(STUCTURED_BUFFER_ALIGNMENT) + ")");
		return ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
	}
#endif

	VkDeviceSize alignedSize = static_cast<VkDeviceSize>(createInfo.size);
	if (createInfo.usageFlags.bits.uniformBuffer)
		alignedSize = RoundUp<VkDeviceSize>(createInfo.size, UNIFORM_BUFFER_ALIGNMENT);

	VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size               = alignedSize;
	bufferInfo.usage              = ToVkBufferUsageFlags(createInfo.usageFlags);
	bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

	VkAllocationCallbacks* allocator = nullptr;
	VkResult vkres = vkCreateBuffer(GetDevice()->GetVkDevice(), &bufferInfo, allocator, &m_buffer);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vkCreateBuffer failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	// Allocate memory
	{
		VmaMemoryUsage memoryUsage = ToVmaMemoryUsage(createInfo.memoryUsage);
		if (memoryUsage == VMA_MEMORY_USAGE_UNKNOWN)
		{
			Fatal("unknown memory usage");
			return ERROR_API_FAILURE;
		}

		VmaAllocationCreateFlags createFlags = 0;
		if ((memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY) || (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY))
		{
			createFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
		}

		VmaAllocationCreateInfo vmaAllocci = {};
		vmaAllocci.flags                   = createFlags;
		vmaAllocci.usage                   = memoryUsage;
		vmaAllocci.requiredFlags           = 0;
		vmaAllocci.preferredFlags          = 0;
		vmaAllocci.memoryTypeBits          = 0;
		vmaAllocci.pool                    = VK_NULL_HANDLE;
		vmaAllocci.pUserData               = nullptr;

		VkResult vkres = vmaAllocateMemoryForBuffer(GetDevice()->GetVmaAllocator(), m_buffer, &vmaAllocci, &m_allocation, &m_allocationInfo);
		if (vkres != VK_SUCCESS)
		{
			Fatal("vmaAllocateMemoryForBuffer failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	// Bind memory
	{
		VkResult vkres = vmaBindBufferMemory(GetDevice()->GetVmaAllocator(), m_allocation, m_buffer);
		if (vkres != VK_SUCCESS)
		{
			Fatal("vmaBindBufferMemory failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

void Buffer::destroyApiObjects()
{
	if (m_allocation)
	{
		vmaFreeMemory(GetDevice()->GetVmaAllocator(), m_allocation);
		m_allocation.Reset();
		m_allocationInfo = {};
	}

	if (m_buffer)
	{
		vkDestroyBuffer(GetDevice()->GetVkDevice(), m_buffer, nullptr);
		m_buffer.Reset();
	}
}

#pragma endregion

#pragma region Image

ImageCreateInfo ImageCreateInfo::SampledImage2D(
	uint32_t    width,
	uint32_t    height,
	Format      format,
	SampleCount sampleCount,
	MemoryUsage memoryUsage)
{
	ImageCreateInfo ci = {};
	ci.type = ImageType::Image2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.memoryUsage = memoryUsage;
	ci.initialState = ResourceState::ShaderResource;
	ci.pApiObject = nullptr;
	return ci;
}

ImageCreateInfo ImageCreateInfo::DepthStencilTarget(
	uint32_t    width,
	uint32_t    height,
	Format      format,
	SampleCount sampleCount)
{
	ImageCreateInfo ci = {};
	ci.type = ImageType::Image2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.depthStencilAttachment = true;
	ci.memoryUsage = MemoryUsage::GPUOnly;
	ci.initialState = ResourceState::DepthStencilWrite;
	ci.pApiObject = nullptr;
	return ci;
}

ImageCreateInfo ImageCreateInfo::RenderTarget2D(
	uint32_t    width,
	uint32_t    height,
	Format      format,
	SampleCount sampleCount)
{
	ImageCreateInfo ci = {};
	ci.type = ImageType::Image2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.colorAttachment = true;
	ci.memoryUsage = MemoryUsage::GPUOnly;
	ci.pApiObject = nullptr;
	return ci;
}

Result Image::MapMemory(uint64_t offset, void** ppMappedAddress)
{
	if (IsNull(ppMappedAddress)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	VkResult vkres = vmaMapMemory(
		GetDevice()->GetVmaAllocator(),
		mAllocation,
		ppMappedAddress);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vmaMapMemory failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Image::UnmapMemory()
{
	vmaUnmapMemory(GetDevice()->GetVmaAllocator(), mAllocation);
}

Result Image::createApiObjects(const ImageCreateInfo& pCreateInfo)
{
	if (IsNull(pCreateInfo.pApiObject))
	{
		// Create image
		{
			VkExtent3D extent = {};
			extent.width = pCreateInfo.width;
			extent.height = pCreateInfo.height;
			extent.depth = pCreateInfo.depth;

			VkImageCreateFlags createFlags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
			if (pCreateInfo.type == ImageType::Cube)
			{
				createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
			}

			if (pCreateInfo.createFlags.bits.subsampledFormat)
			{
				createFlags |= VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;
			}
			auto queueIndices = GetDevice()->GetAllQueueFamilyIndices();

			VkImageCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
			vkci.flags = createFlags;
			vkci.imageType = ToVkImageType(pCreateInfo.type);
			vkci.format = ToVkEnum(pCreateInfo.format);
			vkci.extent = extent;
			vkci.mipLevels = pCreateInfo.mipLevelCount;
			vkci.arrayLayers = pCreateInfo.arrayLayerCount;
			vkci.samples = ToVkSampleCount(pCreateInfo.sampleCount);
			vkci.tiling = pCreateInfo.memoryUsage == MemoryUsage::GPUToCPU ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
			vkci.usage = ToVkImageUsageFlags(pCreateInfo.usageFlags);
			vkci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			if (pCreateInfo.concurrentMultiQueueUsage)
			{
				vkci.sharingMode = VK_SHARING_MODE_CONCURRENT;
				vkci.queueFamilyIndexCount = 3;
				vkci.pQueueFamilyIndices = queueIndices.data();
			}
			else
			{
				vkci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
				vkci.queueFamilyIndexCount = 0;
				vkci.pQueueFamilyIndices = nullptr;
			}

			VkAllocationCallbacks* pAllocator = nullptr;

			VkResult vkres = vkCreateImage(GetDevice()->GetVkDevice(), &vkci, pAllocator, &mImage);
			if (vkres != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vkCreateImage failed: " + ToString(vkres));
				return ERROR_API_FAILURE;
			}
		}

		// Allocate memory
		{
			VmaMemoryUsage memoryUsage = ToVmaMemoryUsage(pCreateInfo.memoryUsage);
			if (memoryUsage == VMA_MEMORY_USAGE_UNKNOWN)
			{
				ASSERT_MSG(false, "unknown memory usage");
				return ERROR_API_FAILURE;
			}

			VmaAllocationCreateFlags createFlags = 0;

			if ((memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY) || (memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU)) {
				createFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
			}

			VmaAllocationCreateInfo vma_alloc_ci = {};
			vma_alloc_ci.flags = createFlags;
			vma_alloc_ci.usage = memoryUsage;
			vma_alloc_ci.requiredFlags = 0;
			vma_alloc_ci.preferredFlags = 0;
			vma_alloc_ci.memoryTypeBits = 0;
			vma_alloc_ci.pool = VK_NULL_HANDLE;
			vma_alloc_ci.pUserData = nullptr;

			VkResult vkres = vmaAllocateMemoryForImage(
				GetDevice()->GetVmaAllocator(),
				mImage,
				&vma_alloc_ci,
				&mAllocation,
				&mAllocationInfo);
			if (vkres != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vmaAllocateMemoryForImage failed: " + ToString(vkres));
				return ERROR_API_FAILURE;
			}
		}

		// Bind memory
		{
			VkResult vkres = vmaBindImageMemory(
				GetDevice()->GetVmaAllocator(),
				mAllocation,
				mImage);
			if (vkres != VK_SUCCESS) {
				ASSERT_MSG(false, "vmaBindImageMemory failed: " + ToString(vkres));
				return ERROR_API_FAILURE;
			}
		}
	}
	else
	{
		mImage = reinterpret_cast<VkImage>(pCreateInfo.pApiObject);
	}

	mVkFormat = ToVkEnum(pCreateInfo.format);
	mImageAspect = DetermineAspectMask(mVkFormat);

	if ((pCreateInfo.initialState != ResourceState::Undefined) && IsNull(pCreateInfo.pApiObject))
	{
		QueuePtr grfxQueue = GetDevice()->GetAnyAvailableQueue();
		if (!grfxQueue)
		{
			return ERROR_FAILED;
		}

		// Determine pipeline stage and layout from the initial state
		VkPipelineStageFlags pipelineStage = 0;
		VkAccessFlags        accessMask = 0;
		VkImageLayout        layout = VK_IMAGE_LAYOUT_UNDEFINED;
		Result               ppxres = ToVkBarrierDst(
			pCreateInfo.initialState,
			grfxQueue->GetCommandType(),
			GetDevice()->GetDeviceFeatures(),
			pipelineStage,
			accessMask,
			layout);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "couldn't determine pipeline stage and layout from initial state");
			return ppxres;
		}

		Queue* pQueue = grfxQueue.Get();

		VkResult vkres = pQueue->TransitionImageLayout(
			mImage,                       // image
			mImageAspect,                 // aspectMask
			0,                            // baseMipLevel
			pCreateInfo.mipLevelCount,   // levelCount
			0,                            // baseArrayLayer
			pCreateInfo.arrayLayerCount, // layerCount
			VK_IMAGE_LAYOUT_UNDEFINED,    // oldLayout
			layout,                       // newLayout
			pipelineStage);               // newPipelineStage)
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vk::Queue::TransitionImageLayout failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

void Image::destroyApiObjects()
{
	// Don't destroy image unless we created it
	if (!IsNull(m_createInfo.pApiObject))
	{
		return;
	}

	if (mAllocation)
	{
		vmaFreeMemory(GetDevice()->GetVmaAllocator(), mAllocation);
		mAllocation.Reset();

		mAllocationInfo = {};
	}

	if (mImage)
	{
		vkDestroyImage(GetDevice()->GetVkDevice(), mImage, nullptr);
		mImage.Reset();
	}
}

Result Image::create(const ImageCreateInfo& pCreateInfo)
{
	if ((pCreateInfo.type == ImageType::Cube) && (pCreateInfo.arrayLayerCount != 6))
	{
		ASSERT_MSG(false, "arrayLayerCount must be 6 if type is ImageType::Cube");
		return ERROR_INVALID_CREATE_ARGUMENT;
	}

	Result ppxres = DeviceObject<ImageCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

ImageViewType Image::GuessImageViewType(bool isCube) const
{
	const uint32_t arrayLayerCount = GetArrayLayerCount();

	if (isCube)
	{
		return (arrayLayerCount > 0) ? IMAGE_VIEW_TYPE_CUBE_ARRAY : IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		switch (m_createInfo.type) {
		default: break;
		case ImageType::Image1D: return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_1D_ARRAY : IMAGE_VIEW_TYPE_1D; break;
		case ImageType::Image2D: return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_2D_ARRAY : IMAGE_VIEW_TYPE_2D; break;
		case ImageType::Cube: return (arrayLayerCount > 6) ? IMAGE_VIEW_TYPE_CUBE_ARRAY : IMAGE_VIEW_TYPE_CUBE; break;
		}
	}

	return IMAGE_VIEW_TYPE_UNDEFINED;
}

Result Sampler::createApiObjects(const SamplerCreateInfo& createInfo)
{
	VkSamplerCreateInfo vkci     = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	vkci.flags                   = 0;
	if (createInfo.createFlags.bits.subsampledFormat) vkci.flags |= VK_SAMPLER_CREATE_SUBSAMPLED_BIT_EXT;
	vkci.magFilter               = ToVkEnum(createInfo.magFilter);
	vkci.minFilter               = ToVkEnum(createInfo.minFilter);
	vkci.mipmapMode              = ToVkEnum(createInfo.mipmapMode);
	vkci.addressModeU            = ToVkEnum(createInfo.addressModeU);
	vkci.addressModeV            = ToVkEnum(createInfo.addressModeV);
	vkci.addressModeW            = ToVkEnum(createInfo.addressModeW);
	vkci.mipLodBias              = createInfo.mipLodBias;
	vkci.anisotropyEnable        = createInfo.anisotropyEnable ? VK_TRUE : VK_FALSE;
	vkci.maxAnisotropy           = createInfo.maxAnisotropy;
	vkci.compareEnable           = createInfo.compareEnable ? VK_TRUE : VK_FALSE;
	vkci.compareOp               = ToVkEnum(createInfo.compareOp);
	vkci.minLod                  = createInfo.minLod;
	vkci.maxLod                  = createInfo.maxLod;
	vkci.borderColor             = ToVkEnum(createInfo.borderColor);
	vkci.unnormalizedCoordinates = VK_FALSE;

	VkSamplerReductionModeCreateInfoEXT createInfoReduction = { VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT };
	createInfoReduction.reductionMode = ToVkEnum(createInfo.reductionMode);

	VkSamplerYcbcrConversionInfo conversionInfo{ VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO };
	if (createInfo.YcbcrConversion != nullptr)
	{
		conversionInfo.conversion = createInfo.YcbcrConversion->GetVkSamplerYcbcrConversion().Get();
		createInfoReduction.pNext = &conversionInfo;
	}

	vkci.pNext = &createInfoReduction;

	VkResult vkres = vkCreateSampler(GetDevice()->GetVkDevice(), &vkci, nullptr, &m_sampler);
	if (vkres != VK_SUCCESS)
	{
		Fatal("vkCreateSampler failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Sampler::destroyApiObjects()
{
	if (m_sampler)
	{
		vkDestroySampler(GetDevice()->GetVkDevice(), m_sampler, nullptr);
		m_sampler.Reset();
	}
}

DepthStencilViewCreateInfo DepthStencilViewCreateInfo::GuessFromImage(Image* pImage)
{
	DepthStencilViewCreateInfo ci = {};
	ci.pImage = pImage;
	ci.imageViewType = pImage->GuessImageViewType();
	ci.format = pImage->GetFormat();
	ci.mipLevel = 0;
	ci.mipLevelCount = 1;
	ci.arrayLayer = 0;
	ci.arrayLayerCount = 1;
	ci.components = {};
	ci.depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	ci.depthStoreOp = ATTACHMENT_STORE_OP_STORE;
	ci.stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
	ci.stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
	ci.ownership = Ownership::Reference;
	return ci;
}

Result DepthStencilView::createApiObjects(const DepthStencilViewCreateInfo& pCreateInfo)
{
	VkImageViewCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.flags = 0;
	vkci.image = pCreateInfo.pImage->GetVkImage();
	vkci.viewType = ToVkImageViewType(pCreateInfo.imageViewType);
	vkci.format = ToVkEnum(pCreateInfo.format);
	vkci.components = ToVkComponentMapping(pCreateInfo.components);
	vkci.subresourceRange.aspectMask = pCreateInfo.pImage->GetVkImageAspectFlags();
	vkci.subresourceRange.baseMipLevel = pCreateInfo.mipLevel;
	vkci.subresourceRange.levelCount = pCreateInfo.mipLevelCount;
	vkci.subresourceRange.baseArrayLayer = pCreateInfo.arrayLayer;
	vkci.subresourceRange.layerCount = pCreateInfo.arrayLayerCount;

	VkResult vkres = vkCreateImageView(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mImageView);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateImageView(DepthStencilView) failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	std::unique_ptr<internal::ImageResourceView> resourceView(new internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
	if (!resourceView)
	{
		ASSERT_MSG(false, "vk::internal::ImageResourceView(DepthStencilView) allocation failed");
		return ERROR_ALLOCATION_FAILED;
	}
	setResourceView(std::move(resourceView));

	return SUCCESS;
}

void DepthStencilView::destroyApiObjects()
{
	if (mImageView) 
	{
		vkDestroyImageView(GetDevice()->GetVkDevice(), mImageView, nullptr);
		mImageView.Reset();
	}
}

RenderTargetViewCreateInfo RenderTargetViewCreateInfo::GuessFromImage(Image* pImage)
{
	RenderTargetViewCreateInfo ci = {};
	ci.pImage = pImage;
	ci.imageViewType = pImage->GuessImageViewType();
	ci.format = pImage->GetFormat();
	ci.mipLevel = 0;
	ci.mipLevelCount = 1;
	ci.arrayLayer = 0;
	ci.arrayLayerCount = 1;
	ci.components = {};
	ci.loadOp = ATTACHMENT_LOAD_OP_LOAD;
	ci.storeOp = ATTACHMENT_STORE_OP_STORE;
	ci.ownership = Ownership::Reference;
	return ci;
}

Result RenderTargetView::createApiObjects(const RenderTargetViewCreateInfo& pCreateInfo)
{
	VkImageViewCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.flags = 0;
	vkci.image = pCreateInfo.pImage->GetVkImage();
	vkci.viewType = ToVkImageViewType(pCreateInfo.imageViewType);
	vkci.format = ToVkEnum(pCreateInfo.format);
	vkci.components = ToVkComponentMapping(pCreateInfo.components);
	vkci.subresourceRange.aspectMask = pCreateInfo.pImage->GetVkImageAspectFlags();
	vkci.subresourceRange.baseMipLevel = pCreateInfo.mipLevel;
	vkci.subresourceRange.levelCount = pCreateInfo.mipLevelCount;
	vkci.subresourceRange.baseArrayLayer = pCreateInfo.arrayLayer;
	vkci.subresourceRange.layerCount = pCreateInfo.arrayLayerCount;

	VkResult vkres = vkCreateImageView(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mImageView);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreateImageView(RenderTargetView) failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	std::unique_ptr<internal::ImageResourceView> resourceView(new internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
	if (!resourceView)
	{
		ASSERT_MSG(false, "vk::internal::ImageResourceView(RenderTargetView) allocation failed");
		return ERROR_ALLOCATION_FAILED;
	}
	setResourceView(std::move(resourceView));

	return SUCCESS;
}

void RenderTargetView::destroyApiObjects()
{
	if (mImageView)
	{
		vkDestroyImageView(GetDevice()->GetVkDevice(), mImageView, nullptr);
		mImageView.Reset();
	}
}

SampledImageViewCreateInfo SampledImageViewCreateInfo::GuessFromImage(Image* pImage)
{
	SampledImageViewCreateInfo ci = {};
	ci.pImage = pImage;
	ci.imageViewType = pImage->GuessImageViewType();
	ci.format = pImage->GetFormat();
	ci.mipLevel = 0;
	ci.mipLevelCount = pImage->GetMipLevelCount();
	ci.arrayLayer = 0;
	ci.arrayLayerCount = pImage->GetArrayLayerCount();
	ci.components = {};
	ci.ownership = Ownership::Reference;
	return ci;
}

Result SampledImageView::createApiObjects(const SampledImageViewCreateInfo& pCreateInfo)
{
	VkImageViewCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.flags = 0;
	vkci.image = pCreateInfo.pImage->GetVkImage();
	vkci.viewType = ToVkImageViewType(pCreateInfo.imageViewType);
	vkci.format = ToVkEnum(pCreateInfo.format);
	vkci.components = ToVkComponentMapping(pCreateInfo.components);
	vkci.subresourceRange.aspectMask = pCreateInfo.pImage->GetVkImageAspectFlags();
	vkci.subresourceRange.baseMipLevel = pCreateInfo.mipLevel;
	vkci.subresourceRange.levelCount = pCreateInfo.mipLevelCount;
	vkci.subresourceRange.baseArrayLayer = pCreateInfo.arrayLayer;
	vkci.subresourceRange.layerCount = pCreateInfo.arrayLayerCount;

	VkSamplerYcbcrConversionInfo conversionInfo{ VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO };
	if (pCreateInfo.pYcbcrConversion != nullptr)
	{
		conversionInfo.conversion = pCreateInfo.pYcbcrConversion->GetVkSamplerYcbcrConversion().Get();
		vkci.pNext = &conversionInfo;
	}

	VkResult vkres = vkCreateImageView(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mImageView);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateImageView(SampledImageView) failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	std::unique_ptr<internal::ImageResourceView> resourceView(new internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
	if (!resourceView)
	{
		ASSERT_MSG(false, "internal::ImageResourceView(SampledImageView) allocation failed");
		return ERROR_ALLOCATION_FAILED;
	}
	setResourceView(std::move(resourceView));

	return SUCCESS;
}

void SampledImageView::destroyApiObjects()
{
	if (mImageView)
	{
		vkDestroyImageView( GetDevice()->GetVkDevice(), mImageView, nullptr);
		mImageView.Reset();
	}
}

Result SamplerYcbcrConversion::createApiObjects(const SamplerYcbcrConversionCreateInfo& pCreateInfo)
{
	VkSamplerYcbcrConversionCreateInfo vkci{ VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO };
	vkci.format = ToVkEnum(pCreateInfo.format);
	vkci.ycbcrModel = ToVkYcbcrModelConversion(pCreateInfo.ycbcrModel);
	vkci.ycbcrRange = ToVkYcbcrRange(pCreateInfo.ycbcrRange);
	vkci.components = ToVkComponentMapping(pCreateInfo.components);
	vkci.xChromaOffset = ToVkChromaLocation(pCreateInfo.xChromaOffset);
	vkci.yChromaOffset = ToVkChromaLocation(pCreateInfo.yChromaOffset);
	vkci.chromaFilter = ToVkEnum(pCreateInfo.filter);
	vkci.forceExplicitReconstruction = pCreateInfo.forceExplicitReconstruction ? VK_TRUE : VK_FALSE;

	VkResult vkres = vkCreateSamplerYcbcrConversion(GetDevice()->GetVkDevice(), &vkci, nullptr, &mSamplerYcbcrConversion);
	//Print("Created sampler ycbcr conversion: " + mSamplerYcbcrConversion.Get());
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateSamplerYcbcrConversion failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void SamplerYcbcrConversion::destroyApiObjects()
{
	if (mSamplerYcbcrConversion)
	{
		vkDestroySamplerYcbcrConversion(GetDevice()->GetVkDevice(), mSamplerYcbcrConversion, nullptr);
		mSamplerYcbcrConversion.Reset();
	}
}

StorageImageViewCreateInfo StorageImageViewCreateInfo::GuessFromImage(Image* pImage)
{
	StorageImageViewCreateInfo ci = {};
	ci.pImage = pImage;
	ci.imageViewType = pImage->GuessImageViewType();
	ci.format = pImage->GetFormat();
	ci.mipLevel = 0;
	ci.mipLevelCount = pImage->GetMipLevelCount();
	ci.arrayLayer = 0;
	ci.arrayLayerCount = pImage->GetArrayLayerCount();
	ci.components = {};
	ci.ownership = Ownership::Reference;
	return ci;
}

Result StorageImageView::createApiObjects(const StorageImageViewCreateInfo& pCreateInfo)
{
	VkImageViewCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.flags = 0;
	vkci.image = pCreateInfo.pImage->GetVkImage();
	vkci.viewType = ToVkImageViewType(pCreateInfo.imageViewType);
	vkci.format = ToVkEnum(pCreateInfo.format);
	vkci.components = ToVkComponentMapping(pCreateInfo.components);
	vkci.subresourceRange.aspectMask = pCreateInfo.pImage->GetVkImageAspectFlags();
	vkci.subresourceRange.baseMipLevel = pCreateInfo.mipLevel;
	vkci.subresourceRange.levelCount = pCreateInfo.mipLevelCount;
	vkci.subresourceRange.baseArrayLayer = pCreateInfo.arrayLayer;
	vkci.subresourceRange.layerCount = pCreateInfo.arrayLayerCount;

	VkResult vkres = vkCreateImageView(GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mImageView);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateImageView(StorageImageView) failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	std::unique_ptr<internal::ImageResourceView> resourceView(new internal::ImageResourceView(mImageView, VK_IMAGE_LAYOUT_GENERAL));
	if (!resourceView)
	{
		ASSERT_MSG(false, "internal::ImageResourceView(StorageImageView) allocation failed");
		return ERROR_ALLOCATION_FAILED;
	}
	setResourceView(std::move(resourceView));

	return SUCCESS;
}

void StorageImageView::destroyApiObjects()
{
	if (mImageView)
	{
		vkDestroyImageView(GetDevice()->GetVkDevice(), mImageView, nullptr);
		mImageView.Reset();
	}
}

#pragma endregion

#pragma region Texture

Result Texture::create(const TextureCreateInfo& pCreateInfo)
{
	// Copy in case view types and formats are specified:
	//   - if an image is supplied, then the next section
	//     will overwrite all the image related fields with
	//     values from the supplied image.
	//   - if an image is NOT supplied, then nothing gets
	//     overwritten.
	//
	m_createInfo = pCreateInfo;

	if (!IsNull(pCreateInfo.pImage)) 
	{
		m_image = pCreateInfo.pImage;
		m_createInfo.imageType = m_image->GetType();
		m_createInfo.width = m_image->GetWidth();
		m_createInfo.height = m_image->GetHeight();
		m_createInfo.depth = m_image->GetDepth();
		m_createInfo.imageFormat = m_image->GetFormat();
		m_createInfo.sampleCount = m_image->GetSampleCount();
		m_createInfo.mipLevelCount = m_image->GetMipLevelCount();
		m_createInfo.arrayLayerCount = m_image->GetArrayLayerCount();
		m_createInfo.usageFlags = m_image->GetUsageFlags();
		m_createInfo.memoryUsage = m_image->GetMemoryUsage();
		m_createInfo.initialState = m_image->GetInitialState();
		m_createInfo.RTVClearValue = m_image->GetRTVClearValue();
		m_createInfo.DSVClearValue = m_image->GetDSVClearValue();
		m_createInfo.concurrentMultiQueueUsage = m_image->GetConcurrentMultiQueueUsageEnabled();
	}

	// Yes, m_createInfo will self overwrite in the following function call.
	Result ppxres = DeviceObject<TextureCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Texture::createApiObjects(const TextureCreateInfo& pCreateInfo)
{
	// Image
	if (IsNull(pCreateInfo.pImage))
	{
		if (pCreateInfo.usageFlags.bits.colorAttachment && pCreateInfo.usageFlags.bits.depthStencilAttachment)
		{
			ASSERT_MSG(false, "texture cannot be both color attachment and depth stencil attachment");
			return ERROR_INVALID_CREATE_ARGUMENT;
		}

		ImageCreateInfo ci = {};
		ci.type = pCreateInfo.imageType;
		ci.width = pCreateInfo.width;
		ci.height = pCreateInfo.height;
		ci.depth = pCreateInfo.depth;
		ci.format = pCreateInfo.imageFormat;
		ci.sampleCount = pCreateInfo.sampleCount;
		ci.mipLevelCount = pCreateInfo.mipLevelCount;
		ci.arrayLayerCount = pCreateInfo.arrayLayerCount;
		ci.usageFlags = pCreateInfo.usageFlags;
		ci.memoryUsage = pCreateInfo.memoryUsage;
		ci.initialState = pCreateInfo.initialState;
		ci.RTVClearValue = pCreateInfo.RTVClearValue;
		ci.DSVClearValue = pCreateInfo.DSVClearValue;
		ci.pApiObject = nullptr;
		ci.ownership = pCreateInfo.ownership;
		ci.concurrentMultiQueueUsage = pCreateInfo.concurrentMultiQueueUsage;
		ci.createFlags = pCreateInfo.imageCreateFlags;

		Result ppxres = GetDevice()->CreateImage(ci, &m_image);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create image failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.sampled)
	{
		SampledImageViewCreateInfo ci = SampledImageViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.sampledImageViewType != IMAGE_VIEW_TYPE_UNDEFINED) 
		{
			ci.imageViewType = pCreateInfo.sampledImageViewType;
		}
		ci.pYcbcrConversion = pCreateInfo.pSampledImageYcbcrConversion;

		Result ppxres = GetDevice()->CreateSampledImageView(ci, &m_sampledImageView);
		if (Failed(ppxres)) 
		{
			ASSERT_MSG(false, "texture create sampled image view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.colorAttachment)
	{
		RenderTargetViewCreateInfo ci = RenderTargetViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.renderTargetViewFormat != Format::Undefined)
		{
			ci.format = pCreateInfo.renderTargetViewFormat;
		}

		Result ppxres = GetDevice()->CreateRenderTargetView(ci, &m_renderTargetView);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create render target view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.depthStencilAttachment)
	{
		DepthStencilViewCreateInfo ci = DepthStencilViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.depthStencilViewFormat != Format::Undefined)
		{
			ci.format = pCreateInfo.depthStencilViewFormat;
		}

		Result ppxres = GetDevice()->CreateDepthStencilView(ci, &m_depthStencilView);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create depth stencil view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.storage)
	{
		StorageImageViewCreateInfo ci = StorageImageViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.storageImageViewFormat != Format::Undefined)
		{
			ci.format = pCreateInfo.storageImageViewFormat;
		}

		Result ppxres = GetDevice()->CreateStorageImageView(ci, &m_storageImageView);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create storage image view failed");
			return ppxres;
		}
	}

	return SUCCESS;
}

void Texture::destroyApiObjects()
{
	if (m_sampledImageView && (m_sampledImageView->GetOwnership() != Ownership::Reference))
	{
		GetDevice()->DestroySampledImageView(m_sampledImageView);
		m_sampledImageView.Reset();
	}

	if (m_renderTargetView && (m_renderTargetView->GetOwnership() != Ownership::Reference))
	{
		GetDevice()->DestroyRenderTargetView(m_renderTargetView);
		m_renderTargetView.Reset();
	}

	if (m_depthStencilView && (m_depthStencilView->GetOwnership() != Ownership::Reference))
	{
		GetDevice()->DestroyDepthStencilView(m_depthStencilView);
		m_depthStencilView.Reset();
	}

	if (m_storageImageView && (m_storageImageView->GetOwnership() != Ownership::Reference))
	{
		GetDevice()->DestroyStorageImageView(m_storageImageView);
		m_storageImageView.Reset();
	}

	if (m_image && (m_image->GetOwnership() != Ownership::Reference))
	{
		GetDevice()->DestroyImage(m_image);
		m_image.Reset();
	}
}

ImageType Texture::GetImageType() const
{
	return m_image->GetType();
}

uint32_t Texture::GetWidth() const
{
	return m_image->GetWidth();
}

uint32_t Texture::GetHeight() const
{
	return m_image->GetHeight();
}

uint32_t Texture::GetDepth() const
{
	return m_image->GetDepth();
}

Format Texture::GetImageFormat() const
{
	return m_image->GetFormat();
}

SampleCount Texture::GetSampleCount() const
{
	return m_image->GetSampleCount();
}

uint32_t Texture::GetMipLevelCount() const
{
	return m_image->GetMipLevelCount();
}

uint32_t Texture::GetArrayLayerCount() const
{
	return m_image->GetArrayLayerCount();
}

const ImageUsageFlags& Texture::GetUsageFlags() const
{
	return m_image->GetUsageFlags();
}

MemoryUsage Texture::GetMemoryUsage() const
{
	return m_image->GetMemoryUsage();
}

Format Texture::GetSampledImageViewFormat() const
{
	return m_sampledImageView ? m_sampledImageView->GetFormat() : Format::Undefined;
}

Format Texture::GetRenderTargetViewFormat() const
{
	return m_renderTargetView ? m_renderTargetView->GetFormat() : Format::Undefined;
}

Format Texture::GetDepthStencilViewFormat() const
{
	return m_depthStencilView ? m_depthStencilView->GetFormat() : Format::Undefined;
}

Format Texture::GetStorageImageViewFormat() const
{
	return m_storageImageView ? m_storageImageView->GetFormat() : Format::Undefined;
}

#pragma endregion

#pragma region RenderPass

void RenderPassCreateInfo::SetAllRenderTargetClearValue(const RenderTargetClearValue& value)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetClearValues[i] = value;
	}
}

void RenderPassCreateInfo2::SetAllRenderTargetUsageFlags(const ImageUsageFlags& flags)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetUsageFlags[i] = flags;
	}
}

void RenderPassCreateInfo2::SetAllRenderTargetClearValue(const RenderTargetClearValue& value)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetClearValues[i] = value;
	}
}

void RenderPassCreateInfo2::SetAllRenderTargetLoadOp(AttachmentLoadOp op)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetLoadOps[i] = op;
	}
}

void RenderPassCreateInfo2::SetAllRenderTargetStoreOp(AttachmentStoreOp op)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetStoreOps[i] = op;
	}
}

void RenderPassCreateInfo2::SetAllRenderTargetToClear()
{
	SetAllRenderTargetLoadOp(ATTACHMENT_LOAD_OP_CLEAR);
}

void RenderPassCreateInfo3::SetAllRenderTargetClearValue(const RenderTargetClearValue& value)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetClearValues[i] = value;
	}
}

void RenderPassCreateInfo3::SetAllRenderTargetLoadOp(AttachmentLoadOp op)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetLoadOps[i] = op;
	}
}

void RenderPassCreateInfo3::SetAllRenderTargetStoreOp(AttachmentStoreOp op)
{
	for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
		this->renderTargetStoreOps[i] = op;
	}
}

void RenderPassCreateInfo3::SetAllRenderTargetToClear()
{
	SetAllRenderTargetLoadOp(ATTACHMENT_LOAD_OP_CLEAR);
}

namespace internal
{

	RenderPassCreateInfo::RenderPassCreateInfo(const vkr::RenderPassCreateInfo& obj)
	{
		this->version = CREATE_INFO_VERSION_1;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->depthStencilState = obj.depthStencilState;
		this->pShadingRatePattern = obj.pShadingRatePattern;
		this->multiViewState = obj.multiViewState;

		// Views
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V1.pRenderTargetViews[i] = obj.pRenderTargetViews[i];
		}
		this->V1.pDepthStencilView = obj.pDepthStencilView;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
		}
		this->depthStencilClearValue = obj.depthStencilClearValue;
	}

	RenderPassCreateInfo::RenderPassCreateInfo(const vkr::RenderPassCreateInfo2& obj)
	{
		this->version = CREATE_INFO_VERSION_2;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->pShadingRatePattern = obj.pShadingRatePattern;

		// Formats
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V2.renderTargetFormats[i] = obj.renderTargetFormats[i];
		}
		this->V2.depthStencilFormat = obj.depthStencilFormat;

		// Sample count
		this->V2.sampleCount = obj.sampleCount;

		// Usage flags
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V2.renderTargetUsageFlags[i] = obj.renderTargetUsageFlags[i];
		}
		this->V2.depthStencilUsageFlags = obj.depthStencilUsageFlags;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
		}
		this->depthStencilClearValue = obj.depthStencilClearValue;

		// Load/store ops
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetLoadOps[i] = obj.renderTargetLoadOps[i];
			this->renderTargetStoreOps[i] = obj.renderTargetStoreOps[i];
		}
		this->depthLoadOp = obj.depthLoadOp;
		this->depthStoreOp = obj.depthStoreOp;
		this->stencilLoadOp = obj.stencilLoadOp;
		this->stencilStoreOp = obj.stencilStoreOp;

		// Initial states
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V2.renderTargetInitialStates[i] = obj.renderTargetInitialStates[i];
		}
		this->V2.depthStencilInitialState = obj.depthStencilInitialState;

		// MultiView
		this->arrayLayerCount = obj.arrayLayerCount;
		this->multiViewState = obj.multiViewState;
	}

	RenderPassCreateInfo::RenderPassCreateInfo(const vkr::RenderPassCreateInfo3& obj)
	{
		this->version = CREATE_INFO_VERSION_3;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->depthStencilState = obj.depthStencilState;
		this->pShadingRatePattern = obj.pShadingRatePattern;

		// Images
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V3.pRenderTargetImages[i] = obj.pRenderTargetImages[i];
		}
		this->V3.pDepthStencilImage = obj.pDepthStencilImage;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
		}
		this->depthStencilClearValue = obj.depthStencilClearValue;

		// Load/store ops
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetLoadOps[i] = obj.renderTargetLoadOps[i];
			this->renderTargetStoreOps[i] = obj.renderTargetStoreOps[i];
		}
		this->depthLoadOp = obj.depthLoadOp;
		this->depthStoreOp = obj.depthStoreOp;
		this->stencilLoadOp = obj.stencilLoadOp;
		this->stencilStoreOp = obj.stencilStoreOp;

		// MultiView
		this->arrayLayerCount = obj.arrayLayerCount;
		this->multiViewState = obj.multiViewState;
	}

} // namespace internal

// -------------------------------------------------------------------------------------------------
// RenderPass
// -------------------------------------------------------------------------------------------------
Result RenderPass::createImagesAndViewsV1(const internal::RenderPassCreateInfo& pCreateInfo)
{
	// Copy RTV and images
	for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
		RenderTargetViewPtr rtv = pCreateInfo.V1.pRenderTargetViews[i];
		if (!rtv)
		{
			ASSERT_MSG(false, "RTV << " + std::to_string(i) + " is null");
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}
		if (!rtv->GetImage())
		{
			ASSERT_MSG(false, "image << " + std::to_string(i) + " is null");
			return ERROR_UNEXPECTED_NULL_ARGUMENT;
		}

		mRenderTargetViews.push_back(rtv);
		mRenderTargetImages.push_back(rtv->GetImage());

		mHasLoadOpClear |= (rtv->GetLoadOp() == ATTACHMENT_LOAD_OP_CLEAR);
	}
	// Copy DSV and image
	if (!IsNull(pCreateInfo.V1.pDepthStencilView)) {
		DepthStencilViewPtr dsv = pCreateInfo.V1.pDepthStencilView;

		mDepthStencilView = dsv;
		mDepthStencilImage = dsv->GetImage();

		mHasLoadOpClear |= (dsv->GetDepthLoadOp() == ATTACHMENT_LOAD_OP_CLEAR);
		mHasLoadOpClear |= (dsv->GetStencilLoadOp() == ATTACHMENT_LOAD_OP_CLEAR);
	}

	return SUCCESS;
}

Result RenderPass::createImagesAndViewsV2(const internal::RenderPassCreateInfo& pCreateInfo)
{
	// Create images
	{
		// RTV images
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			ResourceState initialState = ResourceState::RenderTarget;
			if (pCreateInfo.V2.renderTargetInitialStates[i] != ResourceState::Undefined) {
				initialState = pCreateInfo.V2.renderTargetInitialStates[i];
			}
			ImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.type = ImageType::Image2D;
			imageCreateInfo.width = pCreateInfo.width;
			imageCreateInfo.height = pCreateInfo.height;
			imageCreateInfo.depth = 1;
			imageCreateInfo.format = pCreateInfo.V2.renderTargetFormats[i];
			imageCreateInfo.sampleCount = pCreateInfo.V2.sampleCount;
			imageCreateInfo.mipLevelCount = 1;
			imageCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			imageCreateInfo.usageFlags = pCreateInfo.V2.renderTargetUsageFlags[i];
			imageCreateInfo.memoryUsage = MemoryUsage::GPUOnly;
			imageCreateInfo.initialState = ResourceState::RenderTarget;
			imageCreateInfo.RTVClearValue = pCreateInfo.renderTargetClearValues[i];
			imageCreateInfo.ownership = pCreateInfo.ownership;

			ImagePtr image;
			Result         ppxres = GetDevice()->CreateImage(imageCreateInfo, &image);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV image create failed");
				return ppxres;
			}

			mRenderTargetImages.push_back(image);
		}

		// DSV image
		if (pCreateInfo.V2.depthStencilFormat != Format::Undefined) {
			ResourceState initialState = ResourceState::DepthStencilWrite;
			if (pCreateInfo.V2.depthStencilInitialState != ResourceState::Undefined) {
				initialState = pCreateInfo.V2.depthStencilInitialState;
			}

			ImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.type = ImageType::Image2D;
			imageCreateInfo.width = pCreateInfo.width;
			imageCreateInfo.height = pCreateInfo.height;
			imageCreateInfo.depth = 1;
			imageCreateInfo.format = pCreateInfo.V2.depthStencilFormat;
			imageCreateInfo.sampleCount = pCreateInfo.V2.sampleCount;
			imageCreateInfo.mipLevelCount = 1;
			imageCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			imageCreateInfo.usageFlags = pCreateInfo.V2.depthStencilUsageFlags;
			imageCreateInfo.memoryUsage = MemoryUsage::GPUOnly;
			imageCreateInfo.initialState = initialState;
			imageCreateInfo.DSVClearValue = pCreateInfo.depthStencilClearValue;
			imageCreateInfo.ownership = pCreateInfo.ownership;

			ImagePtr image;
			Result         ppxres = GetDevice()->CreateImage(imageCreateInfo, &image);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "DSV image create failed");
				return ppxres;
			}

			mDepthStencilImage = image;
		}
	}

	// Create views
	{
		// RTVs
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			ImagePtr image = mRenderTargetImages[i];

			RenderTargetViewCreateInfo rtvCreateInfo = {};
			rtvCreateInfo.pImage = image;
			rtvCreateInfo.imageViewType = IMAGE_VIEW_TYPE_2D;
			rtvCreateInfo.format = pCreateInfo.V2.renderTargetFormats[i];
			rtvCreateInfo.mipLevel = 0;
			rtvCreateInfo.mipLevelCount = 1;
			rtvCreateInfo.arrayLayer = 0;
			rtvCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			rtvCreateInfo.components = {};
			rtvCreateInfo.loadOp = pCreateInfo.renderTargetLoadOps[i];
			rtvCreateInfo.storeOp = pCreateInfo.renderTargetStoreOps[i];
			rtvCreateInfo.ownership = pCreateInfo.ownership;

			RenderTargetViewPtr rtv;
			Result                    ppxres = GetDevice()->CreateRenderTargetView(rtvCreateInfo, &rtv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV create failed");
				return ppxres;
			}

			mRenderTargetViews.push_back(rtv);

			mHasLoadOpClear |= (rtvCreateInfo.loadOp == ATTACHMENT_LOAD_OP_CLEAR);
		}

		// DSV
		if (pCreateInfo.V2.depthStencilFormat != Format::Undefined) {
			ImagePtr image = mDepthStencilImage;

			DepthStencilViewCreateInfo dsvCreateInfo = {};
			dsvCreateInfo.pImage = image;
			dsvCreateInfo.imageViewType = IMAGE_VIEW_TYPE_2D;
			dsvCreateInfo.format = pCreateInfo.V2.depthStencilFormat;
			dsvCreateInfo.mipLevel = 0;
			dsvCreateInfo.mipLevelCount = 1;
			dsvCreateInfo.arrayLayer = 0;
			dsvCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			dsvCreateInfo.components = {};
			dsvCreateInfo.depthLoadOp = pCreateInfo.depthLoadOp;
			dsvCreateInfo.depthStoreOp = pCreateInfo.depthStoreOp;
			dsvCreateInfo.stencilLoadOp = pCreateInfo.stencilLoadOp;
			dsvCreateInfo.stencilStoreOp = pCreateInfo.stencilStoreOp;
			dsvCreateInfo.ownership = pCreateInfo.ownership;

			DepthStencilViewPtr dsv;
			Result                    ppxres = GetDevice()->CreateDepthStencilView(dsvCreateInfo, &dsv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV create failed");
				return ppxres;
			}

			mDepthStencilView = dsv;

			mHasLoadOpClear |= (dsvCreateInfo.depthLoadOp == ATTACHMENT_LOAD_OP_CLEAR);
			mHasLoadOpClear |= (dsvCreateInfo.stencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR);
		}
	}

	return SUCCESS;
}

Result RenderPass::createImagesAndViewsV3(const internal::RenderPassCreateInfo& pCreateInfo)
{
	// Copy images
	{
		// Copy RTV images
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			ImagePtr image = pCreateInfo.V3.pRenderTargetImages[i];
			if (!image) {
				ASSERT_MSG(false, "image << " + std::to_string(i) + " is null");
				return ERROR_UNEXPECTED_NULL_ARGUMENT;
			}

			mRenderTargetImages.push_back(image);
		}
		// Copy DSV image
		if (!IsNull(pCreateInfo.V3.pDepthStencilImage)) {
			mDepthStencilImage = pCreateInfo.V3.pDepthStencilImage;
		}
	}

	// Create views
	{
		// RTVs
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			ImagePtr image = mRenderTargetImages[i];

			RenderTargetViewCreateInfo rtvCreateInfo = {};
			rtvCreateInfo.pImage = image;
			rtvCreateInfo.imageViewType = image->GuessImageViewType();
			rtvCreateInfo.format = image->GetFormat();
			rtvCreateInfo.mipLevel = 0;
			rtvCreateInfo.mipLevelCount = image->GetMipLevelCount();
			rtvCreateInfo.arrayLayer = 0;
			rtvCreateInfo.arrayLayerCount = image->GetArrayLayerCount();
			rtvCreateInfo.components = {};
			rtvCreateInfo.loadOp = pCreateInfo.renderTargetLoadOps[i];
			rtvCreateInfo.storeOp = pCreateInfo.renderTargetStoreOps[i];
			rtvCreateInfo.ownership = pCreateInfo.ownership;

			RenderTargetViewPtr rtv;
			Result                    ppxres = GetDevice()->CreateRenderTargetView(rtvCreateInfo, &rtv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV create failed");
				return ppxres;
			}

			mRenderTargetViews.push_back(rtv);

			mHasLoadOpClear |= (rtvCreateInfo.loadOp == ATTACHMENT_LOAD_OP_CLEAR);
		}

		// DSV
		if (mDepthStencilImage) {
			ImagePtr image = mDepthStencilImage;

			DepthStencilViewCreateInfo dsvCreateInfo = {};
			dsvCreateInfo.pImage = image;
			dsvCreateInfo.imageViewType = image->GuessImageViewType();
			dsvCreateInfo.format = image->GetFormat();
			dsvCreateInfo.mipLevel = 0;
			dsvCreateInfo.mipLevelCount = image->GetMipLevelCount();
			dsvCreateInfo.arrayLayer = 0;
			dsvCreateInfo.arrayLayerCount = image->GetArrayLayerCount();
			dsvCreateInfo.components = {};
			dsvCreateInfo.depthLoadOp = pCreateInfo.depthLoadOp;
			dsvCreateInfo.depthStoreOp = pCreateInfo.depthStoreOp;
			dsvCreateInfo.stencilLoadOp = pCreateInfo.stencilLoadOp;
			dsvCreateInfo.stencilStoreOp = pCreateInfo.stencilStoreOp;
			dsvCreateInfo.ownership = pCreateInfo.ownership;

			DepthStencilViewPtr dsv;
			Result                    ppxres = GetDevice()->CreateDepthStencilView(dsvCreateInfo, &dsv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "DSV create failed");
				return ppxres;
			}

			mDepthStencilView = dsv;

			mHasLoadOpClear |= (dsvCreateInfo.depthLoadOp == ATTACHMENT_LOAD_OP_CLEAR);
			mHasLoadOpClear |= (dsvCreateInfo.stencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR);
		}
	}

	return SUCCESS;
}

Result RenderPass::create(const internal::RenderPassCreateInfo& pCreateInfo)
{
	mRenderArea = { 0, 0, pCreateInfo.width, pCreateInfo.height };
	mViewport = { 0.0f, 0.0f, static_cast<float>(pCreateInfo.width), static_cast<float>(pCreateInfo.height), 0.0f, 1.0f };

	switch (pCreateInfo.version) {
	default: return ERROR_INVALID_CREATE_ARGUMENT; break;

	case internal::RenderPassCreateInfo::CREATE_INFO_VERSION_1: {
		Result ppxres = createImagesAndViewsV1(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
	} break;

	case internal::RenderPassCreateInfo::CREATE_INFO_VERSION_2: {
		Result ppxres = createImagesAndViewsV2(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
	} break;

	case internal::RenderPassCreateInfo::CREATE_INFO_VERSION_3: {
		Result ppxres = createImagesAndViewsV3(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
	} break;
	}

	Result ppxres = DeviceObject<internal::RenderPassCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result RenderPass::createApiObjects(const internal::RenderPassCreateInfo& pCreateInfo)
{
	Result ppxres = createRenderPass(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	ppxres = createFramebuffer(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

void RenderPass::destroyApiObjects()
{
	if (mFramebuffer) {
		vkDestroyFramebuffer(
			GetDevice()->GetVkDevice(),
			mFramebuffer,
			nullptr);
		mFramebuffer.Reset();
	}

	if (mRenderPass) {
		vkDestroyRenderPass(
			GetDevice()->GetVkDevice(),
			mRenderPass,
			nullptr);
		mRenderPass.Reset();
	}
}

Result RenderPass::createRenderPass(const internal::RenderPassCreateInfo& pCreateInfo)
{
	bool hasDepthSencil = mDepthStencilView ? true : false;

	uint32_t      depthStencilAttachment = -1;
	size_t        rtvCount = CountU32(mRenderTargetViews);
	VkImageLayout depthStencillayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// Determine layout for depth/stencil
	{
		// These variables are not used for anything meaningful in ToVkBarrierDst so they can be all zeroes.
		VkPhysicalDeviceFeatures features = {};
		VkPipelineStageFlags     stageMask = 0;
		VkAccessFlags            accessMask = 0;

		Result ppxres = ToVkBarrierDst(pCreateInfo.depthStencilState, CommandType::COMMAND_TYPE_GRAPHICS, features, stageMask, accessMask, depthStencillayout);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "failed to determine layout for depth stencil state");
			return ppxres;
		}
	}

	// Attachment descriptions
	std::vector<VkAttachmentDescription> attachmentDescs;
	{
		for (uint32_t i = 0; i < rtvCount; ++i)
		{
			RenderTargetViewPtr rtv = mRenderTargetViews[i];

			VkAttachmentDescription desc = {};
			desc.flags = 0;
			desc.format = ToVkEnum(rtv->GetFormat());
			desc.samples = ToVkSampleCount(rtv->GetSampleCount());
			desc.loadOp = ToVkAttachmentLoadOp(rtv->GetLoadOp());
			desc.storeOp = ToVkAttachmentStoreOp(rtv->GetStoreOp());
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			desc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			attachmentDescs.push_back(desc);
		}

		if (hasDepthSencil)
		{
			DepthStencilViewPtr dsv = mDepthStencilView;

			VkAttachmentDescription desc = {};
			desc.flags = 0;
			desc.format = ToVkEnum(dsv->GetFormat());
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = ToVkAttachmentLoadOp(dsv->GetDepthLoadOp());
			desc.storeOp = ToVkAttachmentStoreOp(dsv->GetDepthStoreOp());
			desc.stencilLoadOp = ToVkAttachmentLoadOp(dsv->GetStencilLoadOp());
			desc.stencilStoreOp = ToVkAttachmentStoreOp(dsv->GetStencilStoreOp());
			desc.initialLayout = depthStencillayout;
			desc.finalLayout = depthStencillayout;

			depthStencilAttachment = attachmentDescs.size();
			attachmentDescs.push_back(desc);
		}
	}

	std::vector<VkAttachmentReference> colorRefs;
	{
		for (uint32_t i = 0; i < rtvCount; ++i) {
			VkAttachmentReference ref = {};
			ref.attachment = i;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefs.push_back(ref);
		}
	}

	VkAttachmentReference depthStencilRef = {};
	if (hasDepthSencil) {
		depthStencilRef.attachment = depthStencilAttachment;
		depthStencilRef.layout = depthStencillayout;
	}

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = CountU32(colorRefs);
	subpassDescription.pColorAttachments = DataPtr(colorRefs);
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = hasDepthSencil ? &depthStencilRef : nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	VkSubpassDependency subpassDependencies = {};
	subpassDependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies.dstSubpass = 0;
	subpassDependencies.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies.srcAccessMask = 0;
	subpassDependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies.dependencyFlags = 0;

	VkRenderPassCreateInfo vkci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	vkci.flags = 0;
	vkci.attachmentCount = CountU32(attachmentDescs);
	vkci.pAttachments = DataPtr(attachmentDescs);
	vkci.subpassCount = 1;
	vkci.pSubpasses = &subpassDescription;
	vkci.dependencyCount = 1;
	vkci.pDependencies = &subpassDependencies;

	bool hasMultiView = GetDevice()->HasMultiView();

	VkRenderPassMultiviewCreateInfo multiviewInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO };
	if (pCreateInfo.multiViewState.viewMask > 0)
	{
		ASSERT_MSG(hasMultiView, "Multiview not supported on the device");
		multiviewInfo.subpassCount = 1;
		multiviewInfo.pViewMasks = &pCreateInfo.multiViewState.viewMask;
		multiviewInfo.correlationMaskCount = 1;
		multiviewInfo.pCorrelationMasks = &pCreateInfo.multiViewState.correlationMask;

		vkci.pNext = &multiviewInfo;
	}

	if (!IsNull(pCreateInfo.pShadingRatePattern))
	{
		SampleCount shadingRateSampleCount = pCreateInfo.pShadingRatePattern->GetSampleCount();
		for (uint32_t i = 0; i < rtvCount; ++i)
		{
			ASSERT_MSG(
				mRenderTargetViews[i]->GetSampleCount() == shadingRateSampleCount,
				"The sample count for each render target must match the sample count of the shading rate pattern.");
		}
		if (hasDepthSencil)
		{
			ASSERT_MSG(
				mDepthStencilView->GetSampleCount() == shadingRateSampleCount,
				"The sample count of the depth attachment must match the sample count of the shading rate pattern.");
		}
		auto     modifiedCreateInfo = pCreateInfo.pShadingRatePattern->GetModifiedRenderPassCreateInfo(vkci);
		VkResult vkres = vkCreateRenderPass2KHR(GetDevice()->GetVkDevice(), modifiedCreateInfo.get(), nullptr, &mRenderPass);
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vkCreateRenderPass failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}
	else
	{
		VkResult vkres = vkCreateRenderPass(GetDevice()->GetVkDevice(), &vkci, nullptr, &mRenderPass);
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vkCreateRenderPass failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

Result RenderPass::createFramebuffer(const internal::RenderPassCreateInfo& pCreateInfo)
{
	bool hasDepthSencil = mDepthStencilView ? true : false;

	size_t rtvCount = CountU32(mRenderTargetViews);

	std::vector<VkImageView> attachments;
	for (uint32_t i = 0; i < rtvCount; ++i)
	{
		RenderTargetViewPtr rtv = mRenderTargetViews[i];
		attachments.push_back(rtv.Get()->GetVkImageView());
	}

	if (hasDepthSencil)
	{
		DepthStencilViewPtr dsv = mDepthStencilView;
		attachments.push_back(dsv.Get()->GetVkImageView());
	}

	if (!IsNull(pCreateInfo.pShadingRatePattern))
	{
		if (pCreateInfo.pShadingRatePattern->GetShadingRateMode() == SHADING_RATE_FDM)
		{
			if (rtvCount > 0)
			{
				// Check that all or none of the render targets and depth-stencil attachments are subsampled.
				bool subsampled = mRenderTargetViews[0]->GetImage()->GetCreateFlags().bits.subsampledFormat;
				if (!subsampled)
				{
					ASSERT_MSG(GetDevice()->GetShadingRateCapabilities().fdm.supportsNonSubsampledImages, "Non-subsampled render target images with FDM shading rate are not supported.");
				}

				// This device does not support non-subsampled image attachments with FDM shading rate.
				// Check that all the attachments are subsampled.
				for (uint32_t i = 0; i < rtvCount; ++i)
				{
					RenderTargetViewPtr rtv = mRenderTargetViews[i];
					if (subsampled)
					{
						ASSERT_MSG(rtv->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render target image 0 is subsampled, but render target " + std::to_string(i) + " is not subsampled.");
					}
					else
					{
						ASSERT_MSG(!rtv->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render target image 0 is not subsampled, but render target " + std::to_string(i) + " is subsampled.");
					}
				}
				if (hasDepthSencil)
				{
					if (subsampled)
					{
						ASSERT_MSG(mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render targets are subsampled, but depth-stencil image is not subsampled.");
					}
					else
					{
						ASSERT_MSG(!mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat, "Render targets are subsampled, but depth-stencil image is not subsampled.");
					}
				}
			}
			else if (hasDepthSencil && !mDepthStencilView->GetImage()->GetCreateFlags().bits.subsampledFormat) 
			{
				// No render targets, only depth/stencil which is not subsampled.
				ASSERT_MSG(GetDevice()->GetShadingRateCapabilities().fdm.supportsNonSubsampledImages, "Non-subsampled depth-stencil image with FDM shading rate are not supported.");
			}
		}
		attachments.push_back(pCreateInfo.pShadingRatePattern->GetAttachmentImageView());
	}
	VkFramebufferCreateInfo vkci = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	vkci.flags = 0;
	vkci.renderPass = mRenderPass;
	vkci.attachmentCount = CountU32(attachments);
	vkci.pAttachments = DataPtr(attachments);
	vkci.width = pCreateInfo.width;
	vkci.height = pCreateInfo.height;
	vkci.layers = 1;

	VkResult vkres = vkCreateFramebuffer(GetDevice()->GetVkDevice(), &vkci, nullptr, &mFramebuffer);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateFramebuffer failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void RenderPass::destroy()
{
	for (uint32_t i = 0; i < m_createInfo.renderTargetCount; ++i) {
		RenderTargetViewPtr& rtv = mRenderTargetViews[i];
		if (rtv && (rtv->GetOwnership() != Ownership::Reference)) {
			GetDevice()->DestroyRenderTargetView(rtv);
			rtv.Reset();
		}

		ImagePtr& image = mRenderTargetImages[i];
		if (image && (image->GetOwnership() != Ownership::Reference)) {
			GetDevice()->DestroyImage(image);
			image.Reset();
		}
	}
	mRenderTargetViews.clear();
	mRenderTargetImages.clear();

	if (mDepthStencilView && (mDepthStencilView->GetOwnership() != Ownership::Reference)) {
		GetDevice()->DestroyDepthStencilView(mDepthStencilView);
		mDepthStencilView.Reset();
	}

	if (mDepthStencilImage && (mDepthStencilImage->GetOwnership() != Ownership::Reference)) {
		GetDevice()->DestroyImage(mDepthStencilImage);
		mDepthStencilImage.Reset();
	}

	DeviceObject<internal::RenderPassCreateInfo>::destroy();
}

Result RenderPass::GetRenderTargetView(uint32_t index, RenderTargetView** ppView) const
{
	if (!IsIndexInRange(index, mRenderTargetViews)) {
		return ERROR_OUT_OF_RANGE;
	}
	*ppView = mRenderTargetViews[index];
	return SUCCESS;
}

Result RenderPass::GetDepthStencilView(DepthStencilView** ppView) const
{
	if (!mDepthStencilView) {
		return ERROR_ELEMENT_NOT_FOUND;
	}
	*ppView = mDepthStencilView;
	return SUCCESS;
}

Result RenderPass::GetRenderTargetImage(uint32_t index, Image** ppImage) const
{
	if (!IsIndexInRange(index, mRenderTargetImages)) {
		return ERROR_OUT_OF_RANGE;
	}
	*ppImage = mRenderTargetImages[index];
	return SUCCESS;
}

Result RenderPass::GetDepthStencilImage(Image** ppImage) const
{
	if (!mDepthStencilImage) {
		return ERROR_ELEMENT_NOT_FOUND;
	}
	*ppImage = mDepthStencilImage;
	return SUCCESS;
}

RenderTargetViewPtr RenderPass::GetRenderTargetView(uint32_t index) const
{
	RenderTargetViewPtr object;
	GetRenderTargetView(index, &object);
	return object;
}

DepthStencilViewPtr RenderPass::GetDepthStencilView() const
{
	DepthStencilViewPtr object;
	GetDepthStencilView(&object);
	return object;
}

ImagePtr RenderPass::GetRenderTargetImage(uint32_t index) const
{
	ImagePtr object;
	GetRenderTargetImage(index, &object);
	return object;
}

ImagePtr RenderPass::GetDepthStencilImage() const
{
	ImagePtr object;
	GetDepthStencilImage(&object);
	return object;
}

uint32_t RenderPass::GetRenderTargetImageIndex(const Image* pImage) const
{
	uint32_t index = UINT32_MAX;
	for (uint32_t i = 0; i < CountU32(mRenderTargetImages); ++i) {
		if (mRenderTargetImages[i] == pImage) {
			index = i;
			break;
		}
	}
	return index;
}

Result RenderPass::DisownRenderTargetView(uint32_t index, RenderTargetView** ppView)
{
	if (IsIndexInRange(index, mRenderTargetViews)) {
		return ERROR_OUT_OF_RANGE;
	}
	if (mRenderTargetViews[index]->GetOwnership() == Ownership::Restricted) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mRenderTargetViews[index]->SetOwnership(Ownership::Reference);

	if (!IsNull(ppView)) {
		*ppView = mRenderTargetViews[index];
	}
	return SUCCESS;
}

Result RenderPass::DisownDepthStencilView(DepthStencilView** ppView)
{
	if (!mDepthStencilView) {
		return ERROR_ELEMENT_NOT_FOUND;
	}
	if (mDepthStencilView->GetOwnership() == Ownership::Restricted) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mDepthStencilView->SetOwnership(Ownership::Reference);

	if (!IsNull(ppView)) {
		*ppView = mDepthStencilView;
	}
	return SUCCESS;
}

Result RenderPass::DisownRenderTargetImage(uint32_t index, Image** ppImage)
{
	if (IsIndexInRange(index, mRenderTargetImages)) {
		return ERROR_OUT_OF_RANGE;
	}
	if (mRenderTargetImages[index]->GetOwnership() == Ownership::Restricted) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mRenderTargetImages[index]->SetOwnership(Ownership::Reference);

	if (!IsNull(ppImage)) {
		*ppImage = mRenderTargetImages[index];
	}
	return SUCCESS;
}

Result RenderPass::DisownDepthStencilImage(Image** ppImage)
{
	if (!mDepthStencilImage) {
		return ERROR_ELEMENT_NOT_FOUND;
	}
	if (mDepthStencilImage->GetOwnership() == Ownership::Restricted) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mDepthStencilImage->SetOwnership(Ownership::Reference);

	if (!IsNull(ppImage)) {
		*ppImage = mDepthStencilImage;
	}
	return SUCCESS;
}

VkResult CreateTransientRenderPass(RenderDevice* device, uint32_t renderTargetCount, const VkFormat* pRenderTargetFormats, VkFormat depthStencilFormat, VkSampleCountFlagBits sampleCount, uint32_t viewMask, uint32_t correlationMask, VkRenderPass* pRenderPass, ShadingRateMode shadingRateMode)
{
	bool hasDepthSencil = (depthStencilFormat != VK_FORMAT_UNDEFINED);

	uint32_t depthStencilAttachment = -1;

	std::vector<VkAttachmentDescription> attachmentDescs;
	{
		for (uint32_t i = 0; i < renderTargetCount; ++i) {
			VkAttachmentDescription desc = {};
			desc.flags = 0;
			desc.format = pRenderTargetFormats[i];
			desc.samples = sampleCount;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentDescs.push_back(desc);
		}

		if (hasDepthSencil) {
			VkAttachmentDescription desc = {};
			desc.flags = 0;
			desc.format = depthStencilFormat;
			desc.samples = VK_SAMPLE_COUNT_1_BIT;
			desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthStencilAttachment = attachmentDescs.size();
			attachmentDescs.push_back(desc);
		}
	}

	std::vector<VkAttachmentReference> colorRefs;
	{
		for (uint32_t i = 0; i < renderTargetCount; ++i) {
			VkAttachmentReference ref = {};
			ref.attachment = i;
			ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			colorRefs.push_back(ref);
		}
	}

	VkAttachmentReference depthStencilRef = {};
	if (hasDepthSencil) {
		depthStencilRef.attachment = depthStencilAttachment;
		depthStencilRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	VkSubpassDescription subpassDescription = {};
	subpassDescription.flags = 0;
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.colorAttachmentCount = CountU32(colorRefs);
	subpassDescription.pColorAttachments = DataPtr(colorRefs);
	subpassDescription.pResolveAttachments = nullptr;
	subpassDescription.pDepthStencilAttachment = hasDepthSencil ? &depthStencilRef : nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;

	VkSubpassDependency subpassDependencies = {};
	subpassDependencies.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies.dstSubpass = 0;
	subpassDependencies.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies.srcAccessMask = 0;
	subpassDependencies.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	subpassDependencies.dependencyFlags = 0;

	VkRenderPassCreateInfo vkci = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	vkci.flags = 0;
	vkci.attachmentCount = CountU32(attachmentDescs);
	vkci.pAttachments = DataPtr(attachmentDescs);
	vkci.subpassCount = 1;
	vkci.pSubpasses = &subpassDescription;
	vkci.dependencyCount = 1;
	vkci.pDependencies = &subpassDependencies;
	// Callers responsibiltiy to only set viewmask if it is required
	VkRenderPassMultiviewCreateInfo multiviewInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_MULTIVIEW_CREATE_INFO };
	if (viewMask > 0) {
		multiviewInfo.subpassCount = 1;
		multiviewInfo.pViewMasks = &viewMask;
		multiviewInfo.correlationMaskCount = 1;
		multiviewInfo.pCorrelationMasks = &correlationMask;

		vkci.pNext = &multiviewInfo;
	}

	if (shadingRateMode != SHADING_RATE_NONE) {
		auto     modifiedCreateInfo = ShadingRatePattern::GetModifiedRenderPassCreateInfo(device, shadingRateMode, vkci);
		VkResult vkres = vkCreateRenderPass2KHR(
			device->GetVkDevice(),
			modifiedCreateInfo.get(),
			nullptr,
			pRenderPass);
		if (vkres != VK_SUCCESS) {
			return vkres;
		}
	}
	else {
		VkResult vkres = vkCreateRenderPass(
			device->GetVkDevice(),
			&vkci,
			nullptr,
			pRenderPass);
		if (vkres != VK_SUCCESS) {
			return vkres;
		}
	}

	return VK_SUCCESS;
}

#pragma endregion

#pragma region DrawPass

namespace internal {

	DrawPassCreateInfo::DrawPassCreateInfo(const vkr::DrawPassCreateInfo& obj)
	{
		this->version = CREATE_INFO_VERSION_1;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->pShadingRatePattern = obj.pShadingRatePattern;

		// Formats
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V1.renderTargetFormats[i] = obj.renderTargetFormats[i];
		}
		this->V1.depthStencilFormat = obj.depthStencilFormat;

		// Sample count
		this->V1.sampleCount = obj.sampleCount;

		// Usage flags
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V1.renderTargetUsageFlags[i] = obj.renderTargetUsageFlags[i] | IMAGE_USAGE_COLOR_ATTACHMENT;
		}
		this->V1.depthStencilUsageFlags = obj.depthStencilUsageFlags | IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V1.renderTargetInitialStates[i] = obj.renderTargetInitialStates[i];
		}
		this->V1.depthStencilInitialState = obj.depthStencilInitialState;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
		}
		this->depthStencilClearValue = obj.depthStencilClearValue;

		this->V1.imageCreateFlags = obj.imageCreateFlags;
	}

	DrawPassCreateInfo::DrawPassCreateInfo(const vkr::DrawPassCreateInfo2& obj)
	{
		this->version = CREATE_INFO_VERSION_2;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->depthStencilState = obj.depthStencilState;
		this->pShadingRatePattern = obj.pShadingRatePattern;

		// Images
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V2.pRenderTargetImages[i] = obj.pRenderTargetImages[i];
		}
		this->V2.pDepthStencilImage = obj.pDepthStencilImage;

		// Clear values
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->renderTargetClearValues[i] = obj.renderTargetClearValues[i];
		}
		this->depthStencilClearValue = obj.depthStencilClearValue;
	}

	DrawPassCreateInfo::DrawPassCreateInfo(const vkr::DrawPassCreateInfo3& obj)
	{
		this->version = CREATE_INFO_VERSION_3;
		this->width = obj.width;
		this->height = obj.height;
		this->renderTargetCount = obj.renderTargetCount;
		this->depthStencilState = obj.depthStencilState;
		this->pShadingRatePattern = obj.pShadingRatePattern;

		// Textures
		for (uint32_t i = 0; i < this->renderTargetCount; ++i) {
			this->V3.pRenderTargetTextures[i] = obj.pRenderTargetTextures[i];
		}
		this->V3.pDepthStencilTexture = obj.pDepthStencilTexture;
	}

}

Result DrawPass::CreateTexturesV1(const internal::DrawPassCreateInfo& pCreateInfo)
{
	// Create textures
	{
		// Create render target textures
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i)
		{
			TextureCreateInfo ci = {};
			ci.pImage = nullptr;
			ci.imageType = ImageType::Image2D;
			ci.width = pCreateInfo.width;
			ci.height = pCreateInfo.height;
			ci.depth = 1;
			ci.imageFormat = pCreateInfo.V1.renderTargetFormats[i];
			ci.sampleCount = pCreateInfo.V1.sampleCount;
			ci.mipLevelCount = 1;
			ci.arrayLayerCount = 1;
			ci.usageFlags = pCreateInfo.V1.renderTargetUsageFlags[i];
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = pCreateInfo.V1.renderTargetInitialStates[i];
			ci.RTVClearValue = pCreateInfo.renderTargetClearValues[i];
			ci.sampledImageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
			ci.sampledImageViewFormat = Format::Undefined;
			ci.renderTargetViewFormat = Format::Undefined;
			ci.depthStencilViewFormat = Format::Undefined;
			ci.storageImageViewFormat = Format::Undefined;
			ci.ownership = Ownership::Exclusive;
			ci.imageCreateFlags = pCreateInfo.V1.imageCreateFlags;

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "render target texture create failed");
				return ppxres;
			}

			mRenderTargetTextures.push_back(texture);
		}

		// DSV image
		if (pCreateInfo.V1.depthStencilFormat != Format::Undefined) {
			TextureCreateInfo ci = {};
			ci.pImage = nullptr;
			ci.imageType = ImageType::Image2D;
			ci.width = pCreateInfo.width;
			ci.height = pCreateInfo.height;
			ci.depth = 1;
			ci.imageFormat = pCreateInfo.V1.depthStencilFormat;
			ci.sampleCount = pCreateInfo.V1.sampleCount;
			ci.mipLevelCount = 1;
			ci.arrayLayerCount = 1;
			ci.usageFlags = pCreateInfo.V1.depthStencilUsageFlags;
			ci.memoryUsage = MemoryUsage::GPUOnly;
			ci.initialState = pCreateInfo.V1.depthStencilInitialState;
			ci.DSVClearValue = pCreateInfo.depthStencilClearValue;
			ci.sampledImageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
			ci.sampledImageViewFormat = Format::Undefined;
			ci.renderTargetViewFormat = Format::Undefined;
			ci.depthStencilViewFormat = Format::Undefined;
			ci.storageImageViewFormat = Format::Undefined;
			ci.ownership = Ownership::Exclusive;
			ci.imageCreateFlags = pCreateInfo.V1.imageCreateFlags;

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "depth stencil texture create failed");
				return ppxres;
			}

			mDepthStencilTexture = texture;
		}
	}
	return SUCCESS;
}

Result DrawPass::CreateTexturesV2(const internal::DrawPassCreateInfo& pCreateInfo)
{
	// Create textures
	{
		// Create render target textures
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			TextureCreateInfo ci = {};
			ci.pImage = pCreateInfo.V2.pRenderTargetImages[i];

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "render target texture create failed");
				return ppxres;
			}

			mRenderTargetTextures.push_back(texture);
		}

		// DSV image
		if (!IsNull(pCreateInfo.V2.pDepthStencilImage)) {
			TextureCreateInfo ci = {};
			ci.pImage = pCreateInfo.V2.pDepthStencilImage;

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "depth stencil texture create failed");
				return ppxres;
			}

			mDepthStencilTexture = texture;
		}
	}
	return SUCCESS;
}

Result DrawPass::CreateTexturesV3(const internal::DrawPassCreateInfo& pCreateInfo)
{
	// Create textures
	{
		// Create render target textures
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			TexturePtr texture = pCreateInfo.V3.pRenderTargetTextures[i];
			mRenderTargetTextures.push_back(texture);
		}

		// DSV image
		if (!IsNull(pCreateInfo.V3.pDepthStencilTexture)) {
			mDepthStencilTexture = pCreateInfo.V3.pDepthStencilTexture;
		}
	}
	return SUCCESS;
}

Result DrawPass::createApiObjects(const internal::DrawPassCreateInfo& pCreateInfo)
{
	mRenderArea = { 0, 0, pCreateInfo.width, pCreateInfo.height };

	// Create backing resources
	switch (pCreateInfo.version) {
	default: return ERROR_INVALID_CREATE_ARGUMENT; break;

	case internal::DrawPassCreateInfo::CREATE_INFO_VERSION_1: {
		Result ppxres = CreateTexturesV1(pCreateInfo);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "create textures(V1) failed");
			return ppxres;
		}
	} break;

	case internal::DrawPassCreateInfo::CREATE_INFO_VERSION_2: {
		Result ppxres = CreateTexturesV2(pCreateInfo);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "create textures(V2) failed");
			return ppxres;
		}
	} break;

	case internal::DrawPassCreateInfo::CREATE_INFO_VERSION_3: {
		Result ppxres = CreateTexturesV3(pCreateInfo);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "create textures(V3) failed");
			return ppxres;
		}
	} break;
	}

	// Create render passes
	for (uint32_t clearMask = 0; clearMask <= static_cast<uint32_t>(DRAW_PASS_CLEAR_FLAG_CLEAR_ALL); ++clearMask) {
		AttachmentLoadOp renderTargetLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		AttachmentLoadOp depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
		AttachmentLoadOp stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;

		if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS) != 0) {
			renderTargetLoadOp = ATTACHMENT_LOAD_OP_CLEAR;
		}
		if (mDepthStencilTexture) {
			if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH) != 0) {
				if (GetFormatDescription(mDepthStencilTexture->GetImageFormat())->aspect & FORMAT_ASPECT_DEPTH) {
					depthLoadOp = ATTACHMENT_LOAD_OP_CLEAR;
				}
			}
			if ((clearMask & DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL) != 0) {
				if (GetFormatDescription(mDepthStencilTexture->GetImageFormat())->aspect & FORMAT_ASPECT_STENCIL) {
					stencilLoadOp = ATTACHMENT_LOAD_OP_CLEAR;
				}
			}
		}

		// If the the depth/stencil state has READ for either depth or stecil then skip creating any LOAD_OP_CLEAR render passes for it. Not skipping will result in API errors.
		bool skip = false;
		if (depthLoadOp == ATTACHMENT_LOAD_OP_CLEAR) {
			switch (pCreateInfo.depthStencilState) {
			default: break;
			case ResourceState::DepthStencilRead:
			case ResourceState::DepthReadStencilWrite: {
				skip = true;
			} break;
			}
		}
		if (stencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR) {
			switch (pCreateInfo.depthStencilState) {
			default: break;
			case ResourceState::DepthStencilRead:
			case ResourceState::DepthWriteStencilRead: {
				skip = true;
			} break;
			}
		}
		if (skip) {
			continue;
		}

		RenderPassCreateInfo3 rpCreateInfo = {};
		rpCreateInfo.width = pCreateInfo.width;
		rpCreateInfo.height = pCreateInfo.height;
		rpCreateInfo.renderTargetCount = pCreateInfo.renderTargetCount;
		rpCreateInfo.depthStencilState = pCreateInfo.depthStencilState;

		for (uint32_t i = 0; i < rpCreateInfo.renderTargetCount; ++i) {
			if (!mRenderTargetTextures[i]) {
				continue;
			}
			rpCreateInfo.pRenderTargetImages[i] = mRenderTargetTextures[i]->GetImage();
			rpCreateInfo.renderTargetClearValues[i] = mRenderTargetTextures[i]->GetImage()->GetRTVClearValue();
			rpCreateInfo.renderTargetLoadOps[i] = renderTargetLoadOp;
			rpCreateInfo.renderTargetStoreOps[i] = ATTACHMENT_STORE_OP_STORE;
		}

		if (mDepthStencilTexture) {
			rpCreateInfo.pDepthStencilImage = mDepthStencilTexture->GetImage();
			rpCreateInfo.depthStencilClearValue = mDepthStencilTexture->GetImage()->GetDSVClearValue();
			rpCreateInfo.depthLoadOp = depthLoadOp;
			rpCreateInfo.depthStoreOp = ATTACHMENT_STORE_OP_STORE;
			rpCreateInfo.stencilLoadOp = stencilLoadOp;
			rpCreateInfo.stencilStoreOp = ATTACHMENT_STORE_OP_STORE;
		}

		if (!IsNull(pCreateInfo.pShadingRatePattern) && pCreateInfo.pShadingRatePattern->GetShadingRateMode() != SHADING_RATE_NONE) {
			rpCreateInfo.pShadingRatePattern = pCreateInfo.pShadingRatePattern;
		}

		Pass pass = {};
		pass.clearMask = clearMask;

		Result ppxres = GetDevice()->CreateRenderPass(rpCreateInfo, &pass.renderPass);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "create render pass failed for clearMask=" + std::to_string(clearMask));
			return ppxres;
		}

		mPasses.push_back(pass);
	}

	return SUCCESS;
}

void DrawPass::destroyApiObjects()
{
	for (size_t i = 0; i < mPasses.size(); ++i) {
		if (mPasses[i].renderPass) {
			GetDevice()->DestroyRenderPass(mPasses[i].renderPass);
			mPasses[i].renderPass.Reset();
		}
	}
	mPasses.clear();

	for (size_t i = 0; i < mRenderTargetTextures.size(); ++i) {
		if (mRenderTargetTextures[i] && (mRenderTargetTextures[i]->GetOwnership() == Ownership::Exclusive)) {
			GetDevice()->DestroyTexture(mRenderTargetTextures[i]);
			mRenderTargetTextures[i].Reset();
		}
	}
	mRenderTargetTextures.clear();

	if (mDepthStencilTexture && (mDepthStencilTexture->GetOwnership() == Ownership::Exclusive)) {
		GetDevice()->DestroyTexture(mDepthStencilTexture);
		mDepthStencilTexture.Reset();
	}

	for (size_t i = 0; i < mPasses.size(); ++i) {
		if (mPasses[i].renderPass) {
			GetDevice()->DestroyRenderPass(mPasses[i].renderPass);
			mPasses[i].renderPass.Reset();
		}
	}
	mPasses.clear();
}

const Rect& DrawPass::GetRenderArea() const
{
	ASSERT_MSG(mPasses.size() > 0, "no render passes");
	ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
	return mPasses[0].renderPass->GetRenderArea();
}

const Rect& DrawPass::GetScissor() const
{
	ASSERT_MSG(mPasses.size() > 0, "no render passes");
	ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
	return mPasses[0].renderPass->GetScissor();
}

const Viewport& DrawPass::GetViewport() const
{
	ASSERT_MSG(mPasses.size() > 0, "no render passes");
	ASSERT_MSG(!IsNull(mPasses[0].renderPass.Get()), "first render pass not valid");
	return mPasses[0].renderPass->GetViewport();
}

Result DrawPass::GetRenderTargetTexture(uint32_t index, Texture** ppRenderTarget) const
{
	if (index >= m_createInfo.renderTargetCount) {
		return ERROR_OUT_OF_RANGE;
	}
	*ppRenderTarget = mRenderTargetTextures[index];
	return SUCCESS;
}

Texture* DrawPass::GetRenderTargetTexture(uint32_t index) const
{
	Texture* pTexture = nullptr;
	GetRenderTargetTexture(index, &pTexture);
	return pTexture;
}

Result DrawPass::GetDepthStencilTexture(Texture** ppDepthStencil) const
{
	if (!HasDepthStencil()) {
		return ERROR_ELEMENT_NOT_FOUND;
	}
	*ppDepthStencil = mDepthStencilTexture;
	return SUCCESS;
}

Texture* DrawPass::GetDepthStencilTexture() const
{
	Texture* pTexture = nullptr;
	GetDepthStencilTexture(&pTexture);
	return pTexture;
}

void DrawPass::PrepareRenderPassBeginInfo(const DrawPassClearFlags& clearFlags, RenderPassBeginInfo* pBeginInfo) const
{
	uint32_t clearMask = clearFlags.flags;

	auto it = FindIf(
		mPasses,
		[clearMask](const Pass& elem) -> bool {
			bool isMatch = (elem.clearMask == clearMask);
			return isMatch; });
	if (it == std::end(mPasses)) {
		ASSERT_MSG(false, "couldn't find matching pass for clearMask=" + std::to_string(clearMask));
		return;
	}

	pBeginInfo->pRenderPass = it->renderPass;
	pBeginInfo->renderArea = GetRenderArea();
	pBeginInfo->RTVClearCount = m_createInfo.renderTargetCount;

	if (clearFlags & DRAW_PASS_CLEAR_FLAG_CLEAR_RENDER_TARGETS) {
		for (uint32_t i = 0; i < m_createInfo.renderTargetCount; ++i) {
			pBeginInfo->RTVClearValues[i] = m_createInfo.renderTargetClearValues[i];
		}
	}

	if ((clearFlags & DRAW_PASS_CLEAR_FLAG_CLEAR_DEPTH) || (clearFlags & DRAW_PASS_CLEAR_FLAG_CLEAR_STENCIL)) {
		pBeginInfo->DSVClearValue = m_createInfo.depthStencilClearValue;
	}
}

#pragma endregion

#pragma region Descriptor

Result DescriptorPool::createApiObjects(const DescriptorPoolCreateInfo& pCreateInfo)
{
	std::vector<VkDescriptorPoolSize> poolSizes;
	if (pCreateInfo.sampler > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLER               , pCreateInfo.sampler });
	if (pCreateInfo.combinedImageSampler > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, pCreateInfo.combinedImageSampler });
	if (pCreateInfo.sampledImage > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         , pCreateInfo.sampledImage });
	if (pCreateInfo.storageImage > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         , pCreateInfo.storageImage });
	if (pCreateInfo.uniformTexelBuffer > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  , pCreateInfo.uniformTexelBuffer });
	if (pCreateInfo.storageTexelBuffer > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  , pCreateInfo.storageTexelBuffer });
	if (pCreateInfo.uniformBuffer > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        , pCreateInfo.uniformBuffer });
	if (pCreateInfo.rawStorageBuffer > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        , pCreateInfo.rawStorageBuffer });
	if (pCreateInfo.uniformBufferDynamic > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, pCreateInfo.uniformBufferDynamic });
	if (pCreateInfo.storageBufferDynamic > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, pCreateInfo.storageBufferDynamic });
	if (pCreateInfo.inputAttachment > 0) poolSizes.push_back({ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT      , pCreateInfo.inputAttachment });

	if (pCreateInfo.structuredBuffer > 0)
	{
		auto it = FindIf(
			poolSizes,
			[](const VkDescriptorPoolSize& elem) -> bool {
				bool isSame = elem.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				return isSame; });
		if (it != std::end(poolSizes)) {
			it->descriptorCount += pCreateInfo.structuredBuffer;
		}
		else {
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, pCreateInfo.structuredBuffer });
		}
	}

	// Flags
	uint32_t flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

	VkDescriptorPoolCreateInfo vkci = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	vkci.flags = flags;
	vkci.maxSets = MaxSetsPerPool;
	vkci.poolSizeCount = CountU32(poolSizes);
	vkci.pPoolSizes = DataPtr(poolSizes);

	VkResult vkres = vkCreateDescriptorPool(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mDescriptorPool);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreateDescriptorPool failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void DescriptorPool::destroyApiObjects()
{
	if (mDescriptorPool) {
		vkDestroyDescriptorPool(GetDevice()->GetVkDevice(), mDescriptorPool, nullptr);
		mDescriptorPool.Reset();
	}
}

Result DescriptorSet::UpdateDescriptors(uint32_t writeCount, const WriteDescriptor* pWrites)
{
	if (writeCount == 0) {
		return ERROR_UNEXPECTED_COUNT_VALUE;
	}

	if (CountU32(mWriteStore) < writeCount)
	{
		mWriteStore.resize(writeCount);
		mImageInfoStore.resize(writeCount);
		mBufferInfoStore.resize(writeCount);
		mTexelBufferStore.resize(writeCount);
	}

	mImageCount = 0;
	mBufferCount = 0;
	mTexelBufferCount = 0;
	for (mWriteCount = 0; mWriteCount < writeCount; ++mWriteCount)
	{
		const WriteDescriptor& srcWrite = pWrites[mWriteCount];

		VkDescriptorImageInfo* pImageInfo = nullptr;
		VkBufferView* pTexelBufferView = nullptr;
		VkDescriptorBufferInfo* pBufferInfo = nullptr;

		VkDescriptorType descriptorType = ToVkDescriptorType(srcWrite.type);
		switch (descriptorType) {
		default: {
			ASSERT_MSG(false, "unknown descriptor type: " + ToString(descriptorType));
			return ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE;
		} break;

		case VK_DESCRIPTOR_TYPE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
		case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
		case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
		case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
		{
			ASSERT_MSG(mImageCount < mImageInfoStore.size(), "image count exceeds image store capacity");
			pImageInfo = &mImageInfoStore[mImageCount];
			// Fill out info
			pImageInfo->sampler = VK_NULL_HANDLE;
			pImageInfo->imageView = VK_NULL_HANDLE;
			pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			switch (descriptorType) {
			default: break;
			case VK_DESCRIPTOR_TYPE_SAMPLER: {
				pImageInfo->sampler = srcWrite.pSampler->GetVkSampler();
			} break;

			case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: {
				pImageInfo->sampler = srcWrite.pSampler->GetVkSampler();
				pImageInfo->imageView = srcWrite.pImageView->GetResourceView()->GetVkImageView();
				pImageInfo->imageLayout = srcWrite.pImageView->GetResourceView()->GetVkImageLayout();
			} break;

			case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
			case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
			case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: {
				pImageInfo->imageView = srcWrite.pImageView->GetResourceView()->GetVkImageView();
				pImageInfo->imageLayout = srcWrite.pImageView->GetResourceView()->GetVkImageLayout();
			} break;
			}
			// Increment count
			mImageCount += 1;
		} break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: {
			ASSERT_MSG(false, "TEXEL BUFFER NOT IMPLEMENTED");
			ASSERT_MSG(mTexelBufferCount < mImageInfoStore.size(), "texel buffer count exceeds texel buffer store capacity");
			pTexelBufferView = &mTexelBufferStore[mTexelBufferCount];
			// Fill out info
			// Increment count
			mTexelBufferCount += 1;
		} break;

		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
		case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
		case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
			ASSERT_MSG(mBufferCount < mBufferInfoStore.size(), "buffer count exceeds buffer store capacity");
			pBufferInfo = &mBufferInfoStore[mBufferCount];
			// Fill out info
			pBufferInfo->buffer = srcWrite.pBuffer->GetVkBuffer();
			pBufferInfo->offset = srcWrite.bufferOffset;
			pBufferInfo->range = (srcWrite.bufferRange == WHOLE_SIZE) ? VK_WHOLE_SIZE : static_cast<VkDeviceSize>(srcWrite.bufferRange);
			// Increment count
			mBufferCount += 1;
		} break;
		}

		VkWriteDescriptorSet& vkWrite = mWriteStore[mWriteCount];
		vkWrite = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
		vkWrite.dstSet = mDescriptorSet;
		vkWrite.dstBinding = srcWrite.binding;
		vkWrite.dstArrayElement = srcWrite.arrayIndex;
		vkWrite.descriptorCount = 1;
		vkWrite.descriptorType = descriptorType;
		vkWrite.pImageInfo = pImageInfo;
		vkWrite.pBufferInfo = pBufferInfo;
		vkWrite.pTexelBufferView = pTexelBufferView;
	}

	vkUpdateDescriptorSets(
		GetDevice()->GetVkDevice(),
		mWriteCount,
		mWriteStore.data(),
		0,
		nullptr);

	return SUCCESS;
}

Result DescriptorSet::UpdateSampler(
	uint32_t             binding,
	uint32_t             arrayIndex,
	const Sampler* pSampler)
{
	WriteDescriptor write = {};
	write.binding = binding;
	write.arrayIndex = arrayIndex;
	write.type = DESCRIPTOR_TYPE_SAMPLER;
	write.pSampler = pSampler;

	Result ppxres = UpdateDescriptors(1, &write);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result DescriptorSet::UpdateSampledImage(
	uint32_t                      binding,
	uint32_t                      arrayIndex,
	const SampledImageView* pImageView)
{
	WriteDescriptor write = {};
	write.binding = binding;
	write.arrayIndex = arrayIndex;
	write.type = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageView = pImageView;

	return UpdateDescriptors(1, &write);
}

Result DescriptorSet::UpdateSampledImage(
	uint32_t             binding,
	uint32_t             arrayIndex,
	const Texture* pTexture)
{
	WriteDescriptor write = {};
	write.binding = binding;
	write.arrayIndex = arrayIndex;
	write.type = DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	write.pImageView = pTexture->GetSampledImageView();

	return UpdateDescriptors(1, &write);
}

Result DescriptorSet::UpdateStorageImage(
	uint32_t             binding,
	uint32_t             arrayIndex,
	const Texture* pTexture)
{
	WriteDescriptor write = {};
	write.binding = binding;
	write.arrayIndex = arrayIndex;
	write.type = DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write.pImageView = pTexture->GetStorageImageView();

	Result ppxres = UpdateDescriptors(1, &write);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result DescriptorSet::UpdateUniformBuffer(
	uint32_t            binding,
	uint32_t            arrayIndex,
	const Buffer* pBuffer,
	uint64_t            offset,
	uint64_t            range)
{
	WriteDescriptor write = {};
	write.binding = binding;
	write.arrayIndex = arrayIndex;
	write.type = DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.bufferOffset = offset;
	write.bufferRange = range;
	write.pBuffer = pBuffer;

	Result ppxres = UpdateDescriptors(1, &write);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result DescriptorSet::createApiObjects(const internal::DescriptorSetCreateInfo& pCreateInfo)
{
	mDescriptorPool = pCreateInfo.pPool->GetVkDescriptorPool();

	VkDescriptorSetLayout layout = pCreateInfo.pLayout->GetVkDescriptorSetLayout();

	VkDescriptorSetAllocateInfo vkai = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	vkai.descriptorPool = mDescriptorPool;
	vkai.descriptorSetCount = 1;
	vkai.pSetLayouts = &layout;

	VkResult vkres = vkAllocateDescriptorSets(
		GetDevice()->GetVkDevice(),
		&vkai,
		&mDescriptorSet);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkAllocateDescriptorSets failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	// Allocate 32 entries initially
	const uint32_t count = 32;
	mWriteStore.resize(count);
	mImageInfoStore.resize(count);
	mBufferInfoStore.resize(count);
	mTexelBufferStore.resize(count);

	return SUCCESS;
}

void DescriptorSet::destroyApiObjects()
{
	if (mDescriptorSet)
	{
		vkFreeDescriptorSets(
			GetDevice()->GetVkDevice(),
			mDescriptorPool,
			1,
			mDescriptorSet);

		mDescriptorSet.Reset();
	}

	if (mDescriptorPool) {
		mDescriptorPool.Reset();
	}
}

Result DescriptorSetLayout::create(const DescriptorSetLayoutCreateInfo& pCreateInfo)
{
	// Bail if there's any binding overlaps - overlaps are not permitted to make D3D12 and Vulkan agreeable. Even though we use descriptor arrays in Vulkan, we do not allow the subsequent bindings to be occupied, to keep descriptor binding register occupancy consistent between Vulkan and D3D12.
	std::vector<RangeU32> ranges;
	const size_t               bindingCount = pCreateInfo.bindings.size();
	for (size_t i = 0; i < bindingCount; ++i) {
		const DescriptorBinding& binding = pCreateInfo.bindings[i];

		// Calculate range
		RangeU32 range = {};
		range.start = binding.binding;
		range.end = binding.binding + binding.arrayCount;

		size_t rangeCount = ranges.size();
		for (size_t j = 0; j < rangeCount; ++j) {
			bool overlaps = HasOverlapHalfOpen(range, ranges[j]);
			if (overlaps) {
				std::stringstream ss;
				ss << "[DESCRIPTOR BINDING RANGE ALIASES]: "
					<< "binding at entry " << i << " aliases with binding at entry " << j;
				ASSERT_MSG(false, ss.str());
				return ERROR_RANGE_ALIASING_NOT_ALLOWED;
			}
		}

		ranges.push_back(range);
	}

	Result ppxres = DeviceObject<DescriptorSetLayoutCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result DescriptorSetLayout::createApiObjects(const DescriptorSetLayoutCreateInfo& pCreateInfo)
{
	// Make sure the device has VK_KHR_push_descriptors if pushable is turned on
	if (pCreateInfo.flags.bits.pushable && (GetDevice()->GetMaxPushDescriptors() == 0))
	{
		ASSERT_MSG(false, "Descriptor set layout create info has pushable flag but device does not support VK_KHR_push_descriptor");
		return ERROR_REQUIRED_FEATURE_UNAVAILABLE;
	}

	std::vector<std::vector<VkSampler>>       immutableSamplers;
	std::vector<VkDescriptorSetLayoutBinding> vkBindings;
	std::vector<VkDescriptorBindingFlags>     vkBindingFlags;
	bool                                      hasBindingFlags = false;
	for (size_t i = 0; i < pCreateInfo.bindings.size(); ++i)
	{
		const DescriptorBinding& baseBinding = pCreateInfo.bindings[i];

		VkDescriptorSetLayoutBinding vkBinding = {};
		vkBinding.binding = baseBinding.binding;
		vkBinding.descriptorType = ToVkDescriptorType(baseBinding.type);
		vkBinding.descriptorCount = baseBinding.arrayCount;
		vkBinding.stageFlags = ToVkShaderStageFlags(baseBinding.shaderVisiblity);
		if (baseBinding.immutableSamplers.size() == 0) {
			vkBinding.pImmutableSamplers = nullptr;
		}
		else
		{
			ASSERT_MSG(baseBinding.arrayCount == baseBinding.immutableSamplers.size(), "Length of immutableSamplers must be 0 or descriptorCount.");
			auto& bindingImmutableSamplers = immutableSamplers.emplace_back();
			for (const auto& immutableSampler : baseBinding.immutableSamplers) {
				bindingImmutableSamplers.push_back(immutableSampler->GetVkSampler());
			}
			vkBinding.pImmutableSamplers = DataPtr(bindingImmutableSamplers);
		}
		vkBindings.push_back(vkBinding);

		CHECKED_CALL(validateDescriptorBindingFlags(baseBinding.flags));
		VkDescriptorBindingFlags vkBindingFlag = ToVkDescriptorBindingFlags(baseBinding.flags);
		vkBindingFlags.push_back(vkBindingFlag);
		if (baseBinding.flags != 0) {
			hasBindingFlags = true;
		}
	}

	VkDescriptorSetLayoutCreateInfo vkci = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr };
	vkci.bindingCount = CountU32(vkBindings);
	vkci.pBindings = DataPtr(vkBindings);

	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT vkBindingWrapper = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr };
	if (hasBindingFlags) {
		vkBindingWrapper.bindingCount = CountU32(vkBindingFlags);
		vkBindingWrapper.pBindingFlags = DataPtr(vkBindingFlags);
		vkci.pNext = &vkBindingWrapper;
	}

	if (pCreateInfo.flags.bits.pushable) {
		vkci.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	}
	else {
		vkci.flags |= VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
	}

	VkResult vkres = vkCreateDescriptorSetLayout(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mDescriptorSetLayout);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreateDescriptorSetLayout failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void DescriptorSetLayout::destroyApiObjects()
{
	if (mDescriptorSetLayout) {
		vkDestroyDescriptorSetLayout(GetDevice()->GetVkDevice(), mDescriptorSetLayout, nullptr);
		mDescriptorSetLayout.Reset();
	}
}

Result DescriptorSetLayout::validateDescriptorBindingFlags(const DescriptorBindingFlags& flags) const
{
	if (flags == 0) {
		return SUCCESS;
	}

	if (flags.bits.partiallyBound && !GetDevice()->PartialDescriptorBindingsSupported())
	{
		ASSERT_MSG(false, "Partial descriptor bindings aren't supported, but are enabled by flags!");
		return ERROR_REQUIRED_FEATURE_UNAVAILABLE;
	}

	if (!GetDevice()->HasDescriptorIndexingFeatures())
	{
		ASSERT_MSG(false, "Descriptor indexing features aren't supported, binding flags must be 0!");
		return ERROR_REQUIRED_FEATURE_UNAVAILABLE;
	}

	return SUCCESS;
}

#pragma endregion

#pragma region Shader

Result ShaderModule::createApiObjects(const ShaderModuleCreateInfo& pCreateInfo)
{
	VkShaderModuleCreateInfo vkci = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vkci.flags = 0;
	vkci.codeSize = static_cast<size_t>(pCreateInfo.size);
	vkci.pCode = reinterpret_cast<const uint32_t*>(pCreateInfo.pCode);

	VkResult vkres = vkCreateShaderModule(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mShaderModule);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateShaderModule failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void ShaderModule::destroyApiObjects()
{
	if (mShaderModule)
	{
		vkDestroyShaderModule(
			GetDevice()->GetVkDevice(),
			mShaderModule,
			nullptr);

		mShaderModule.Reset();
	}
}

#pragma endregion

#pragma region ShadingRate

namespace internal {

	uint32_t FDMShadingRateEncoder::encodeFragmentDensityImpl(uint8_t xDensity, uint8_t yDensity)
	{
		return (static_cast<uint16_t>(yDensity) << 8) | xDensity;
	}

	uint32_t FDMShadingRateEncoder::EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const
	{
		return encodeFragmentDensityImpl(xDensity, yDensity);
	}

	uint32_t FDMShadingRateEncoder::EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const
	{
		return encodeFragmentDensityImpl(255u / fragmentWidth, 255u / fragmentHeight);
	}

	uint32_t VRSShadingRateEncoder::rawEncode(uint8_t width, uint8_t height)
	{
		uint32_t widthEnc = std::min<uint8_t>(width, 4) / 2;
		uint32_t heightEnc = std::min<uint8_t>(height, 4) / 2;
		return (widthEnc << 2) | heightEnc;
	}

	void VRSShadingRateEncoder::Initialize(SampleCount sampleCount, const ShadingRateCapabilities& capabilities)
	{
		// Calculate the mapping from requested shading rates to supported shading rates.

		// Initialize all shading rate values with UINT8_MAX to mark as unsupported.
		std::fill(mMapRateToSupported.begin(), mMapRateToSupported.end(), UINT8_MAX);

		// Supported shading rates map to themselves.
		for (const auto& rate : capabilities.vrs.supportedRates) {
			if ((rate.sampleCountMask & sampleCount) != 0) {
				uint32_t encoded = rawEncode(rate.fragmentSize.width, rate.fragmentSize.height);
				mMapRateToSupported[encoded] = encoded;
			}
		}

		// Calculate the mapping for unsupported shading rates.
		for (uint32_t i = 0; i < 3; ++i) {
			uint32_t width = 1u << i;
			for (uint32_t j = 0; j < 3; ++j) {
				uint32_t height = 1u << j;
				uint32_t encoded = rawEncode(width, height);
				if (mMapRateToSupported[encoded] == UINT8_MAX) {
					// This shading rate is not supported. Find the largest supported shading rate where neither width nor height is greater than this fragment size.
					// Ties are broken lexicographically, e.g. if 2x2, 1x4 and 4x1 are supported, then 2x4 will be mapped to 2x2 but 4x2 will map to 4x1.

					ASSERT_MSG((width > 1) || (height > 1), "The 1x1 fragment size must always be supported");

					if (width == 1) {
						// Width is minimum, can only shrink height.
						mMapRateToSupported[encoded] = mMapRateToSupported[rawEncode(width, height - 1)];
					}
					else if (height == 1) {
						// Height is minimum, can only shrink width.
						mMapRateToSupported[encoded] = mMapRateToSupported[rawEncode(width - 1, height)];
					}
					else {
						// mMapRateToSupported is correctly filled in for smaller values of i and j, so find the largest supported shading rate with smaller width...
						uint8_t supportedSmallerWidth = mMapRateToSupported[rawEncode((i - 1) * 2, j * 2)];

						// ...and the largest supported shading rate with smaller height.
						uint8_t supportedSmallerHeight = mMapRateToSupported[rawEncode(i * 2, (j - 1) * 2)];

						// The largest supported shading rate
						mMapRateToSupported[encoded] = std::max(supportedSmallerWidth, supportedSmallerHeight);
					}
				}
			}
		}
	}

	uint32_t VRSShadingRateEncoder::encodeFragmentSizeImpl(uint8_t fragmentWidth, uint8_t fragmentHeight) const
	{
		uint32_t encoded = rawEncode(fragmentWidth, fragmentHeight);
		return mMapRateToSupported[encoded];
	}

	uint32_t VRSShadingRateEncoder::EncodeFragmentDensity(uint8_t xDensity, uint8_t yDensity) const
	{
		return encodeFragmentSizeImpl(255u / std::max<uint8_t>(xDensity, 1), 255u / std::max<uint8_t>(yDensity, 1));
	}

	uint32_t VRSShadingRateEncoder::EncodeFragmentSize(uint8_t fragmentWidth, uint8_t fragmentHeight) const
	{
		return encodeFragmentSizeImpl(fragmentWidth, fragmentHeight);
	}

} // namespace internal

std::unique_ptr<Bitmap> ShadingRatePattern::CreateBitmap() const
{
	auto bitmap = std::make_unique<Bitmap>();
	CHECKED_CALL(Bitmap::Create(
		GetAttachmentWidth(), GetAttachmentHeight(), GetBitmapFormat(), bitmap.get()));
	return bitmap;
}

Result ShadingRatePattern::LoadFromBitmap(Bitmap* bitmap)
{
	return vkrUtil::CopyBitmapToImage(GetDevice()->GetGraphicsQueue(), bitmap, mAttachmentImage, 0, 0, mAttachmentImage->GetInitialState(), mAttachmentImage->GetInitialState());
}

Bitmap::Format ShadingRatePattern::GetBitmapFormat() const
{
	switch (GetShadingRateMode()) {
	case SHADING_RATE_FDM:
		return Bitmap::FORMAT_RG_UINT8;
	case SHADING_RATE_VRS:
		return Bitmap::FORMAT_R_UINT8;
	default:
		return Bitmap::FORMAT_UNDEFINED;
	}
}

const ShadingRateEncoder* ShadingRatePattern::GetShadingRateEncoder() const
{
	return mShadingRateEncoder.get();
}

Result ShadingRatePattern::createApiObjects(const ShadingRatePatternCreateInfo& pCreateInfo)
{
	mShadingRateMode = pCreateInfo.shadingRateMode;
	mSampleCount = pCreateInfo.sampleCount;
	const auto& capabilities = GetDevice()->GetShadingRateCapabilities();

	ImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.usageFlags.bits.transferDst = true;
	imageCreateInfo.ownership = Ownership::Exclusive;
	Extent2D minTexelSize, maxTexelSize;
	switch (pCreateInfo.shadingRateMode)
	{
	case SHADING_RATE_FDM:
	{
		minTexelSize = capabilities.fdm.minTexelSize;
		maxTexelSize = capabilities.fdm.maxTexelSize;
		imageCreateInfo.format = Format::R8G8_UNORM;
		imageCreateInfo.usageFlags.bits.fragmentDensityMap = true;
		imageCreateInfo.initialState = ResourceState::FragmentDensityMapAttachment;
		mShadingRateEncoder = std::make_unique<internal::FDMShadingRateEncoder>();
	} break;
	case SHADING_RATE_VRS:
	{
		minTexelSize = capabilities.vrs.minTexelSize;
		maxTexelSize = capabilities.vrs.maxTexelSize;
		imageCreateInfo.format = Format::R8_UINT;
		imageCreateInfo.usageFlags.bits.fragmentShadingRateAttachment = true;
		imageCreateInfo.initialState = ResourceState::FragmentShadingRateAttachment;

		auto vrsShadingRateEncoder = std::make_unique<internal::VRSShadingRateEncoder>();
		vrsShadingRateEncoder->Initialize(mSampleCount, capabilities);
		mShadingRateEncoder = std::move(vrsShadingRateEncoder);
	} break;
	default:
		ASSERT_MSG(false, "Cannot create ShadingRatePattern for ShadingRateMode");
		return ERROR_FAILED;
	}

	if (pCreateInfo.texelSize.width == 0 && pCreateInfo.texelSize.height == 0) {
		mTexelSize = minTexelSize;
	}
	else {
		mTexelSize = pCreateInfo.texelSize;
	}

	ASSERT_MSG(
		mTexelSize.width >= minTexelSize.width,
		"Texel width (" + std::to_string(mTexelSize.width) + ") must be >= the minimum texel width from capabilities (" + std::to_string(minTexelSize.width) + ")");
	ASSERT_MSG(
		mTexelSize.height >= minTexelSize.height,
		"Texel height (" + std::to_string(mTexelSize.height) + ") must be >= the minimum texel height from capabilities (" + std::to_string(minTexelSize.height) + ")");
	ASSERT_MSG(
		mTexelSize.width <= maxTexelSize.width,
		"Texel width (" + std::to_string(mTexelSize.width) + ") must be <= the maximum texel width from capabilities (" + std::to_string(maxTexelSize.width) + ")");
	ASSERT_MSG(
		mTexelSize.height <= maxTexelSize.height,
		"Texel height (" + std::to_string(mTexelSize.height) + ") must be <= the maximum texel height from capabilities (" + std::to_string(maxTexelSize.height) + ")");

	imageCreateInfo.width = (pCreateInfo.framebufferSize.width + mTexelSize.width - 1) / mTexelSize.width;
	imageCreateInfo.height = (pCreateInfo.framebufferSize.height + mTexelSize.height - 1) / mTexelSize.height;
	imageCreateInfo.depth = 1;

	CHECKED_CALL(GetDevice()->CreateImage(imageCreateInfo, &mAttachmentImage));

	VkImageViewCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	vkci.flags = 0;
	vkci.image = mAttachmentImage->GetVkImage();
	vkci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	vkci.format = ToVkEnum(imageCreateInfo.format);
	vkci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	vkci.subresourceRange.baseMipLevel = 0;
	vkci.subresourceRange.levelCount = 1;
	vkci.subresourceRange.baseArrayLayer = 0;
	vkci.subresourceRange.layerCount = 1;

	VkResult vkres = vkCreateImageView(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mAttachmentView);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateImageView(ShadingRatePatternView) failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void ShadingRatePattern::destroyApiObjects()
{
	if (mAttachmentView)
	{
		vkDestroyImageView(
			GetDevice()->GetVkDevice(),
			mAttachmentView,
			nullptr);
		mAttachmentView.Reset();
	}
	if (mAttachmentImage)
	{
		GetDevice()->DestroyImage(mAttachmentImage);
		mAttachmentImage.Reset();
	}
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci)
{
	return CreateModifiedRenderPassCreateInfo()->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(const VkRenderPassCreateInfo2& vkci)
{
	return CreateModifiedRenderPassCreateInfo()->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode, const VkRenderPassCreateInfo& vkci)
{
	return CreateModifiedRenderPassCreateInfo(device, mode)->Initialize(vkci).Get();
}

std::shared_ptr<const VkRenderPassCreateInfo2> ShadingRatePattern::GetModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode, const VkRenderPassCreateInfo2& vkci)
{
	return CreateModifiedRenderPassCreateInfo(device, mode)->Initialize(vkci).Get();
}

std::shared_ptr<ShadingRatePattern::ModifiedRenderPassCreateInfo> ShadingRatePattern::CreateModifiedRenderPassCreateInfo(RenderDevice* device, ShadingRateMode mode)
{
	switch (mode) {
	case SHADING_RATE_FDM:
		return std::make_shared<ShadingRatePattern::FDMModifiedRenderPassCreateInfo>();
	case SHADING_RATE_VRS:
		return std::make_shared<ShadingRatePattern::VRSModifiedRenderPassCreateInfo>(device->GetShadingRateCapabilities());
	default:
		return nullptr;
	}
}

ShadingRatePattern::ModifiedRenderPassCreateInfo& ShadingRatePattern::ModifiedRenderPassCreateInfo::Initialize(const VkRenderPassCreateInfo& vkci)
{
	LoadVkRenderPassCreateInfo(vkci);
	UpdateRenderPassForShadingRateImplementation();
	return *this;
}

ShadingRatePattern::ModifiedRenderPassCreateInfo& ShadingRatePattern::ModifiedRenderPassCreateInfo::Initialize(const VkRenderPassCreateInfo2& vkci)
{
	LoadVkRenderPassCreateInfo2(vkci);
	UpdateRenderPassForShadingRateImplementation();
	return *this;
}

namespace {

	// Converts VkAttachmentDescription to VkAttachmentDescription2
	VkAttachmentDescription2 ToVkAttachmentDescription2(const VkAttachmentDescription& attachment)
	{
		VkAttachmentDescription2 attachment2 = { VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };
		attachment2.flags = attachment.flags;
		attachment2.format = attachment.format;
		attachment2.samples = attachment.samples;
		attachment2.loadOp = attachment.loadOp;
		attachment2.storeOp = attachment.storeOp;
		attachment2.stencilLoadOp = attachment.stencilLoadOp;
		attachment2.stencilStoreOp = attachment.stencilStoreOp;
		attachment2.initialLayout = attachment.initialLayout;
		attachment2.finalLayout = attachment.finalLayout;
		return attachment2;
	}

	// Converts VkAttachmentReference to VkAttachmentReference2
	VkAttachmentReference2 ToVkAttachmentReference2(const VkAttachmentReference& ref)
	{
		VkAttachmentReference2 ref2 = { VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2 };
		ref2.attachment = ref.attachment;
		ref2.layout = ref.layout;
		return ref2;
	}

	// Converts VkSubpassDependency to VkSubpassDependency2
	VkSubpassDependency2 ToVkSubpassDependency2(const VkSubpassDependency& dependency)
	{
		VkSubpassDependency2 dependency2 = { VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2 };
		dependency2.srcSubpass = dependency.srcSubpass;
		dependency2.dstSubpass = dependency.dstSubpass;
		dependency2.srcStageMask = dependency.srcStageMask;
		dependency2.dstStageMask = dependency.dstStageMask;
		dependency2.srcAccessMask = dependency.srcAccessMask;
		dependency2.dstAccessMask = dependency.dstAccessMask;
		dependency2.dependencyFlags = dependency.dependencyFlags;
		return dependency2;
	}

	// Copy an array to a vector and initialize an output count and data pointer.
	template <typename T>
	void CopyArrayWithVectorStorage(uint32_t inCount, const T* inData, std::vector<T>& vec, uint32_t& outCount, T const*& outData)
	{
		vec.clear();
		if (inCount == 0) {
			outCount = 0;
			outData = nullptr;
		}
		vec.resize(inCount);
		std::copy_n(inData, inCount, vec.begin());
		outCount = CountU32(vec);
		outData = DataPtr(vec);
	}

	// Convert the elements in an array, store in a vector, and initialize an output count and data pointer.
	template <typename T1, typename T2, typename Conv>
	void ConvertArrayWithVectorStorage(uint32_t inCount, const T1* inData, std::vector<T2>& vec, uint32_t& outCount, T2 const*& outData, Conv&& conv)
	{
		vec.clear();
		if (inCount == 0) {
			outCount = 0;
			outData = nullptr;
		}
		vec.resize(inCount);
		for (uint32_t i = 0; i < inCount; ++i) {
			vec[i] = conv(inData[i]);
		}
		outCount = CountU32(vec);
		outData = DataPtr(vec);
	}

} // namespace

void ShadingRatePattern::ModifiedRenderPassCreateInfo::LoadVkRenderPassCreateInfo(const VkRenderPassCreateInfo& vkci)
{
	auto& vkci2 = mVkRenderPassCreateInfo2;
	vkci2.pNext = vkci.pNext;
	vkci2.flags = vkci.flags;

	ConvertArrayWithVectorStorage(vkci.attachmentCount, vkci.pAttachments, mAttachments, vkci2.attachmentCount, vkci2.pAttachments, &ToVkAttachmentDescription2);
	ConvertArrayWithVectorStorage(vkci.subpassCount, vkci.pSubpasses, mSubpasses, vkci2.subpassCount, vkci2.pSubpasses, [this](const VkSubpassDescription& subpass) {
		VkSubpassDescription2 subpass2 = { VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2 };
		subpass2.flags = subpass.flags;
		subpass2.pipelineBindPoint = subpass.pipelineBindPoint;

		auto& subpassAttachments = mSubpassAttachments.emplace_back();
		ConvertArrayWithVectorStorage(subpass.inputAttachmentCount, subpass.pInputAttachments, subpassAttachments.inputAttachments, subpass2.inputAttachmentCount, subpass2.pInputAttachments, &ToVkAttachmentReference2);
		ConvertArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pColorAttachments, subpassAttachments.colorAttachments, subpass2.colorAttachmentCount, subpass2.pColorAttachments, &ToVkAttachmentReference2);
		if (!IsNull(subpass.pResolveAttachments)) {
			uint32_t resolveAttachmentCount = 0;
			ConvertArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pResolveAttachments, subpassAttachments.resolveAttachments, resolveAttachmentCount, subpass2.pResolveAttachments, &ToVkAttachmentReference2);
		}
		if (!IsNull(subpass.pDepthStencilAttachment)) {
			auto& depthStencilAttachment = subpassAttachments.depthStencilAttachment;
			depthStencilAttachment = ToVkAttachmentReference2(*subpass.pDepthStencilAttachment);
			subpass2.pDepthStencilAttachment = &depthStencilAttachment;
		}
		CopyArrayWithVectorStorage(subpass.preserveAttachmentCount, subpass.pPreserveAttachments, subpassAttachments.preserveAttachments, subpass2.preserveAttachmentCount, subpass2.pPreserveAttachments);
		return subpass2;
		});
	ConvertArrayWithVectorStorage(vkci.dependencyCount, vkci.pDependencies, mDependencies, vkci2.dependencyCount, vkci2.pDependencies, &ToVkSubpassDependency2);
}

void ShadingRatePattern::ModifiedRenderPassCreateInfo::LoadVkRenderPassCreateInfo2(const VkRenderPassCreateInfo2& vkci)
{
	VkRenderPassCreateInfo2& vkci2 = mVkRenderPassCreateInfo2;
	vkci2 = vkci;

	CopyArrayWithVectorStorage(vkci.attachmentCount, vkci.pAttachments, mAttachments, vkci2.attachmentCount, vkci2.pAttachments);
	ConvertArrayWithVectorStorage(vkci.subpassCount, vkci.pSubpasses, mSubpasses, vkci2.subpassCount, vkci2.pSubpasses, [this](const VkSubpassDescription2& subpass) {
		VkSubpassDescription2 subpass2 = subpass;

		auto& subpassAttachments = mSubpassAttachments.emplace_back();
		CopyArrayWithVectorStorage(subpass.inputAttachmentCount, subpass.pInputAttachments, subpassAttachments.inputAttachments, subpass2.inputAttachmentCount, subpass2.pInputAttachments);
		CopyArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pColorAttachments, subpassAttachments.colorAttachments, subpass2.colorAttachmentCount, subpass2.pColorAttachments);
		if (!IsNull(subpass.pResolveAttachments)) {
			uint32_t resolveAttachmentCount = 0;
			CopyArrayWithVectorStorage(subpass.colorAttachmentCount, subpass.pResolveAttachments, subpassAttachments.resolveAttachments, resolveAttachmentCount, subpass2.pResolveAttachments);
		}
		if (!IsNull(subpass.pDepthStencilAttachment)) {
			auto& depthStencilAttachment = subpassAttachments.depthStencilAttachment;
			depthStencilAttachment = *subpass.pDepthStencilAttachment;
			subpass2.pDepthStencilAttachment = &depthStencilAttachment;
		}
		CopyArrayWithVectorStorage(subpass.preserveAttachmentCount, subpass.pPreserveAttachments, subpassAttachments.preserveAttachments, subpass2.preserveAttachmentCount, subpass2.pPreserveAttachments);
		return subpass2;
		});
	CopyArrayWithVectorStorage(vkci.dependencyCount, vkci.pDependencies, mDependencies, vkci2.dependencyCount, vkci2.pDependencies);
}

void ShadingRatePattern::FDMModifiedRenderPassCreateInfo::UpdateRenderPassForShadingRateImplementation()
{
	VkRenderPassCreateInfo2& vkci = mVkRenderPassCreateInfo2;

	VkAttachmentDescription2 densityMapDesc = { VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };
	densityMapDesc.flags = 0;
	densityMapDesc.format = VK_FORMAT_R8G8_UNORM;
	densityMapDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	densityMapDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	densityMapDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	densityMapDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	densityMapDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	densityMapDesc.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	densityMapDesc.finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	mAttachments.push_back(densityMapDesc);

	mFdmInfo.fragmentDensityMapAttachment.layout = VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
	mFdmInfo.fragmentDensityMapAttachment.attachment = mAttachments.size() - 1;

	InsertPNext(vkci, mFdmInfo);

	vkci.attachmentCount = CountU32(mAttachments);
	vkci.pAttachments = DataPtr(mAttachments);
}

void ShadingRatePattern::VRSModifiedRenderPassCreateInfo::UpdateRenderPassForShadingRateImplementation()
{
	VkRenderPassCreateInfo2& vkci = mVkRenderPassCreateInfo2;

	VkAttachmentDescription2 vrsAttachmentDesc = { VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2 };
	vrsAttachmentDesc.format = VK_FORMAT_R8_UINT;
	vrsAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	vrsAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
	vrsAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vrsAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	vrsAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	vrsAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	vrsAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
	mAttachments.push_back(vrsAttachmentDesc);

	vkci.attachmentCount = CountU32(mAttachments);
	vkci.pAttachments = DataPtr(mAttachments);

	mVrsAttachmentRef.attachment = mAttachments.size() - 1;
	mVrsAttachmentRef.layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;

	mVrsAttachmentInfo.pFragmentShadingRateAttachment = &mVrsAttachmentRef;
	mVrsAttachmentInfo.shadingRateAttachmentTexelSize = {
		mCapabilities.vrs.minTexelSize.width,
		mCapabilities.vrs.minTexelSize.height,
	};

	for (auto& subpass : mSubpasses) {
		InsertPNext(subpass, mVrsAttachmentInfo);
	}
}

#pragma endregion

#pragma region Shading Rate Util

void FillShadingRateUniformFragmentSize(ShadingRatePatternPtr pattern, uint32_t fragmentWidth, uint32_t fragmentHeight, Bitmap* bitmap)
{
	FillShadingRateUniformFragmentDensity(pattern, 255u / fragmentWidth, 255u / fragmentHeight, bitmap);
}

void FillShadingRateUniformFragmentDensity(ShadingRatePatternPtr pattern, uint32_t xDensity, uint32_t yDensity, Bitmap* bitmap)
{
	auto     encoder = pattern->GetShadingRateEncoder();
	uint32_t encoded = encoder->EncodeFragmentDensity(xDensity, yDensity);
	uint8_t* encodedBytes = reinterpret_cast<uint8_t*>(&encoded);
	bitmap->Fill<uint8_t>(encodedBytes[0], encodedBytes[1], 0, 0);
}

void FillShadingRateRadial(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap)
{
	auto encoder = pattern->GetShadingRateEncoder();
	scale /= std::min<uint32_t>(bitmap->GetWidth(), bitmap->GetHeight());
	for (uint32_t j = 0; j < bitmap->GetHeight(); ++j) {
		float    y = scale * (2.0 * j - bitmap->GetHeight());
		uint8_t* addr = bitmap->GetPixel8u(0, j);
		for (uint32_t i = 0; i < bitmap->GetWidth(); ++i, addr += bitmap->GetPixelStride()) {
			float          x = scale * (2.0 * i - bitmap->GetWidth());
			float          r2 = x * x + y * y;
			uint32_t       encoded = encoder->EncodeFragmentSize(r2 + 1, r2 + 1);
			const uint8_t* encodedBytes = reinterpret_cast<const uint8_t*>(&encoded);
			for (uint32_t k = 0; k < bitmap->GetChannelCount(); ++k) {
				addr[k] = encodedBytes[k];
			}
		}
	}
}

void FillShadingRateAnisotropic(ShadingRatePatternPtr pattern, float scale, Bitmap* bitmap)
{
	auto encoder = pattern->GetShadingRateEncoder();
	scale /= std::min<uint32_t>(bitmap->GetWidth(), bitmap->GetHeight());
	for (uint32_t j = 0; j < bitmap->GetHeight(); ++j) {
		float    y = scale * (2.0 * j - bitmap->GetHeight());
		uint8_t* addr = bitmap->GetPixel8u(0, j);
		for (uint32_t i = 0; i < bitmap->GetWidth(); ++i, addr += bitmap->GetPixelStride()) {
			float          x = scale * (2.0 * i - bitmap->GetWidth());
			uint32_t       encoded = encoder->EncodeFragmentSize(x * x + 1, y * y + 1);
			const uint8_t* encodedBytes = reinterpret_cast<const uint8_t*>(&encoded);
			for (uint32_t k = 0; k < bitmap->GetChannelCount(); ++k) {
				addr[k] = encodedBytes[k];
			}
		}
	}
}

#pragma endregion

#pragma region Mesh

MeshCreateInfo::MeshCreateInfo(const Geometry& geometry)
{
	this->indexType = geometry.GetIndexType();
	this->indexCount = geometry.GetIndexCount();
	this->vertexCount = geometry.GetVertexCount();
	this->vertexBufferCount = geometry.GetVertexBufferCount();
	this->memoryUsage = MemoryUsage::GPUOnly;
	std::memset(&this->vertexBuffers, 0, MaxVertexBindings * sizeof(MeshVertexBufferDescription));

	const uint32_t bindingCount = geometry.GetVertexBindingCount();
	for (uint32_t bindingIdx = 0; bindingIdx < bindingCount; ++bindingIdx) {
		const uint32_t attrCount = geometry.GetVertexBinding(bindingIdx)->GetAttributeCount();

		this->vertexBuffers[bindingIdx].attributeCount = attrCount;
		for (uint32_t attrIdx = 0; attrIdx < attrCount; ++attrIdx) {
			const VertexAttribute* pAttribute = nullptr;
			geometry.GetVertexBinding(bindingIdx)->GetAttribute(attrIdx, &pAttribute);

			this->vertexBuffers[bindingIdx].attributes[attrIdx].format = pAttribute->format;
			this->vertexBuffers[bindingIdx].attributes[attrIdx].stride = 0; // Calculated later
			this->vertexBuffers[bindingIdx].attributes[attrIdx].vertexSemantic = pAttribute->semantic;
		}

		this->vertexBuffers[bindingIdx].vertexInputRate = VERTEX_INPUT_RATE_VERTEX;
	}
}

// -------------------------------------------------------------------------------------------------
// Mesh
// -------------------------------------------------------------------------------------------------
Result Mesh::createApiObjects(const MeshCreateInfo& pCreateInfo)
{
	// Bail if both index count and vertex count are 0
	if ((pCreateInfo.indexCount == 0) && (pCreateInfo.vertexCount) == 0) {
		return Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION;
	}
	// Bail if vertexCount is not 0 but vertex buffer count is 0
	if ((pCreateInfo.vertexCount > 0) && (pCreateInfo.vertexBufferCount == 0)) {
		return Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION;
	}

	// Index buffer
	if (pCreateInfo.indexCount > 0) {
		// Bail if index type doesn't make sense
		if ((pCreateInfo.indexType != IndexType::Uint16) && (pCreateInfo.indexType != IndexType::Uint32)) {
			return Result::ERROR_GRFX_INVALID_INDEX_TYPE;
		}

		BufferCreateInfo createInfo = {};
		createInfo.size = pCreateInfo.indexCount * IndexTypeSize(pCreateInfo.indexType);
		createInfo.usageFlags.bits.indexBuffer = true;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.memoryUsage = pCreateInfo.memoryUsage;
		createInfo.initialState = ResourceState::General;
		createInfo.ownership = Ownership::Reference;

		auto ppxres = GetDevice()->CreateBuffer(createInfo, &mIndexBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "create mesh index buffer failed");
			return ppxres;
		}
	}

	// Vertex buffers
	if (pCreateInfo.vertexCount > 0) {
		mVertexBuffers.resize(pCreateInfo.vertexBufferCount);

		// Iterate through all the vertex buffer descriptions and create appropriately sized buffers
		for (uint32_t vbIdx = 0; vbIdx < pCreateInfo.vertexBufferCount; ++vbIdx) {
			// Copy vertex buffer description
			std::memcpy(&mVertexBuffers[vbIdx].second, &pCreateInfo.vertexBuffers[vbIdx], sizeof(MeshVertexBufferDescription));

			auto& vertexBufferDesc = mVertexBuffers[vbIdx].second;
			// Bail if attribute count is 0
			if (vertexBufferDesc.attributeCount == 0) {
				return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT;
			}

			// Calculate vertex stride (if needed) and attribute offsets
			bool calculateVertexStride = (mVertexBuffers[vbIdx].second.stride == 0);
			for (uint32_t attrIdx = 0; attrIdx < vertexBufferDesc.attributeCount; ++attrIdx) {
				auto& attr = vertexBufferDesc.attributes[attrIdx];
				Format format = attr.format;
				if (format == Format::Undefined) {
					return Result::ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED;
				}

				auto* pFormatDesc = GetFormatDescription(format);
				// Bail if the format's size is zero
				if (pFormatDesc->bytesPerTexel == 0) {
					return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE;
				}
				// Bail if the attribute stride is NOT 0 and less than the format size
				if ((attr.stride != 0) && (attr.stride < pFormatDesc->bytesPerTexel)) {
					return Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE;
				}

				// Calculate stride if needed
				if (attr.stride == 0) {
					attr.stride = pFormatDesc->bytesPerTexel;
				}

				// Set offset
				attr.offset = mVertexBuffers[vbIdx].second.stride;

				// Increment vertex stride
				if (calculateVertexStride) {
					mVertexBuffers[vbIdx].second.stride += attr.stride;
				}
			}

			BufferCreateInfo createInfo = {};
			createInfo.size = pCreateInfo.vertexCount * mVertexBuffers[vbIdx].second.stride;
			createInfo.usageFlags.bits.vertexBuffer = true;
			createInfo.usageFlags.bits.transferDst = true;
			createInfo.memoryUsage = pCreateInfo.memoryUsage;
			createInfo.initialState = ResourceState::General;
			createInfo.ownership = Ownership::Reference;

			auto ppxres = GetDevice()->CreateBuffer(createInfo, &mVertexBuffers[vbIdx].first);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "create mesh vertex buffer failed");
				return ppxres;
			}
		}
	}

	// Derived vertex bindings
	{
		uint32_t location = 0;
		for (uint32_t bufferIndex = 0; bufferIndex < GetVertexBufferCount(); ++bufferIndex) {
			auto pBufferDesc = GetVertexBufferDescription(bufferIndex);

			VertexBinding binding = VertexBinding(bufferIndex, pBufferDesc->vertexInputRate);
			binding.SetBinding(bufferIndex);

			for (uint32_t attrIdx = 0; attrIdx < pBufferDesc->attributeCount; ++attrIdx) {
				auto& srcAttr = pBufferDesc->attributes[attrIdx];

				std::string semanticName = SEMANTIC_NAME_CUSTOM;
				// clang-format off
				switch (srcAttr.vertexSemantic) {
				default: break;
				case VERTEX_SEMANTIC_POSITION: semanticName = SEMANTIC_NAME_POSITION;  break;
				case VERTEX_SEMANTIC_NORMAL: semanticName = SEMANTIC_NAME_NORMAL;    break;
				case VERTEX_SEMANTIC_COLOR: semanticName = SEMANTIC_NAME_COLOR;     break;
				case VERTEX_SEMANTIC_TEXCOORD: semanticName = SEMANTIC_NAME_TEXCOORD;  break;
				case VERTEX_SEMANTIC_TANGENT: semanticName = SEMANTIC_NAME_TANGENT;   break;
				case VERTEX_SEMANTIC_BITANGENT: semanticName = SEMANTIC_NAME_BITANGENT; break;
				}
				// clang-format on

				VertexAttribute attr = {};
				attr.semanticName = semanticName;
				attr.location = location;
				attr.format = srcAttr.format;
				attr.binding = bufferIndex;
				attr.offset = srcAttr.offset;
				attr.inputRate = pBufferDesc->vertexInputRate;
				attr.semantic = srcAttr.vertexSemantic;

				binding.AppendAttribute(attr);

				++location;
			}

			binding.SetStride(pBufferDesc->stride);

			mDerivedVertexBindings.push_back(binding);
		}
	}

	return Result::SUCCESS;
}

void Mesh::destroyApiObjects()
{
	if (mIndexBuffer) {
		GetDevice()->DestroyBuffer(mIndexBuffer);
	}

	for (auto& elem : mVertexBuffers) {
		GetDevice()->DestroyBuffer(elem.first);
	}
	mVertexBuffers.clear();
}

BufferPtr Mesh::GetVertexBuffer(uint32_t index) const
{
	const uint32_t vertexBufferCount = CountU32(mVertexBuffers);
	if (index >= vertexBufferCount) {
		return nullptr;
	}
	return mVertexBuffers[index].first;
}

const MeshVertexBufferDescription* Mesh::GetVertexBufferDescription(uint32_t index) const
{
	const uint32_t vertexBufferCount = CountU32(mVertexBuffers);
	if (index >= vertexBufferCount) {
		return nullptr;
	}
	return &mVertexBuffers[index].second;
}

#pragma endregion

#pragma region Pipeline

BlendAttachmentState BlendAttachmentState::BlendModeAdditive()
{
	BlendAttachmentState state = {};
	state.blendEnable = true;
	state.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstColorBlendFactor = BLEND_FACTOR_ONE;
	state.colorBlendOp = BLEND_OP_ADD;
	state.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstAlphaBlendFactor = BLEND_FACTOR_ONE;
	state.alphaBlendOp = BLEND_OP_ADD;
	state.colorWriteMask = ColorComponentFlags::RGBA();

	return state;
}

BlendAttachmentState BlendAttachmentState::BlendModeAlpha()
{
	BlendAttachmentState state = {};
	state.blendEnable = true;
	state.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.colorBlendOp = BLEND_OP_ADD;
	state.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.alphaBlendOp = BLEND_OP_ADD;
	state.colorWriteMask = ColorComponentFlags::RGBA();

	return state;
}

BlendAttachmentState BlendAttachmentState::BlendModeOver()
{
	BlendAttachmentState state = {};
	state.blendEnable = true;
	state.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.colorBlendOp = BLEND_OP_ADD;
	state.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
	state.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.alphaBlendOp = BLEND_OP_ADD;
	state.colorWriteMask = ColorComponentFlags::RGBA();

	return state;
}

BlendAttachmentState BlendAttachmentState::BlendModeUnder()
{
	BlendAttachmentState state = {};
	state.blendEnable = true;
	state.srcColorBlendFactor = BLEND_FACTOR_DST_ALPHA;
	state.dstColorBlendFactor = BLEND_FACTOR_ONE;
	state.colorBlendOp = BLEND_OP_ADD;
	state.srcAlphaBlendFactor = BLEND_FACTOR_ZERO;
	state.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.alphaBlendOp = BLEND_OP_ADD;
	state.colorWriteMask = ColorComponentFlags::RGBA();

	return state;
}

BlendAttachmentState BlendAttachmentState::BlendModePremultAlpha()
{
	BlendAttachmentState state = {};
	state.blendEnable = true;
	state.srcColorBlendFactor = BLEND_FACTOR_ONE;
	state.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.colorBlendOp = BLEND_OP_ADD;
	state.srcAlphaBlendFactor = BLEND_FACTOR_ONE;
	state.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	state.alphaBlendOp = BLEND_OP_ADD;
	state.colorWriteMask = ColorComponentFlags::RGBA();

	return state;
}

namespace internal
{

	void FillOutGraphicsPipelineCreateInfo(const GraphicsPipelineCreateInfo2& pSrcCreateInfo, GraphicsPipelineCreateInfo* pDstCreateInfo)
	{
		// Set to default values
		*pDstCreateInfo = {};

		pDstCreateInfo->dynamicRenderPass = pSrcCreateInfo.dynamicRenderPass;

		// Shaders
		pDstCreateInfo->VS = pSrcCreateInfo.VS;
		pDstCreateInfo->PS = pSrcCreateInfo.PS;

		// Vertex input
		{
			pDstCreateInfo->vertexInputState.bindingCount = pSrcCreateInfo.vertexInputState.bindingCount;
			for (uint32_t i = 0; i < pDstCreateInfo->vertexInputState.bindingCount; ++i) {
				pDstCreateInfo->vertexInputState.bindings[i] = pSrcCreateInfo.vertexInputState.bindings[i];
			}
		}

		// Input aasembly
		{
			pDstCreateInfo->inputAssemblyState.topology = pSrcCreateInfo.topology;
		}

		// Raster
		{
			pDstCreateInfo->rasterState.polygonMode = pSrcCreateInfo.polygonMode;
			pDstCreateInfo->rasterState.cullMode = pSrcCreateInfo.cullMode;
			pDstCreateInfo->rasterState.frontFace = pSrcCreateInfo.frontFace;
		}

		// Depth/stencil
		{
			pDstCreateInfo->depthStencilState.depthTestEnable = pSrcCreateInfo.depthReadEnable;
			pDstCreateInfo->depthStencilState.depthWriteEnable = pSrcCreateInfo.depthWriteEnable;
			pDstCreateInfo->depthStencilState.depthCompareOp = pSrcCreateInfo.depthCompareOp;
			pDstCreateInfo->depthStencilState.depthBoundsTestEnable = false;
			pDstCreateInfo->depthStencilState.minDepthBounds = 0.0f;
			pDstCreateInfo->depthStencilState.maxDepthBounds = 1.0f;
			pDstCreateInfo->depthStencilState.stencilTestEnable = false;
			pDstCreateInfo->depthStencilState.front = {};
			pDstCreateInfo->depthStencilState.back = {};
		}

		// Color blend
		{
			pDstCreateInfo->colorBlendState.blendAttachmentCount = pSrcCreateInfo.outputState.renderTargetCount;
			for (uint32_t i = 0; i < pDstCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
				switch (pSrcCreateInfo.blendModes[i]) {
				default: break;

				case BLEND_MODE_ADDITIVE: {
					pDstCreateInfo->colorBlendState.blendAttachments[i] = BlendAttachmentState::BlendModeAdditive();
				} break;

				case BLEND_MODE_ALPHA: {
					pDstCreateInfo->colorBlendState.blendAttachments[i] = BlendAttachmentState::BlendModeAlpha();
				} break;

				case BLEND_MODE_OVER: {
					pDstCreateInfo->colorBlendState.blendAttachments[i] = BlendAttachmentState::BlendModeOver();
				} break;

				case BLEND_MODE_UNDER: {
					pDstCreateInfo->colorBlendState.blendAttachments[i] = BlendAttachmentState::BlendModeUnder();
				} break;

				case BLEND_MODE_PREMULT_ALPHA: {
					pDstCreateInfo->colorBlendState.blendAttachments[i] = BlendAttachmentState::BlendModePremultAlpha();
				} break;
				}
				pDstCreateInfo->colorBlendState.blendAttachments[i].colorWriteMask = ColorComponentFlags::RGBA();
			}
		}

		// Output
		{
			pDstCreateInfo->outputState.renderTargetCount = pSrcCreateInfo.outputState.renderTargetCount;
			for (uint32_t i = 0; i < pDstCreateInfo->outputState.renderTargetCount; ++i) {
				pDstCreateInfo->outputState.renderTargetFormats[i] = pSrcCreateInfo.outputState.renderTargetFormats[i];
			}

			pDstCreateInfo->outputState.depthStencilFormat = pSrcCreateInfo.outputState.depthStencilFormat;
		}

		// Shading rate mode
		pDstCreateInfo->shadingRateMode = pSrcCreateInfo.shadingRateMode;

		// Pipeline internface
		pDstCreateInfo->pPipelineInterface = pSrcCreateInfo.pPipelineInterface;

		// MultiView details
		pDstCreateInfo->multiViewState = pSrcCreateInfo.multiViewState;
	}

} // namespace internal

Result ComputePipeline::create(const ComputePipelineCreateInfo& pCreateInfo)
{
	if (IsNull(pCreateInfo.pPipelineInterface)) {
		ASSERT_MSG(false, "pipeline interface is null (compute pipeline)");
		return ERROR_GRFX_OPERATION_NOT_PERMITTED;
	}

	Result ppxres = DeviceObject<ComputePipelineCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result ComputePipeline::createApiObjects(const ComputePipelineCreateInfo& pCreateInfo)
{
	VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	ssci.flags = 0;
	ssci.pSpecializationInfo = nullptr;
	ssci.pName = pCreateInfo.CS.entryPoint.c_str();
	ssci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	ssci.module = pCreateInfo.CS.pModule->GetVkShaderModule();

	VkComputePipelineCreateInfo vkci = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	vkci.flags = 0;
	vkci.stage = ssci;
	vkci.layout = pCreateInfo.pPipelineInterface->GetVkPipelineLayout();
	vkci.basePipelineHandle = VK_NULL_HANDLE;
	vkci.basePipelineIndex = 0;

	VkResult vkres = vkCreateComputePipelines(
		GetDevice()->GetVkDevice(),
		VK_NULL_HANDLE,
		1,
		&vkci,
		nullptr,
		&mPipeline);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateComputePipelines failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void ComputePipeline::destroyApiObjects()
{
	if (mPipeline) {
		vkDestroyPipeline(GetDevice()->GetVkDevice(), mPipeline, nullptr);
		mPipeline.Reset();
	}
}

Result GraphicsPipeline::create(const GraphicsPipelineCreateInfo& pCreateInfo)
{
	//// Checked binding range
	// for (uint32_t i = 0; i < pCreateInfo.vertexInputState.attributeCount; ++i) {
	//     const VertexAttribute& attribute = pCreateInfo.vertexInputState.attributes[i];
	//     if (attribute.binding >= MAX_VERTEX_BINDINGS) {
	//         ASSERT_MSG(false, "binding exceeds PPX_MAX_VERTEX_ATTRIBUTES");
	//         return ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED;
	//     }
	//     if (attribute.format == Format::Undefined) {
	//         ASSERT_MSG(false, "vertex attribute format is undefined");
	//         return ERROR_GRFX_VERTEX_ATTRIBUTE_FROMAT_UNDEFINED;
	//     }
	// }
	//
	//// Build input bindings
	//{
	//    // Collect attributes into bindings
	//    for (uint32_t i = 0; i < pCreateInfo.vertexInputState.attributeCount; ++i) {
	//        const VertexAttribute& attribute = pCreateInfo.vertexInputState.attributes[i];
	//
	//        auto it = std::find_if(
	//            std::begin(mInputBindings),
	//            std::end(mInputBindings),
	//            [attribute](const VertexInputBinding& elem) -> bool {
	//            bool isSame = attribute.binding == elem.binding;
	//            return isSame; });
	//        if (it != std::end(mInputBindings)) {
	//            it->attributes.push_back(attribute);
	//        }
	//        else {
	//            VertexInputBinding set = {attribute.binding};
	//            mInputBindings.push_back(set);
	//            mInputBindings.back().attributes.push_back(attribute);
	//        }
	//    }
	//
	//    // Calculate offsets and stride
	//    for (auto& elem : mInputBindings) {
	//        elem.CalculateOffsetsAndStride();
	//    }
	//
	//    // Check classifactions
	//    for (auto& elem : mInputBindings) {
	//        uint32_t inputRateVertexCount   = 0;
	//        uint32_t inputRateInstanceCount = 0;
	//        for (auto& attr : elem.attributes) {
	//            inputRateVertexCount += (attr.inputRate == VERTEX_INPUT_RATE_VERTEX) ? 1 : 0;
	//            inputRateInstanceCount += (attr.inputRate == VERETX_INPUT_RATE_INSTANCE) ? 1 : 0;
	//        }
	//        // Cannot mix input rates
	//        if ((inputRateInstanceCount > 0) && (inputRateVertexCount > 0)) {
	//            ASSERT_MSG(false, "cannot mix vertex input rates in same binding");
	//            return ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES;
	//        }
	//    }
	//}

	if (IsNull(pCreateInfo.pPipelineInterface)) {
		ASSERT_MSG(false, "pipeline interface is null (graphics pipeline)");
		return ERROR_GRFX_OPERATION_NOT_PERMITTED;
	}

	//if (pCreateInfo.dynamicRenderPass && !GetDevice()->DynamicRenderingSupported()) {
	//	ASSERT_MSG(false, "Cannot create a pipeline with dynamic render pass, dynamic rendering is not supported.");
	//	return ERROR_GRFX_OPERATION_NOT_PERMITTED;
	//}

	Result ppxres = DeviceObject<GraphicsPipelineCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result GraphicsPipeline::initializeShaderStages(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	std::vector<VkPipelineShaderStageCreateInfo>& shaderStages,
	VkGraphicsPipelineCreateInfo& vkCreateInfo)
{
	// VS
	if (!IsNull(pCreateInfo.VS.pModule))
	{
		const ShaderModule* pModule = pCreateInfo.VS.pModule;

		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags = 0;
		ssci.pSpecializationInfo = nullptr;
		ssci.pName = pCreateInfo.VS.entryPoint.c_str();
		ssci.stage = VK_SHADER_STAGE_VERTEX_BIT;
		ssci.module = pModule->GetVkShaderModule();
		shaderStages.push_back(ssci);
	}

	// HS
	if (!IsNull(pCreateInfo.HS.pModule))
	{
		const ShaderModule* pModule = pCreateInfo.HS.pModule;

		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags = 0;
		ssci.pSpecializationInfo = nullptr;
		ssci.pName = pCreateInfo.HS.entryPoint.c_str();
		ssci.stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		ssci.module = pModule->GetVkShaderModule();
		shaderStages.push_back(ssci);
	}

	// DS
	if (!IsNull(pCreateInfo.DS.pModule))
	{
		const ShaderModule* pModule = pCreateInfo.DS.pModule;

		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags = 0;
		ssci.pSpecializationInfo = nullptr;
		ssci.pName = pCreateInfo.DS.entryPoint.c_str();
		ssci.stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		ssci.module = pModule->GetVkShaderModule();
		shaderStages.push_back(ssci);
	}

	// GS
	if (!IsNull(pCreateInfo.GS.pModule))
	{
		const ShaderModule* pModule = pCreateInfo.GS.pModule;

		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags = 0;
		ssci.pSpecializationInfo = nullptr;
		ssci.pName = pCreateInfo.GS.entryPoint.c_str();
		ssci.stage = VK_SHADER_STAGE_GEOMETRY_BIT;
		ssci.module = pModule->GetVkShaderModule();
		shaderStages.push_back(ssci);
	}

	// PS
	if (!IsNull(pCreateInfo.PS.pModule))
	{
		const ShaderModule* pModule = pCreateInfo.PS.pModule;

		VkPipelineShaderStageCreateInfo ssci = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		ssci.flags = 0;
		ssci.pSpecializationInfo = nullptr;
		ssci.pName = pCreateInfo.PS.entryPoint.c_str();
		ssci.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		ssci.module = pModule->GetVkShaderModule();
		shaderStages.push_back(ssci);
	}

	return SUCCESS;
}

Result GraphicsPipeline::initializeVertexInput(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	std::vector<VkVertexInputAttributeDescription>& vkAttributes,
	std::vector<VkVertexInputBindingDescription>& vkBindings,
	VkPipelineVertexInputStateCreateInfo& stateCreateInfo)
{
	// Fill out Vulkan attributes and bindings
	for (uint32_t bindingIndex = 0; bindingIndex < pCreateInfo.vertexInputState.bindingCount; ++bindingIndex)
	{
		const VertexBinding& binding = pCreateInfo.vertexInputState.bindings[bindingIndex];
		// Iterate each attribute in the binding
		const uint32_t attributeCount = binding.GetAttributeCount();
		for (uint32_t attributeIndex = 0; attributeIndex < attributeCount; ++attributeIndex) 
		{
			// This should be safe since there's no modifications to the index
			const VertexAttribute* pAttribute = nullptr;
			binding.GetAttribute(attributeIndex, &pAttribute);

			VkVertexInputAttributeDescription vkAttribute = {};
			vkAttribute.location = pAttribute->location;
			vkAttribute.binding = pAttribute->binding;
			vkAttribute.format = ToVkEnum(pAttribute->format);
			vkAttribute.offset = pAttribute->offset;
			vkAttributes.push_back(vkAttribute);
		}

		VkVertexInputBindingDescription vkBinding = {};
		vkBinding.binding = binding.GetBinding();
		vkBinding.stride = binding.GetStride();
		vkBinding.inputRate = ToVkVertexInputRate(binding.GetInputRate());
		vkBindings.push_back(vkBinding);
	}

	stateCreateInfo.flags = 0;
	stateCreateInfo.vertexBindingDescriptionCount = CountU32(vkBindings);
	stateCreateInfo.pVertexBindingDescriptions = DataPtr(vkBindings);
	stateCreateInfo.vertexAttributeDescriptionCount = CountU32(vkAttributes);
	stateCreateInfo.pVertexAttributeDescriptions = DataPtr(vkAttributes);

	return SUCCESS;
}

Result GraphicsPipeline::initializeInputAssembly(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	VkPipelineInputAssemblyStateCreateInfo& stateCreateInfo)
{
	stateCreateInfo.flags = 0;
	stateCreateInfo.topology = ToVkPrimitiveTopology(pCreateInfo.inputAssemblyState.topology);
	stateCreateInfo.primitiveRestartEnable = pCreateInfo.inputAssemblyState.primitiveRestartEnable ? VK_TRUE : VK_FALSE;

	return SUCCESS;
}

Result GraphicsPipeline::initializeTessellation(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	VkPipelineTessellationDomainOriginStateCreateInfoKHR& domainOriginStateCreateInfo,
	VkPipelineTessellationStateCreateInfo& stateCreateInfo)
{
	domainOriginStateCreateInfo.domainOrigin = ToVkTessellationDomainOrigin(pCreateInfo.tessellationState.domainOrigin);

	stateCreateInfo.flags = 0;
	stateCreateInfo.patchControlPoints = pCreateInfo.tessellationState.patchControlPoints;

	return SUCCESS;
}

Result GraphicsPipeline::initializeViewports(const GraphicsPipelineCreateInfo& pCreateInfo, VkPipelineViewportStateCreateInfo& stateCreateInfo)
{
	stateCreateInfo.flags = 0;
	stateCreateInfo.viewportCount = 1;
	stateCreateInfo.pViewports = nullptr;
	stateCreateInfo.scissorCount = 1;
	stateCreateInfo.pScissors = nullptr;

	return SUCCESS;
}

Result GraphicsPipeline::initializeRasterization(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthClipStateCreateInfo,
	VkPipelineRasterizationStateCreateInfo& stateCreateInfo)
{
	stateCreateInfo.flags = 0;
	stateCreateInfo.depthClampEnable = pCreateInfo.rasterState.depthClampEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.rasterizerDiscardEnable = pCreateInfo.rasterState.rasterizeDiscardEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.polygonMode = ToVkPolygonMode(pCreateInfo.rasterState.polygonMode);
	stateCreateInfo.cullMode = ToVkCullMode(pCreateInfo.rasterState.cullMode);
	stateCreateInfo.frontFace = ToVkFrontFace(pCreateInfo.rasterState.frontFace);
	stateCreateInfo.depthBiasEnable = pCreateInfo.rasterState.depthBiasEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.depthBiasConstantFactor = pCreateInfo.rasterState.depthBiasConstantFactor;
	stateCreateInfo.depthBiasClamp = pCreateInfo.rasterState.depthBiasClamp;
	stateCreateInfo.depthBiasSlopeFactor = pCreateInfo.rasterState.depthBiasSlopeFactor;
	stateCreateInfo.lineWidth = 1.0f;

	// Handle depth clip enable
	if (GetDevice()->HasDepthClipEnabled()) {
		depthClipStateCreateInfo.flags = 0;
		depthClipStateCreateInfo.depthClipEnable = pCreateInfo.rasterState.depthClipEnable;
		// Set pNext
		stateCreateInfo.pNext = &depthClipStateCreateInfo;
	}

	return SUCCESS;
}

Result GraphicsPipeline::initializeMultisample(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	VkPipelineMultisampleStateCreateInfo& stateCreateInfo)
{
	stateCreateInfo.flags = 0;
	stateCreateInfo.rasterizationSamples = ToVkSampleCount(pCreateInfo.rasterState.rasterizationSamples);
	stateCreateInfo.sampleShadingEnable = VK_FALSE;
	stateCreateInfo.minSampleShading = 0.0f;
	stateCreateInfo.pSampleMask = nullptr;
	stateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	stateCreateInfo.alphaToOneEnable = VK_FALSE;

	return SUCCESS;
}

Result GraphicsPipeline::initializeDepthStencil(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	VkPipelineDepthStencilStateCreateInfo& stateCreateInfo)
{
	stateCreateInfo.flags = 0;
	stateCreateInfo.depthTestEnable = pCreateInfo.depthStencilState.depthTestEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.depthWriteEnable = pCreateInfo.depthStencilState.depthWriteEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.depthCompareOp = ToVkEnum(pCreateInfo.depthStencilState.depthCompareOp);
	stateCreateInfo.depthBoundsTestEnable = pCreateInfo.depthStencilState.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.stencilTestEnable = pCreateInfo.depthStencilState.stencilTestEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.front.failOp = ToVkStencilOp(pCreateInfo.depthStencilState.front.failOp);
	stateCreateInfo.front.passOp = ToVkStencilOp(pCreateInfo.depthStencilState.front.passOp);
	stateCreateInfo.front.depthFailOp = ToVkStencilOp(pCreateInfo.depthStencilState.front.depthFailOp);
	stateCreateInfo.front.compareOp = ToVkEnum(pCreateInfo.depthStencilState.front.compareOp);
	stateCreateInfo.front.compareMask = pCreateInfo.depthStencilState.front.compareMask;
	stateCreateInfo.front.writeMask = pCreateInfo.depthStencilState.front.writeMask;
	stateCreateInfo.front.reference = pCreateInfo.depthStencilState.front.reference;
	stateCreateInfo.back.failOp = ToVkStencilOp(pCreateInfo.depthStencilState.back.failOp);
	stateCreateInfo.back.passOp = ToVkStencilOp(pCreateInfo.depthStencilState.back.passOp);
	stateCreateInfo.back.depthFailOp = ToVkStencilOp(pCreateInfo.depthStencilState.back.depthFailOp);
	stateCreateInfo.back.compareOp = ToVkEnum(pCreateInfo.depthStencilState.back.compareOp);
	stateCreateInfo.back.compareMask = pCreateInfo.depthStencilState.back.compareMask;
	stateCreateInfo.back.writeMask = pCreateInfo.depthStencilState.back.writeMask;
	stateCreateInfo.back.reference = pCreateInfo.depthStencilState.back.reference;
	stateCreateInfo.minDepthBounds = pCreateInfo.depthStencilState.minDepthBounds;
	stateCreateInfo.maxDepthBounds = pCreateInfo.depthStencilState.maxDepthBounds;

	return SUCCESS;
}

Result GraphicsPipeline::initializeColorBlend(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	std::vector<VkPipelineColorBlendAttachmentState>& vkAttachments,
	VkPipelineColorBlendStateCreateInfo& stateCreateInfo)
{
	// auto& attachemnts = m_create_info.color_blend_attachment_states.GetStates();
	//// Warn if colorWriteMask is zero
	//{
	//    uint32_t count = CountU32(attachemnts);
	//    for (uint32_t i = 0; i < count; ++i) {
	//        auto& attachment = attachemnts[i];
	//        if (attachment.colorWriteMask == 0) {
	//            std::string name = "<UNAMED>";
	//            VKEX_LOG_RAW("\n*** VKEX WARNING: Graphics Pipeline Warning! ***");
	//            VKEX_LOG_WARN("Function : " << __FUNCTION__);
	//            VKEX_LOG_WARN("Mesage   : Color blend attachment state " << i << " has colorWriteMask=0x0, is this what you want?");
	//            VKEX_LOG_RAW("");
	//        }
	//    }
	//}

	for (uint32_t i = 0; i < pCreateInfo.colorBlendState.blendAttachmentCount; ++i)
	{
		const BlendAttachmentState& attachment = pCreateInfo.colorBlendState.blendAttachments[i];

		VkPipelineColorBlendAttachmentState vkAttachment = {};
		vkAttachment.blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE;
		vkAttachment.srcColorBlendFactor = ToVkBlendFactor(attachment.srcColorBlendFactor);
		vkAttachment.dstColorBlendFactor = ToVkBlendFactor(attachment.dstColorBlendFactor);
		vkAttachment.colorBlendOp = ToVkBlendOp(attachment.colorBlendOp);
		vkAttachment.srcAlphaBlendFactor = ToVkBlendFactor(attachment.srcAlphaBlendFactor);
		vkAttachment.dstAlphaBlendFactor = ToVkBlendFactor(attachment.dstAlphaBlendFactor);
		vkAttachment.alphaBlendOp = ToVkBlendOp(attachment.alphaBlendOp);
		vkAttachment.colorWriteMask = ToVkColorComponentFlags(attachment.colorWriteMask);

		vkAttachments.push_back(vkAttachment);
	}

	stateCreateInfo.flags = 0;
	stateCreateInfo.logicOpEnable = pCreateInfo.colorBlendState.logicOpEnable ? VK_TRUE : VK_FALSE;
	stateCreateInfo.logicOp = ToVkLogicOp(pCreateInfo.colorBlendState.logicOp);
	stateCreateInfo.attachmentCount = CountU32(vkAttachments);
	stateCreateInfo.pAttachments = DataPtr(vkAttachments);
	stateCreateInfo.blendConstants[0] = pCreateInfo.colorBlendState.blendConstants[0];
	stateCreateInfo.blendConstants[1] = pCreateInfo.colorBlendState.blendConstants[1];
	stateCreateInfo.blendConstants[2] = pCreateInfo.colorBlendState.blendConstants[2];
	stateCreateInfo.blendConstants[3] = pCreateInfo.colorBlendState.blendConstants[3];

	return SUCCESS;
}

Result GraphicsPipeline::initializeDynamicState(
	const GraphicsPipelineCreateInfo& pCreateInfo,
	std::vector<VkDynamicState>& dynamicStates,
	VkPipelineDynamicStateCreateInfo& stateCreateInfo)
{
	// NOTE: Since D3D12 doesn't have line width other than 1.0, dynamic
	//       line width is not supported.
	//
	dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR);
	// dynamicStates.push_back(VK_DYNAMIC_STATE_LINE_WIDTH);
	dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BIAS);
	dynamicStates.push_back(VK_DYNAMIC_STATE_BLEND_CONSTANTS);
	dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS);
	dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK);
	dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_WRITE_MASK);
	dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_REFERENCE);

#if defined(VK_EXTENDED_DYNAMIC_STATE)
	if (GetDevice()->IsExtendedDynamicStateAvailable()) {
		// Provided by VK_EXT_extended_dynamic_state
		dynamicStates.push_back(VK_DYNAMIC_STATE_CULL_MODE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_FRONT_FACE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_PRIMITIVE_TOPOLOGY_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_VERTEX_INPUT_BINDING_STRIDE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE_EXT);
		dynamicStates.push_back(VK_DYNAMIC_STATE_STENCIL_OP_EXT);
	}
#endif // defined(PPX_VK_EXTENDED_DYNAMIC_STATE)

	stateCreateInfo.flags = 0;
	stateCreateInfo.dynamicStateCount = CountU32(dynamicStates);
	stateCreateInfo.pDynamicStates = DataPtr(dynamicStates);

	return SUCCESS;
}

Result GraphicsPipeline::createApiObjects(const GraphicsPipelineCreateInfo& pCreateInfo)
{
	VkGraphicsPipelineCreateInfo vkci = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	Result                                       ppxres = initializeShaderStages(pCreateInfo, shaderStages, vkci);

	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription>   vertexBindings;
	VkPipelineVertexInputStateCreateInfo           vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	//
	ppxres = initializeVertexInput(pCreateInfo, vertexAttributes, vertexBindings, vertexInputState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	//
	ppxres = initializeInputAssembly(pCreateInfo, inputAssemblyState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkPipelineTessellationDomainOriginStateCreateInfoKHR domainOriginStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO_KHR };
	VkPipelineTessellationStateCreateInfo                tessellationState = { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	//
	ppxres = initializeTessellation(pCreateInfo, domainOriginStateCreateInfo, tessellationState);
	if (Failed(ppxres)) {
		return ppxres;
	}
	tessellationState.pNext = (pCreateInfo.tessellationState.patchControlPoints > 0) ? &domainOriginStateCreateInfo : nullptr;

	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	//
	ppxres = initializeViewports(pCreateInfo, viewportState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT };
	VkPipelineRasterizationStateCreateInfo             rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	//
	ppxres = initializeRasterization(pCreateInfo, depthClipStateCreateInfo, rasterizationState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	//
	ppxres = initializeMultisample(pCreateInfo, multisampleState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	//
	ppxres = initializeDepthStencil(pCreateInfo, depthStencilState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	std::vector<VkPipelineColorBlendAttachmentState> attachments;
	VkPipelineColorBlendStateCreateInfo              colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	//
	ppxres = initializeColorBlend(pCreateInfo, attachments, colorBlendState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	std::vector<VkDynamicState>      dynamicStatesArray;
	VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	//
	ppxres = initializeDynamicState(pCreateInfo, dynamicStatesArray, dynamicState);
	if (Failed(ppxres)) {
		return ppxres;
	}

	VkRenderPassPtr       renderPass = VK_NULL_HANDLE;
	std::vector<VkFormat> renderTargetFormats;
	for (uint32_t i = 0; i < pCreateInfo.outputState.renderTargetCount; ++i) {
		renderTargetFormats.push_back(ToVkEnum(pCreateInfo.outputState.renderTargetFormats[i]));
	}
	VkFormat depthStencilFormat = ToVkEnum(pCreateInfo.outputState.depthStencilFormat);

#if defined(VK_KHR_dynamic_rendering)
	VkPipelineRenderingCreateInfo renderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };

	if (pCreateInfo.dynamicRenderPass) {
		renderingCreateInfo.viewMask = 0;
		renderingCreateInfo.colorAttachmentCount = CountU32(renderTargetFormats);
		renderingCreateInfo.pColorAttachmentFormats = DataPtr(renderTargetFormats);
		renderingCreateInfo.depthAttachmentFormat = depthStencilFormat;
		if (GetFormatDescription(pCreateInfo.outputState.depthStencilFormat)->aspect & FORMAT_ASPECT_STENCIL) {
			renderingCreateInfo.stencilAttachmentFormat = depthStencilFormat;
		}

		vkci.pNext = &renderingCreateInfo;
	}
	else
#endif
	{
		// Create temporary render pass
		//

		VkResult vkres = CreateTransientRenderPass(
			GetDevice(),
			CountU32(renderTargetFormats),
			DataPtr(renderTargetFormats),
			depthStencilFormat,
			ToVkSampleCount(pCreateInfo.rasterState.rasterizationSamples),
			pCreateInfo.multiViewState.viewMask,
			pCreateInfo.multiViewState.correlationMask,
			&renderPass,
			pCreateInfo.shadingRateMode);
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "CreateTransientRenderPass failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	// Fill in pointers nad remaining values
	//
	vkci.flags = 0;
	vkci.stageCount = CountU32(shaderStages);
	vkci.pStages = DataPtr(shaderStages);
	vkci.pVertexInputState = &vertexInputState;
	vkci.pInputAssemblyState = &inputAssemblyState;
	vkci.pTessellationState = &tessellationState;
	vkci.pViewportState = &viewportState;
	vkci.pRasterizationState = &rasterizationState;
	vkci.pMultisampleState = &multisampleState;
	vkci.pDepthStencilState = &depthStencilState;
	vkci.pColorBlendState = &colorBlendState;
	vkci.pDynamicState = &dynamicState;
	vkci.layout = pCreateInfo.pPipelineInterface->GetVkPipelineLayout();
	vkci.renderPass = renderPass;
	vkci.subpass = 0; // One subpass to rule them all
	vkci.basePipelineHandle = VK_NULL_HANDLE;
	vkci.basePipelineIndex = -1;

	// [VRS] set pipeline shading rate
	VkPipelineFragmentShadingRateStateCreateInfoKHR shadingRate = { VK_STRUCTURE_TYPE_PIPELINE_FRAGMENT_SHADING_RATE_STATE_CREATE_INFO_KHR };
	if (pCreateInfo.shadingRateMode == SHADING_RATE_VRS)
	{
		shadingRate.fragmentSize = VkExtent2D{ 1, 1 };
		shadingRate.combinerOps[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
		shadingRate.combinerOps[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
		InsertPNext(vkci, shadingRate);
	}

	VkResult vkres = vkCreateGraphicsPipelines(
		GetDevice()->GetVkDevice(),
		VK_NULL_HANDLE,
		1,
		&vkci,
		nullptr,
		&mPipeline);
	// Destroy transient render pass
	if (renderPass) {
		vkDestroyRenderPass(
			GetDevice()->GetVkDevice(),
			renderPass,
			nullptr);
	}
	// Process result
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreateGraphicsPipelines failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void GraphicsPipeline::destroyApiObjects()
{
	if (mPipeline) {
		vkDestroyPipeline(GetDevice()->GetVkDevice(), mPipeline, nullptr);
		mPipeline.Reset();
	}
}

Result PipelineInterface::create(const PipelineInterfaceCreateInfo& pCreateInfo)
{
	if (pCreateInfo.setCount > MaxBoundDescriptorSets) {
		ASSERT_MSG(false, "set count exceeds MAX_BOUND_DESCRIPTOR_SETS");
		return ERROR_LIMIT_EXCEEDED;
	}

	// If we have more than one set...we need to do some checks
	if (pCreateInfo.setCount > 0) {
		// Paranoid clear
		mSetNumbers.clear();
		// Copy set numbers
		std::vector<uint32_t> sortedSetNumbers;
		for (uint32_t i = 0; i < pCreateInfo.setCount; ++i) {
			uint32_t set = pCreateInfo.sets[i].set;
			sortedSetNumbers.push_back(set); // Sortable array
			mSetNumbers.push_back(set);      // Preserves declared ordering
		}
		// Sort set numbers
		std::sort(std::begin(sortedSetNumbers), std::end(sortedSetNumbers));
		// Check for uniqueness
		for (size_t i = 1; i < sortedSetNumbers.size(); ++i) {
			uint32_t setB = sortedSetNumbers[i];
			uint32_t setA = sortedSetNumbers[i - 1];
			uint32_t diff = setB - setA;
			if (diff == 0) {
				ASSERT_MSG(false, "set numbers are not unique");
				return ERROR_GRFX_NON_UNIQUE_SET;
			}
		}
		// Check for consecutive ness
		//
		// Assume consecutive
		mHasConsecutiveSetNumbers = true;
		for (size_t i = 1; i < mSetNumbers.size(); ++i) {
			int32_t setB = static_cast<int32_t>(sortedSetNumbers[i]);
			int32_t setA = static_cast<int32_t>(sortedSetNumbers[i - 1]);
			int32_t diff = setB - setA;
			if (diff != 1) {
				mHasConsecutiveSetNumbers = false;
				break;
			}
		}
	}

	// Check limit and make sure the push constants binding/set doesn't collide
	// with an existing binding in the set layouts.
	//
	if (pCreateInfo.pushConstants.count > 0) {
		if (pCreateInfo.pushConstants.count > MaxPushConstants) {
			ASSERT_MSG(false, "push constants count (" + std::to_string(pCreateInfo.pushConstants.count) + ") exceeds MAX_PUSH_CONSTANTS (" + std::to_string(MaxPushConstants) + ")");
			return ERROR_LIMIT_EXCEEDED;
		}

		if (pCreateInfo.pushConstants.binding == VALUE_IGNORED) {
			ASSERT_MSG(false, "push constants binding number is invalid");
			return ERROR_GRFX_INVALID_BINDING_NUMBER;
		}
		if (pCreateInfo.pushConstants.set == VALUE_IGNORED) {
			ASSERT_MSG(false, "push constants set number is invalid");
			return ERROR_GRFX_INVALID_SET_NUMBER;
		}

		for (uint32_t i = 0; i < pCreateInfo.setCount; ++i) {
			auto& set = pCreateInfo.sets[i];
			// Skip if set number doesn't match
			if (set.set != pCreateInfo.pushConstants.set) {
				continue;
			}
			// See if the layout has a binding that's the same as the push constants binding
			const uint32_t pushConstantsBinding = pCreateInfo.pushConstants.binding;
			auto& bindings = set.pLayout->GetBindings();
			auto           it = std::find_if(
				bindings.begin(),
				bindings.end(),
				[pushConstantsBinding](const DescriptorBinding& elem) -> bool {
					bool match = (elem.binding == pushConstantsBinding);
					return match; });
			// Error out if a match is found
			if (it != bindings.end()) {
				ASSERT_MSG(false, "push constants binding and set overlaps with a binding in set " + std::to_string(set.set));
				return ERROR_GRFX_NON_UNIQUE_BINDING;
			}
		}
	}

	Result ppxres = DeviceObject<PipelineInterfaceCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result PipelineInterface::createApiObjects(const PipelineInterfaceCreateInfo& pCreateInfo)
{
	VkDescriptorSetLayout setLayouts[MaxBoundDescriptorSets] = { VK_NULL_HANDLE };
	for (uint32_t i = 0; i < pCreateInfo.setCount; ++i) {
		setLayouts[i] = pCreateInfo.sets[i].pLayout->GetVkDescriptorSetLayout();
	}

	bool                hasPushConstants = (pCreateInfo.pushConstants.count > 0);
	VkPushConstantRange pushConstantsRange = {};
	if (hasPushConstants) {
		const uint32_t sizeInBytes = pCreateInfo.pushConstants.count * sizeof(uint32_t);

		// Double check device limits
		auto& limits = GetDevice()->GetDeviceLimits();
		if (sizeInBytes > limits.maxPushConstantsSize)
		{
			ASSERT_MSG(false, "push constants size in bytes (" + std::to_string(sizeInBytes) + ") exceeds VkPhysicalDeviceLimits::maxPushConstantsSize (" + std::to_string(limits.maxPushConstantsSize) + ")");
			return ERROR_LIMIT_EXCEEDED;
		}

		// Save stage flags for use in command buffer
		mPushConstantShaderStageFlags = ToVkShaderStageFlags(pCreateInfo.pushConstants.shaderVisiblity);

		// Fill out range
		pushConstantsRange.stageFlags = mPushConstantShaderStageFlags;
		pushConstantsRange.offset = 0;
		pushConstantsRange.size = sizeInBytes;
	}

	VkPipelineLayoutCreateInfo vkci = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	vkci.flags = 0;
	vkci.setLayoutCount = pCreateInfo.setCount;
	vkci.pSetLayouts = setLayouts;
	vkci.pushConstantRangeCount = hasPushConstants ? 1 : 0;
	vkci.pPushConstantRanges = hasPushConstants ? &pushConstantsRange : nullptr;

	VkResult vkres = vkCreatePipelineLayout(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mPipelineLayout);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreatePipelineLayout failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void PipelineInterface::destroyApiObjects()
{
	if (mPipelineLayout) {
		vkDestroyPipelineLayout(GetDevice()->GetVkDevice(), mPipelineLayout, nullptr);
		mPipelineLayout.Reset();
	}
}

const DescriptorSetLayout* PipelineInterface::GetSetLayout(uint32_t setNumber) const
{
	const DescriptorSetLayout* pLayout = nullptr;
	for (uint32_t i = 0; i < m_createInfo.setCount; ++i) {
		if (m_createInfo.sets[i].set == setNumber) {
			pLayout = m_createInfo.sets[i].pLayout;
			break;
		}
	}
	return pLayout;
}

#pragma endregion

#pragma region Command

CommandType CommandPool::GetCommandType() const
{
	return m_createInfo.pQueue->GetCommandType();
}

Result CommandPool::createApiObjects(const CommandPoolCreateInfo& pCreateInfo)
{
	VkCommandPoolCreateInfo vkci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	vkci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkci.queueFamilyIndex = pCreateInfo.pQueue->GetQueueFamilyIndex();

	VkResult vkres = vkCreateCommandPool(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mCommandPool);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateCommandPool failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void CommandPool::destroyApiObjects()
{
	if (mCommandPool)
	{
		vkDestroyCommandPool(GetDevice()->GetVkDevice(), mCommandPool, nullptr);
		mCommandPool.Reset();
	}
}

bool CommandBuffer::HasActiveRenderPass() const
{
	return !IsNull(mCurrentRenderPass) || mDynamicRenderPassActive;
}

void CommandBuffer::BeginRenderPass(const RenderPassBeginInfo* pBeginInfo)
{
	if (HasActiveRenderPass()) {
		ASSERT_MSG(false, "cannot nest render passes");
	}

	if (pBeginInfo->pRenderPass->HasLoadOpClear()) {
		uint32_t rtvCount = pBeginInfo->pRenderPass->GetRenderTargetCount();
		uint32_t clearCount = pBeginInfo->RTVClearCount;
		if (clearCount < rtvCount) {
			ASSERT_MSG(false, "clear count cannot less than RTV count");
		}
	}

	BeginRenderPassImpl(pBeginInfo);
	mCurrentRenderPass = pBeginInfo->pRenderPass;
}

void CommandBuffer::EndRenderPass()
{
	if (IsNull(mCurrentRenderPass)) {
		ASSERT_MSG(false, "no render pass to end");
	}
	ASSERT_MSG(!mDynamicRenderPassActive, "Dynamic render pass active, use EndRendering instead");

	EndRenderPassImpl();
	mCurrentRenderPass = nullptr;
}

void CommandBuffer::BeginRendering(const RenderingInfo* pRenderingInfo)
{
	ASSERT_NULL_ARG(pRenderingInfo);
	ASSERT_MSG(!HasActiveRenderPass(), "cannot nest render passes");

	BeginRenderingImpl(pRenderingInfo);
	mDynamicRenderPassActive = true;
	mDynamicRenderPassInfo.mRenderArea = pRenderingInfo->renderArea;
	auto views = pRenderingInfo->pRenderTargetViews;
	for (uint32_t i = 0; i < pRenderingInfo->renderTargetCount; ++i) {
		const RenderTargetViewPtr& rtv = views[i];
		mDynamicRenderPassInfo.mRenderTargetViews.push_back(rtv);
	}
	mDynamicRenderPassInfo.mDepthStencilView = pRenderingInfo->pDepthStencilView;
}

void CommandBuffer::EndRendering()
{
	ASSERT_MSG(mDynamicRenderPassActive, "no render pass to end")
		ASSERT_MSG(IsNull(mCurrentRenderPass), "Non-dynamic render pass active, use EndRendering instead");

	EndRenderingImpl();
	mDynamicRenderPassActive = false;
	mDynamicRenderPassInfo = {};
}

void CommandBuffer::BeginRenderPass(const RenderPass* pRenderPass)
{
	ASSERT_NULL_ARG(pRenderPass);

	RenderPassBeginInfo beginInfo = {};
	beginInfo.pRenderPass = pRenderPass;
	beginInfo.renderArea = pRenderPass->GetRenderArea();

	beginInfo.RTVClearCount = pRenderPass->GetRenderTargetCount();
	for (uint32_t i = 0; i < beginInfo.RTVClearCount; ++i) {
		ImagePtr rtvImage = pRenderPass->GetRenderTargetImage(i);
		beginInfo.RTVClearValues[i] = rtvImage->GetRTVClearValue();
	}

	ImagePtr dsvImage = pRenderPass->GetDepthStencilImage();
	if (dsvImage) {
		beginInfo.DSVClearValue = dsvImage->GetDSVClearValue();
	}

	BeginRenderPass(&beginInfo);
}

void CommandBuffer::BeginRenderPass(
	const DrawPass* pDrawPass,
	const DrawPassClearFlags& clearFlags)
{
	ASSERT_NULL_ARG(pDrawPass);

	RenderPassBeginInfo beginInfo = {};
	pDrawPass->PrepareRenderPassBeginInfo(clearFlags, &beginInfo);

	BeginRenderPass(&beginInfo);
}

void CommandBuffer::TransitionImageLayout(
	RenderPass* pRenderPass,
	ResourceState renderTargetBeforeState,
	ResourceState renderTargetAfterState,
	ResourceState depthStencilTargetBeforeState,
	ResourceState depthStencilTargetAfterState)
{
	ASSERT_NULL_ARG(pRenderPass);

	const uint32_t n = pRenderPass->GetRenderTargetCount();
	for (uint32_t i = 0; i < n; ++i) {
		ImagePtr renderTarget;
		Result         ppxres = pRenderPass->GetRenderTargetImage(i, &renderTarget);
		ASSERT_MSG(ppxres == SUCCESS, "failed getting render pass render target");

		TransitionImageLayout(
			renderTarget,
			ALL_SUBRESOURCES,
			renderTargetBeforeState,
			renderTargetAfterState);
	}

	if (pRenderPass->HasDepthStencil()) {
		ImagePtr depthStencil;
		Result         ppxres = pRenderPass->GetDepthStencilImage(&depthStencil);
		ASSERT_MSG(ppxres == SUCCESS, "failed getting render pass depth/stencil");

		TransitionImageLayout(
			depthStencil,
			ALL_SUBRESOURCES,
			depthStencilTargetBeforeState,
			depthStencilTargetAfterState);
	}
}

void CommandBuffer::TransitionImageLayout(
	const Texture* pTexture,
	uint32_t             mipLevel,
	uint32_t             mipLevelCount,
	uint32_t             arrayLayer,
	uint32_t             arrayLayerCount,
	ResourceState  beforeState,
	ResourceState  afterState,
	const Queue* pSrcQueue,
	const Queue* pDstQueue)
{
	TransitionImageLayout(
		pTexture->GetImage(),
		mipLevel,
		mipLevelCount,
		arrayLayer,
		arrayLayerCount,
		beforeState,
		afterState,
		pSrcQueue,
		pDstQueue);
}

void CommandBuffer::TransitionImageLayout(
	DrawPass* pDrawPass,
	ResourceState renderTargetBeforeState,
	ResourceState renderTargetAfterState,
	ResourceState depthStencilTargetBeforeState,
	ResourceState depthStencilTargetAfterState)
{
	ASSERT_NULL_ARG(pDrawPass);

	const uint32_t n = pDrawPass->GetRenderTargetCount();
	for (uint32_t i = 0; i < n; ++i) {
		TexturePtr renderTarget;
		Result           ppxres = pDrawPass->GetRenderTargetTexture(i, &renderTarget);
		ASSERT_MSG(ppxres == SUCCESS, "failed getting draw pass render target");

		TransitionImageLayout(
			renderTarget->GetImage(),
			ALL_SUBRESOURCES,
			renderTargetBeforeState,
			renderTargetAfterState);
	}

	if (pDrawPass->HasDepthStencil()) {
		TexturePtr depthSencil;
		Result           ppxres = pDrawPass->GetDepthStencilTexture(&depthSencil);
		ASSERT_MSG(ppxres == SUCCESS, "failed getting draw pass depth/stencil");

		TransitionImageLayout(
			depthSencil->GetImage(),
			ALL_SUBRESOURCES,
			depthStencilTargetBeforeState,
			depthStencilTargetAfterState);
	}
}

void CommandBuffer::SetViewports(const Viewport& viewport)
{
	SetViewports(1, &viewport);
}

void CommandBuffer::SetScissors(const Rect& scissor)
{
	SetScissors(1, &scissor);
}

void CommandBuffer::BindIndexBuffer(const Buffer* pBuffer, IndexType indexType, uint64_t offset)
{
	ASSERT_NULL_ARG(pBuffer);

	IndexBufferView view = {};
	view.pBuffer = pBuffer;
	view.indexType = indexType;
	view.offset = offset;

	BindIndexBuffer(&view);
}

void CommandBuffer::BindIndexBuffer(const Mesh* pMesh, uint64_t offset)
{
	ASSERT_NULL_ARG(pMesh);

	BindIndexBuffer(pMesh->GetIndexBuffer(), pMesh->GetIndexType(), offset);
}

void CommandBuffer::BindVertexBuffers(
	uint32_t                   bufferCount,
	const Buffer* const* buffers,
	const uint32_t* pStrides,
	const uint64_t* pOffsets)
{
	ASSERT_NULL_ARG(buffers);
	ASSERT_NULL_ARG(pStrides);
	ASSERT_MSG(bufferCount < MaxVertexBindings, "bufferCount exceeds PPX_MAX_VERTEX_ATTRIBUTES");

	VertexBufferView views[MaxVertexBindings] = {};
	for (uint32_t i = 0; i < bufferCount; ++i) {
		views[i].pBuffer = buffers[i];
		views[i].stride = pStrides[i];
		if (!IsNull(pOffsets)) {
			views[i].offset = pOffsets[i];
		}
	}

	BindVertexBuffers(bufferCount, views);
}

void CommandBuffer::BindVertexBuffers(const Mesh* pMesh, const uint64_t* pOffsets)
{
	ASSERT_NULL_ARG(pMesh);

	const Buffer* buffers[MaxVertexBindings] = { nullptr };
	uint32_t            strides[MaxVertexBindings] = { 0 };

	uint32_t bufferCount = pMesh->GetVertexBufferCount();
	for (uint32_t i = 0; i < bufferCount; ++i) {
		buffers[i] = pMesh->GetVertexBuffer(i);
		strides[i] = pMesh->GetVertexBufferDescription(i)->stride;
	}

	BindVertexBuffers(bufferCount, buffers, strides, pOffsets);
}

void CommandBuffer::Draw(const FullscreenQuad* pQuad, uint32_t setCount, const DescriptorSet* const* ppSets)
{
	BindGraphicsDescriptorSets(pQuad->GetPipelineInterface(), setCount, ppSets);
	BindGraphicsPipeline(pQuad->GetPipeline());
	Draw(3, 1);
}

void CommandBuffer::PushGraphicsUniformBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,          // pipelineBindPoint
		pInterface,                           // pInterface
		DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
		binding,                              // binding
		set,                                  // set
		bufferOffset,                         // bufferOffset
		pBuffer,                              // pBuffer
		nullptr,                              // pSampledImageView
		nullptr,                              // pStorageImageView
		nullptr);                             // pSampler
}

void CommandBuffer::PushGraphicsStructuredBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,                // pipelineBindPoint
		pInterface,                                 // pInterface
		DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, // descriptorType
		binding,                                    // binding
		set,                                        // set
		bufferOffset,                               // bufferOffset
		pBuffer,                                    // pBuffer
		nullptr,                                    // pSampledImageView
		nullptr,                                    // pStorageImageView
		nullptr);                                   // pSampler
}

void CommandBuffer::PushGraphicsStorageBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,                // pipelineBindPoint
		pInterface,                                 // pInterface
		DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, // descriptorType
		binding,                                    // binding
		set,                                        // set
		bufferOffset,                               // bufferOffset
		pBuffer,                                    // pBuffer
		nullptr,                                    // pSampledImageView
		nullptr,                                    // pStorageImageView
		nullptr);                                   // pSampler
}

void CommandBuffer::PushGraphicsSampledImage(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const SampledImageView* pView)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,         // pipelineBindPoint
		pInterface,                          // pInterface
		DESCRIPTOR_TYPE_SAMPLED_IMAGE, // descriptorType
		binding,                             // binding
		set,                                 // set
		0,                                   // bufferOffset
		nullptr,                             // pBuffer
		pView,                               // pSampledImageView
		nullptr,                             // pStorageImageView
		nullptr);                            // pSampler
}

void CommandBuffer::PushGraphicsStorageImage(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const StorageImageView* pView)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,         // pipelineBindPoint
		pInterface,                          // pInterface
		DESCRIPTOR_TYPE_STORAGE_IMAGE, // descriptorType
		binding,                             // binding
		set,                                 // set
		0,                                   // bufferOffset
		nullptr,                             // pBuffer
		nullptr,                             // pSampledImageView
		pView,                               // pStorageImageView
		nullptr);                            // pSampler
}

void CommandBuffer::PushGraphicsSampler(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const Sampler* pSampler)
{
	PushDescriptorImpl(
		COMMAND_TYPE_GRAPHICS,   // pipelineBindPoint
		pInterface,                    // pInterface
		DESCRIPTOR_TYPE_SAMPLER, // descriptorType
		binding,                       // binding
		set,                           // set
		0,                             // bufferOffset
		nullptr,                       // pBuffer
		nullptr,                       // pSampledImageView
		nullptr,                       // pStorageImageView
		pSampler);                     // pSampler
}

void CommandBuffer::PushComputeUniformBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,           // pipelineBindPoint
		pInterface,                           // pInterface
		DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
		binding,                              // binding
		set,                                  // set
		bufferOffset,                         // bufferOffset
		pBuffer,                              // pBuffer
		nullptr,                              // pSampledImageView
		nullptr,                              // pStorageImageView
		nullptr);                             // pSampler
}

void CommandBuffer::PushComputeStructuredBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,                 // pipelineBindPoint
		pInterface,                                 // pInterface
		DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER, // descriptorType
		binding,                                    // binding
		set,                                        // set
		bufferOffset,                               // bufferOffset
		pBuffer,                                    // pBuffer
		nullptr,                                    // pSampledImageView
		nullptr,                                    // pStorageImageView
		nullptr);                                   // pSampler
}

void CommandBuffer::PushComputeStorageBuffer(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,                 // pipelineBindPoint
		pInterface,                                 // pInterface
		DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER, // descriptorType
		binding,                                    // binding
		set,                                        // set
		bufferOffset,                               // bufferOffset
		pBuffer,                                    // pBuffer
		nullptr,                                    // pSampledImageView
		nullptr,                                    // pStorageImageView
		nullptr);                                   // pSampler
}

void CommandBuffer::PushComputeSampledImage(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const SampledImageView* pView)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,          // pipelineBindPoint
		pInterface,                          // pInterface
		DESCRIPTOR_TYPE_SAMPLED_IMAGE, // descriptorType
		binding,                             // binding
		set,                                 // set
		0,                                   // bufferOffset
		nullptr,                             // pBuffer
		pView,                               // pSampledImageView
		nullptr,                             // pStorageImageView
		nullptr);                            // pSampler
}

void CommandBuffer::PushComputeStorageImage(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const StorageImageView* pView)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,          // pipelineBindPoint
		pInterface,                          // pInterface
		DESCRIPTOR_TYPE_STORAGE_IMAGE, // descriptorType
		binding,                             // binding
		set,                                 // set
		0,                                   // bufferOffset
		nullptr,                             // pBuffer
		nullptr,                             // pSampledImageView
		pView,                               // pStorageImageView
		nullptr);                            // pSampler
}

void CommandBuffer::PushComputeSampler(
	const PipelineInterface* pInterface,
	uint32_t                       binding,
	uint32_t                       set,
	const Sampler* pSampler)
{
	PushDescriptorImpl(
		COMMAND_TYPE_COMPUTE,    // pipelineBindPoint
		pInterface,                    // pInterface
		DESCRIPTOR_TYPE_SAMPLER, // descriptorType
		binding,                       // binding
		set,                           // set
		0,                             // bufferOffset
		nullptr,                       // pBuffer
		nullptr,                       // pSampledImageView
		nullptr,                       // pStorageImageView
		pSampler);                     // pSampler
}

Result CommandBuffer::createApiObjects(const internal::CommandBufferCreateInfo& pCreateInfo)
{
	VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	vkai.commandPool = pCreateInfo.pPool->GetVkCommandPool();
	vkai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkai.commandBufferCount = 1;

	VkResult vkres = vkAllocateCommandBuffers(
		GetDevice()->GetVkDevice(),
		&vkai,
		&mCommandBuffer);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkAllocateCommandBuffers failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void CommandBuffer::destroyApiObjects()
{
	if (mCommandBuffer) {
		vkFreeCommandBuffers(
			GetDevice()->GetVkDevice(),
			m_createInfo.pPool->GetVkCommandPool(),
			1,
			mCommandBuffer);

		mCommandBuffer.Reset();
	}
}

Result CommandBuffer::Begin()
{
	VkCommandBufferBeginInfo vkbi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

	VkResult vkres = vkBeginCommandBuffer(mCommandBuffer, &vkbi);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkBeginCommandBuffer failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

Result CommandBuffer::End()
{
	VkResult vkres = vkEndCommandBuffer(mCommandBuffer);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkEndCommandBuffer failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void CommandBuffer::BeginRenderPassImpl(const RenderPassBeginInfo* pBeginInfo)
{
	VkRect2D rect = {};
	rect.offset = { pBeginInfo->renderArea.x, pBeginInfo->renderArea.x };
	rect.extent = { pBeginInfo->renderArea.width, pBeginInfo->renderArea.height };

	uint32_t     clearValueCount = 0;
	VkClearValue clearValues[MaxRenderTargets + 1] = {};

	for (uint32_t i = 0; i < pBeginInfo->RTVClearCount; ++i) {
		clearValues[i].color = ToVkClearColorValue(pBeginInfo->RTVClearValues[i]);
		++clearValueCount;
	}

	if (pBeginInfo->pRenderPass->GetDepthStencilView()) {
		uint32_t i = clearValueCount;
		clearValues[i].depthStencil = ToVkClearDepthStencilValue(pBeginInfo->DSVClearValue);
		++clearValueCount;
	}

	VkRenderPassBeginInfo vkbi = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	vkbi.renderPass = pBeginInfo->pRenderPass->GetVkRenderPass();
	vkbi.framebuffer = pBeginInfo->pRenderPass->GetVkFramebuffer();
	vkbi.renderArea = rect;
	vkbi.clearValueCount = clearValueCount;
	vkbi.pClearValues = clearValues;

	vkCmdBeginRenderPass(mCommandBuffer, &vkbi, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPassImpl()
{
	vkCmdEndRenderPass(mCommandBuffer);
}

void CommandBuffer::BeginRenderingImpl(const RenderingInfo* pRenderingInfo)
{
#if defined(VK_KHR_dynamic_rendering)
	VkRect2D rect = {};
	rect.offset = { pRenderingInfo->renderArea.x, pRenderingInfo->renderArea.x };
	rect.extent = { pRenderingInfo->renderArea.width, pRenderingInfo->renderArea.height };

	std::vector<VkRenderingAttachmentInfo> colorAttachmentDescs;
	for (uint32_t i = 0; i < pRenderingInfo->renderTargetCount; ++i)
	{
		const RenderTargetViewPtr& rtv = pRenderingInfo->pRenderTargetViews[i];
		VkRenderingAttachmentInfo        colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		colorAttachment.imageView = rtv.Get()->GetVkImageView();
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachment.loadOp = ToVkAttachmentLoadOp(rtv->GetLoadOp());
		colorAttachment.storeOp = ToVkAttachmentStoreOp(rtv->GetStoreOp());
		colorAttachment.clearValue.color = ToVkClearColorValue(pRenderingInfo->RTVClearValues[i]);
		colorAttachmentDescs.push_back(colorAttachment);
	}

	VkRenderingInfo vkri = { VK_STRUCTURE_TYPE_RENDERING_INFO };
	vkri.renderArea = rect;
	vkri.layerCount = 1;
	vkri.viewMask = 0;
	vkri.colorAttachmentCount = CountU32(colorAttachmentDescs);
	vkri.pColorAttachments = DataPtr(colorAttachmentDescs);

	if (pRenderingInfo->pDepthStencilView) {
		DepthStencilViewPtr dsv = pRenderingInfo->pDepthStencilView;
		VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
		depthAttachment.imageView = dsv.Get()->GetVkImageView();
		depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		depthAttachment.loadOp = ToVkAttachmentLoadOp(dsv->GetDepthLoadOp());
		depthAttachment.storeOp = ToVkAttachmentStoreOp(dsv->GetDepthStoreOp());
		depthAttachment.clearValue.depthStencil = ToVkClearDepthStencilValue(pRenderingInfo->DSVClearValue);
		vkri.pDepthAttachment = &depthAttachment;
	}

	if (pRenderingInfo->flags.bits.resuming) {
		vkri.flags |= VK_RENDERING_RESUMING_BIT;
	}
	if (pRenderingInfo->flags.bits.suspending) {
		vkri.flags |= VK_RENDERING_SUSPENDING_BIT;
	}

	vkCmdBeginRenderingKHR(mCommandBuffer, &vkri);
#endif
}

void CommandBuffer::EndRenderingImpl()
{
#if defined(VK_KHR_dynamic_rendering)
	vkCmdEndRenderingKHR(mCommandBuffer);
#endif
}

void CommandBuffer::PushDescriptorImpl(
	CommandType              pipelineBindPoint,
	const PipelineInterface* pInterface,
	DescriptorType           descriptorType,
	uint32_t                       binding,
	uint32_t                       set,
	uint32_t                       bufferOffset,
	const Buffer* pBuffer,
	const SampledImageView* pSampledImageView,
	const StorageImageView* pStorageImageView,
	const Sampler* pSampler)
{
	auto pLayout = pInterface->GetSetLayout(set);
	ASSERT_MSG((pLayout != nullptr), "set=" + std::to_string(set) + " does not match a set layout in the pipeline interface");
	ASSERT_MSG(pLayout->IsPushable(), "set=" + std::to_string(set) + " refers to a set layout that is not pushable");

	// Determine pipeline bind point
	VkPipelineBindPoint vulkanPipelineBindPoint = InvalidValue<VkPipelineBindPoint>();
	switch (pipelineBindPoint) {
	default: ASSERT_MSG(false, "invalid pipeline bindpoint"); break;
	case COMMAND_TYPE_GRAPHICS: vulkanPipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; break;
	case COMMAND_TYPE_COMPUTE: vulkanPipelineBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE; break;
	}

	// Determine descriptor type and fill out buffer/image info
	VkDescriptorImageInfo         imageInfo = {};
	VkDescriptorBufferInfo        bufferInfo = {};
	const VkDescriptorImageInfo* pImageInfo = nullptr;
	const VkDescriptorBufferInfo* pBufferInfo = nullptr;
	VkDescriptorType              vulkanDescriptorType = InvalidValue<VkDescriptorType>();

	switch (descriptorType) {
	default: ASSERT_MSG(false, "descriptor is not of pushable type binding=" + std::to_string(binding) + ", set=" + std::to_string(set)); break;

	case DESCRIPTOR_TYPE_SAMPLER: {
		vulkanDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		imageInfo.sampler = pSampler->GetVkSampler();
		pImageInfo = &imageInfo;
	} break;

	case DESCRIPTOR_TYPE_SAMPLED_IMAGE: {
		vulkanDescriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = pSampledImageView->GetVkImageView();
		pImageInfo = &imageInfo;
	} break;

	case DESCRIPTOR_TYPE_STORAGE_IMAGE: {
		vulkanDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo.imageView = pStorageImageView->GetVkImageView();
		pImageInfo = &imageInfo;
	} break;

	case DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
		vulkanDescriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferInfo.buffer = pBuffer->GetVkBuffer();
		bufferInfo.offset = bufferOffset;
		bufferInfo.range = VK_WHOLE_SIZE;
		pBufferInfo = &bufferInfo;
	} break;

	case DESCRIPTOR_TYPE_RAW_STORAGE_BUFFER:
	case DESCRIPTOR_TYPE_RO_STRUCTURED_BUFFER:
	case DESCRIPTOR_TYPE_RW_STRUCTURED_BUFFER: {
		vulkanDescriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bufferInfo.buffer = pBuffer->GetVkBuffer();
		bufferInfo.offset = bufferOffset;
		bufferInfo.range = VK_WHOLE_SIZE;
		pBufferInfo = &bufferInfo;
	} break;
	}

	// Fill out most of descriptor write
	VkWriteDescriptorSet write = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	write.pNext = nullptr;
	write.dstSet = VK_NULL_HANDLE;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorCount = 1;
	write.descriptorType = vulkanDescriptorType;
	write.pImageInfo = pImageInfo;
	write.pBufferInfo = pBufferInfo;
	write.pTexelBufferView = nullptr;

	vkCmdPushDescriptorSetKHR(
		mCommandBuffer,                           // commandBuffer
		vulkanPipelineBindPoint,                  // pipelineBindPoint
		pInterface->GetVkPipelineLayout(), // layout
		set,                                      // set
		1,                                        // descriptorWriteCount
		&write);                                  // pDescriptorWrites;
}

void CommandBuffer::ClearRenderTarget(
	Image* pImage,
	const RenderTargetClearValue& clearValue)
{
	if (!HasActiveRenderPass()) {
		Warning("No active render pass.");
		return;
	}

	Rect renderArea;
	uint32_t   colorAttachment = UINT32_MAX;
	uint32_t   baseArrayLayer;

	// Dynamic render pass
	if (mDynamicRenderPassActive) {
		renderArea = mDynamicRenderPassInfo.mRenderArea;

		auto views = mDynamicRenderPassInfo.mRenderTargetViews;
		for (uint32_t i = 0; i < views.size(); ++i) {
			auto rtv = views[i];
			auto image = rtv->GetImage();
			if (image.Get() == pImage) {
				colorAttachment = i;
				baseArrayLayer = rtv->GetArrayLayer();
				break;
			}
		}
	}
	else {
		// active regular render pass
		auto pCurrentRenderPass = GetCurrentRenderPass();
		renderArea = pCurrentRenderPass->GetRenderArea();

		// Make sure pImage is a render target in current render pass
		const uint32_t renderTargetIndex = pCurrentRenderPass->GetRenderTargetImageIndex(pImage);
		colorAttachment = renderTargetIndex;

		auto view = pCurrentRenderPass->GetRenderTargetView(renderTargetIndex);
		baseArrayLayer = view->GetArrayLayer();
	}

	if (colorAttachment == UINT32_MAX) {
		ASSERT_MSG(false, "Passed image is not a render target.");
		return;
	}

	// Clear attachment
	VkClearAttachment attachment = {};
	attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	attachment.colorAttachment = colorAttachment;
	attachment.clearValue.color.float32[0] = clearValue.r;
	attachment.clearValue.color.float32[1] = clearValue.g;
	attachment.clearValue.color.float32[2] = clearValue.b;
	attachment.clearValue.color.float32[3] = clearValue.a;

	// Clear rect
	VkClearRect clearRect = {};
	clearRect.rect.offset = { renderArea.x, renderArea.y };
	clearRect.rect.extent = { renderArea.width, renderArea.height };
	clearRect.baseArrayLayer = baseArrayLayer;
	clearRect.layerCount = 1;

	vkCmdClearAttachments(
		mCommandBuffer,
		1,
		&attachment,
		1,
		&clearRect);
}

void CommandBuffer::ClearDepthStencil(
	Image* pImage,
	const DepthStencilClearValue& clearValue,
	uint32_t                            clearFlags)
{
	if (!HasActiveRenderPass()) {
		Warning("No active render pass.");
		return;
	}

	Rect renderArea;
	uint32_t   baseArrayLayer;

	// Dynamic render pass
	if (mDynamicRenderPassActive) {
		renderArea = mDynamicRenderPassInfo.mRenderArea;

		baseArrayLayer = mDynamicRenderPassInfo.mDepthStencilView->GetArrayLayer();
	}
	else {
		// active regular render pass
		auto pCurrentRenderPass = GetCurrentRenderPass();

		auto view = pCurrentRenderPass->GetDepthStencilView();

		renderArea = pCurrentRenderPass->GetRenderArea();

		baseArrayLayer = view->GetArrayLayer();
	}

	// Aspect mask
	VkImageAspectFlags aspectMask = 0;
	if (clearFlags & CLEAR_FLAG_DEPTH) {
		aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}
	if (clearFlags & CLEAR_FLAG_STENCIL) {
		aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	// Clear attachment
	VkClearAttachment attachment = {};
	attachment.aspectMask = aspectMask;
	attachment.clearValue.depthStencil.depth = clearValue.depth;
	attachment.clearValue.depthStencil.stencil = clearValue.stencil;

	// Clear rect
	VkClearRect clearRect = {};
	clearRect.rect.offset = { renderArea.x, renderArea.y };
	clearRect.rect.extent = { renderArea.width, renderArea.height };
	clearRect.baseArrayLayer = baseArrayLayer;
	clearRect.layerCount = 1;

	vkCmdClearAttachments(
		mCommandBuffer,
		1,
		&attachment,
		1,
		&clearRect);
}

void CommandBuffer::TransitionImageLayout(
	const Image* pImage,
	uint32_t            mipLevel,
	uint32_t            mipLevelCount,
	uint32_t            arrayLayer,
	uint32_t            arrayLayerCount,
	ResourceState beforeState,
	ResourceState afterState,
	const Queue* pSrcQueue,
	const Queue* pDstQueue)
{
	ASSERT_NULL_ARG(pImage);

	if ((!IsNull(pSrcQueue) && IsNull(pDstQueue)) || (IsNull(pSrcQueue) && !IsNull(pDstQueue))) {
		ASSERT_MSG(false, "queue family transfer requires both pSrcQueue and pDstQueue to be NOT NULL");
	}

	uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	if (!IsNull(pSrcQueue)) {
		srcQueueFamilyIndex = pSrcQueue->GetQueueFamilyIndex();
	}

	if (!IsNull(pDstQueue)) {
		dstQueueFamilyIndex = pDstQueue->GetQueueFamilyIndex();
	}

	if (srcQueueFamilyIndex == dstQueueFamilyIndex) {
		srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}

	if (beforeState == afterState && srcQueueFamilyIndex == dstQueueFamilyIndex) {
		return;
	}

	if (mipLevelCount == RemainingMipLevels) {
		mipLevelCount = pImage->GetMipLevelCount();
	}

	if (arrayLayerCount == RemainingArrayLayers) {
		arrayLayerCount = pImage->GetArrayLayerCount();
	}

	VkPipelineStageFlags srcStageMask = InvalidValue<VkPipelineStageFlags>();
	VkPipelineStageFlags dstStageMask = InvalidValue<VkPipelineStageFlags>();
	VkAccessFlags        srcAccessMask = InvalidValue<VkAccessFlags>();
	VkAccessFlags        dstAccessMask = InvalidValue<VkAccessFlags>();
	VkImageLayout        oldLayout = InvalidValue<VkImageLayout>();
	VkImageLayout        newLayout = InvalidValue<VkImageLayout>();
	VkDependencyFlags    dependencyFlags = 0;

	CommandType commandType = GetCommandType();

	Result ppxres = ToVkBarrierSrc(
		beforeState,
		commandType,
		GetDevice()->GetDeviceFeatures(),
		srcStageMask,
		srcAccessMask,
		oldLayout);
	ASSERT_MSG(ppxres == SUCCESS, "couldn't get src barrier data");

	ppxres = ToVkBarrierDst(
		afterState,
		commandType,
		GetDevice()->GetDeviceFeatures(),
		dstStageMask,
		dstAccessMask,
		newLayout);
	ASSERT_MSG(ppxres == SUCCESS, "couldn't get dst barrier data");

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
	barrier.image = pImage->GetVkImage();
	barrier.subresourceRange.aspectMask = pImage->GetVkImageAspectFlags();
	barrier.subresourceRange.baseMipLevel = mipLevel;
	barrier.subresourceRange.levelCount = mipLevelCount;
	barrier.subresourceRange.baseArrayLayer = arrayLayer;
	barrier.subresourceRange.layerCount = arrayLayerCount;

	vkCmdPipelineBarrier(
		mCommandBuffer,  // commandBuffer
		srcStageMask,    // srcStageMask
		dstStageMask,    // dstStageMask
		dependencyFlags, // dependencyFlags
		0,               // memoryBarrierCount
		nullptr,         // pMemoryBarriers
		0,               // bufferMemoryBarrierCount
		nullptr,         // pBufferMemoryBarriers
		1,               // imageMemoryBarrierCount
		&barrier);       // pImageMemoryBarriers);
}

void CommandBuffer::BufferResourceBarrier(
	const Buffer* pBuffer,
	ResourceState beforeState,
	ResourceState afterState,
	const Queue* pSrcQueue,
	const Queue* pDstQueue)
{
	ASSERT_NULL_ARG(pBuffer);

	if ((!IsNull(pSrcQueue) && IsNull(pDstQueue)) || (IsNull(pSrcQueue) && !IsNull(pDstQueue))) {
		ASSERT_MSG(false, "queue family transfer requires both pSrcQueue and pDstQueue to be NOT NULL");
	}

	uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	uint32_t dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	if (!IsNull(pSrcQueue)) {
		srcQueueFamilyIndex = pSrcQueue->GetQueueFamilyIndex();
	}

	if (!IsNull(pDstQueue)) {
		dstQueueFamilyIndex = pDstQueue->GetQueueFamilyIndex();
	}

	if (srcQueueFamilyIndex == dstQueueFamilyIndex) {
		srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	}

	if (beforeState == afterState && srcQueueFamilyIndex == dstQueueFamilyIndex) {
		return;
	}

	VkPipelineStageFlags srcStageMask = InvalidValue<VkPipelineStageFlags>();
	VkPipelineStageFlags dstStageMask = InvalidValue<VkPipelineStageFlags>();
	VkAccessFlags        srcAccessMask = InvalidValue<VkAccessFlags>();
	VkAccessFlags        dstAccessMask = InvalidValue<VkAccessFlags>();
	VkImageLayout        oldLayout = InvalidValue<VkImageLayout>();
	VkImageLayout        newLayout = InvalidValue<VkImageLayout>();
	VkDependencyFlags    dependencyFlags = 0;

	CommandType commandType = GetCommandType();

	Result ppxres = ToVkBarrierSrc(
		beforeState,
		commandType,
		GetDevice()->GetDeviceFeatures(),
		srcStageMask,
		srcAccessMask,
		oldLayout);
	ASSERT_MSG(ppxres == SUCCESS, "couldn't get src barrier data");

	ppxres = ToVkBarrierDst(
		afterState,
		commandType,
		GetDevice()->GetDeviceFeatures(),
		dstStageMask,
		dstAccessMask,
		newLayout);
	ASSERT_MSG(ppxres == SUCCESS, "couldn't get dst barrier data");

	VkBufferMemoryBarrier barrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
	barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
	barrier.buffer = pBuffer->GetVkBuffer();
	barrier.offset = static_cast<VkDeviceSize>(0);
	barrier.size = static_cast<VkDeviceSize>(pBuffer->GetSize());

	vkCmdPipelineBarrier(
		mCommandBuffer,  // commandBuffer
		srcStageMask,    // srcStageMask
		dstStageMask,    // dstStageMask
		dependencyFlags, // dependencyFlags
		0,               // memoryBarrierCount
		nullptr,         // pMemoryBarriers
		1,               // bufferMemoryBarrierCount
		&barrier,        // pBufferMemoryBarriers
		0,               // imageMemoryBarrierCount
		nullptr);        // pImageMemoryBarriers);
}

void CommandBuffer::SetViewports(uint32_t viewportCount, const Viewport* pViewports)
{
	VkViewport viewports[MaxViewports] = {};
	for (uint32_t i = 0; i < viewportCount; ++i) {
		// clang-format off
		viewports[i].x = pViewports[i].x;
		viewports[i].y = pViewports[i].height;
		viewports[i].width = pViewports[i].width;
		viewports[i].height = -pViewports[i].height;
		viewports[i].minDepth = pViewports[i].minDepth;
		viewports[i].maxDepth = pViewports[i].maxDepth;
		// clang-format on
	}

	vkCmdSetViewport(
		mCommandBuffer,
		0,
		viewportCount,
		viewports);
}

void CommandBuffer::SetScissors(uint32_t scissorCount, const Rect* pScissors)
{
	vkCmdSetScissor(
		mCommandBuffer,
		0,
		scissorCount,
		reinterpret_cast<const VkRect2D*>(pScissors));
}

void CommandBuffer::BindDescriptorSets(
	VkPipelineBindPoint               bindPoint,
	const PipelineInterface* pInterface,
	uint32_t                          setCount,
	const DescriptorSet* const* ppSets)
{
	ASSERT_NULL_ARG(pInterface);

	// D3D12 needs the pipeline interface (root signature) bound even if there
	// aren't any descriptor sets. Since Vulkan doesn't require this, we'll
	// just treat it as a NOOP if setCount is zero.
	//
	if (setCount == 0) {
		return;
	}

	// Get set numbers
	const std::vector<uint32_t>& setNumbers = pInterface->GetSetNumbers();

	// setCount cannot exceed the number of sets in the pipeline interface
	uint32_t setNumberCount = CountU32(setNumbers);
	if (setCount > setNumberCount) {
		ASSERT_MSG(false, "setCount exceeds the number of sets in pipeline interface");
	}

	if (setCount > 0) {
		// Get Vulkan handles
		VkDescriptorSet vkSets[MaxBoundDescriptorSets] = { VK_NULL_HANDLE };
		for (uint32_t i = 0; i < setCount; ++i) {
			vkSets[i] = ppSets[i]->GetVkDescriptorSet();
		}

		// If we have consecutive set numbers we can bind just once...
		if (pInterface->HasConsecutiveSetNumbers()) {
			uint32_t firstSet = setNumbers[0];

			vkCmdBindDescriptorSets(
				mCommandBuffer,                           // commandBuffer
				bindPoint,                                // pipelineBindPoint
				pInterface->GetVkPipelineLayout(), // layout
				firstSet,                                 // firstSet
				setCount,                                 // descriptorSetCount
				vkSets,                                   // pDescriptorSets
				0,                                        // dynamicOffsetCount
				nullptr);                                 // pDynamicOffsets
		}
		// ...otherwise we get to bind a bunch of times
		else {
			for (uint32_t i = 0; i < setCount; ++i) {
				uint32_t firstSet = setNumbers[i];

				vkCmdBindDescriptorSets(
					mCommandBuffer,                           // commandBuffer
					bindPoint,                                // pipelineBindPoint
					pInterface->GetVkPipelineLayout(), // layout
					firstSet,                                 // firstSet
					1,                                        // descriptorSetCount
					&vkSets[i],                               // pDescriptorSets
					0,                                        // dynamicOffsetCount
					nullptr);                                 // pDynamicOffsets
			}
		}
	}
	else {
		vkCmdBindDescriptorSets(
			mCommandBuffer,                           // commandBuffer
			bindPoint,                                // pipelineBindPoint
			pInterface->GetVkPipelineLayout(), // layout
			0,                                        // firstSet
			0,                                        // descriptorSetCount
			nullptr,                                  // pDescriptorSets
			0,                                        // dynamicOffsetCount
			nullptr);                                 // pDynamicOffsets
	}
}

void CommandBuffer::BindGraphicsDescriptorSets(
	const PipelineInterface* pInterface,
	uint32_t                          setCount,
	const DescriptorSet* const* ppSets)
{
	BindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pInterface, setCount, ppSets);
}

void CommandBuffer::PushConstants(
	const PipelineInterface* pInterface,
	uint32_t                       count,
	const void* pValues,
	uint32_t                       dstOffset)
{
	ASSERT_NULL_ARG(pInterface);
	ASSERT_NULL_ARG(pValues);

	ASSERT_MSG(((dstOffset + count) <= MaxPushConstants), "dstOffset + count (" + std::to_string(dstOffset + count) + ") exceeds PPX_MAX_PUSH_CONSTANTS (" + std::to_string(MaxPushConstants) + ")");

	const uint32_t sizeInBytes = count * sizeof(uint32_t);
	const uint32_t offsetInBytes = dstOffset * sizeof(uint32_t);

	vkCmdPushConstants(
		mCommandBuffer,
		pInterface->GetVkPipelineLayout(),
		pInterface->GetPushConstantShaderStageFlags(),
		offsetInBytes,
		sizeInBytes,
		pValues);
}

void CommandBuffer::PushGraphicsConstants(
	const PipelineInterface* pInterface,
	uint32_t                       count,
	const void* pValues,
	uint32_t                       dstOffset)
{
	ASSERT_MSG(((dstOffset + count) <= MaxPushConstants), "dstOffset + count (" + std::to_string(dstOffset + count) + ") exceeds PPX_MAX_PUSH_CONSTANTS (" + std::to_string(MaxPushConstants) + ")");

	const VkShaderStageFlags shaderStageFlags = pInterface->GetPushConstantShaderStageFlags();
	if ((shaderStageFlags & VK_SHADER_STAGE_ALL_GRAPHICS) == 0) {
		ASSERT_MSG(false, "push constants shader visibility flags in pInterface does not have any graphics stages");
	}

	PushConstants(pInterface, count, pValues, dstOffset);
}

void CommandBuffer::BindGraphicsPipeline(const GraphicsPipeline* pPipeline)
{
	ASSERT_NULL_ARG(pPipeline);

	vkCmdBindPipeline(
		mCommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pPipeline->GetVkPipeline());
}

void CommandBuffer::BindComputeDescriptorSets(
	const PipelineInterface* pInterface,
	uint32_t                          setCount,
	const DescriptorSet* const* ppSets)
{
	BindDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, pInterface, setCount, ppSets);
}

void CommandBuffer::PushComputeConstants(
	const PipelineInterface* pInterface,
	uint32_t                       count,
	const void* pValues,
	uint32_t                       dstOffset)
{
	ASSERT_MSG(((dstOffset + count) <= MaxPushConstants), "dstOffset + count (" + std::to_string(dstOffset + count) + ") exceeds PPX_MAX_PUSH_CONSTANTS (" + std::to_string(MaxPushConstants) + ")");

	const VkShaderStageFlags shaderStageFlags = pInterface->GetPushConstantShaderStageFlags();
	if ((shaderStageFlags & VK_SHADER_STAGE_COMPUTE_BIT) == 0) {
		ASSERT_MSG(false, "push constants shader visibility flags in pInterface does not have compute stage");
	}

	PushConstants(pInterface, count, pValues, dstOffset);
}

void CommandBuffer::BindComputePipeline(const ComputePipeline* pPipeline)
{
	ASSERT_NULL_ARG(pPipeline);

	vkCmdBindPipeline(
		mCommandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pPipeline->GetVkPipeline());
}

void CommandBuffer::BindIndexBuffer(const IndexBufferView* pView)
{
	ASSERT_NULL_ARG(pView);
	ASSERT_NULL_ARG(pView->pBuffer);

	vkCmdBindIndexBuffer(
		mCommandBuffer,
		pView->pBuffer->GetVkBuffer(),
		static_cast<VkDeviceSize>(pView->offset),
		ToVkIndexType(pView->indexType));
}

void CommandBuffer::BindVertexBuffers(uint32_t viewCount, const VertexBufferView* pViews)
{
	ASSERT_NULL_ARG(pViews);
	ASSERT_MSG(viewCount < MaxVertexBindings, "viewCount exceeds PPX_MAX_VERTEX_ATTRIBUTES");

	VkBuffer     buffers[MaxRenderTargets] = { VK_NULL_HANDLE };
	VkDeviceSize offsets[MaxRenderTargets] = { 0 };

	for (uint32_t i = 0; i < viewCount; ++i) {
		buffers[i] = pViews[i].pBuffer->GetVkBuffer();
		offsets[i] = static_cast<VkDeviceSize>(pViews[i].offset);
	}

	vkCmdBindVertexBuffers(
		mCommandBuffer,
		0,
		viewCount,
		buffers,
		offsets);
}

void CommandBuffer::Draw(
	uint32_t vertexCount,
	uint32_t instanceCount,
	uint32_t firstVertex,
	uint32_t firstInstance)
{
	vkCmdDraw(mCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(
	uint32_t indexCount,
	uint32_t instanceCount,
	uint32_t firstIndex,
	int32_t  vertexOffset,
	uint32_t firstInstance)
{
	vkCmdDrawIndexed(mCommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::Dispatch(
	uint32_t groupCountX,
	uint32_t groupCountY,
	uint32_t groupCountZ)
{
	vkCmdDispatch(mCommandBuffer, groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::CopyBufferToBuffer(
	const BufferToBufferCopyInfo* pCopyInfo,
	Buffer* pSrcBuffer,
	Buffer* pDstBuffer)
{
	VkBufferCopy region = {};
	region.srcOffset = static_cast<VkDeviceSize>(pCopyInfo->srcBuffer.offset);
	region.dstOffset = static_cast<VkDeviceSize>(pCopyInfo->dstBuffer.offset);
	region.size = static_cast<VkDeviceSize>(pCopyInfo->size);

	vkCmdCopyBuffer(
		mCommandBuffer,
		pSrcBuffer->GetVkBuffer(),
		pDstBuffer->GetVkBuffer(),
		1,
		&region);
}

void CommandBuffer::CopyBufferToImage(
	const std::vector<BufferToImageCopyInfo>& pCopyInfos,
	Buffer* pSrcBuffer,
	Image* pDstImage)
{
	ASSERT_NULL_ARG(pSrcBuffer);
	ASSERT_NULL_ARG(pDstImage);

	std::vector<VkBufferImageCopy> regions(pCopyInfos.size());

	for (size_t i = 0; i < pCopyInfos.size(); i++) {
		regions[i].bufferOffset = static_cast<VkDeviceSize>(pCopyInfos[i].srcBuffer.footprintOffset);
		regions[i].bufferRowLength = pCopyInfos[i].srcBuffer.imageWidth;
		regions[i].bufferImageHeight = pCopyInfos[i].srcBuffer.imageHeight;
		regions[i].imageSubresource.aspectMask = pDstImage->GetVkImageAspectFlags();
		regions[i].imageSubresource.mipLevel = pCopyInfos[i].dstImage.mipLevel;
		regions[i].imageSubresource.baseArrayLayer = pCopyInfos[i].dstImage.arrayLayer;
		regions[i].imageSubresource.layerCount = pCopyInfos[i].dstImage.arrayLayerCount;
		regions[i].imageOffset.x = pCopyInfos[i].dstImage.x;
		regions[i].imageOffset.y = pCopyInfos[i].dstImage.y;
		regions[i].imageOffset.z = pCopyInfos[i].dstImage.z;
		regions[i].imageExtent.width = pCopyInfos[i].dstImage.width;
		regions[i].imageExtent.height = pCopyInfos[i].dstImage.height;
		regions[i].imageExtent.depth = pCopyInfos[i].dstImage.depth;
	}

	vkCmdCopyBufferToImage(
		mCommandBuffer,
		pSrcBuffer->GetVkBuffer(),
		pDstImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		static_cast<uint32_t>(pCopyInfos.size()),
		regions.data());
}

void CommandBuffer::CopyBufferToImage(
	const BufferToImageCopyInfo* pCopyInfo,
	Buffer* pSrcBuffer,
	Image* pDstImage)
{
	return CopyBufferToImage(std::vector<BufferToImageCopyInfo>{*pCopyInfo}, pSrcBuffer, pDstImage);
}

ImageToBufferOutputPitch CommandBuffer::CopyImageToBuffer(
	const ImageToBufferCopyInfo* pCopyInfo,
	Image* pSrcImage,
	Buffer* pDstBuffer)
{
	std::vector<VkBufferImageCopy> regions;

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0; // Tightly-packed texels.
	region.bufferImageHeight = 0; // Tightly-packed texels.
	region.imageSubresource.mipLevel = pCopyInfo->srcImage.mipLevel;
	region.imageSubresource.baseArrayLayer = pCopyInfo->srcImage.arrayLayer;
	region.imageSubresource.layerCount = pCopyInfo->srcImage.arrayLayerCount;
	region.imageOffset.x = pCopyInfo->srcImage.offset.x;
	region.imageOffset.y = pCopyInfo->srcImage.offset.y;
	region.imageOffset.z = pCopyInfo->srcImage.offset.z;
	region.imageExtent = { 0, 1, 1 };
	region.imageExtent.width = pCopyInfo->extent.x;
	if (pSrcImage->GetType() != ImageType::Image1D) { // Can only be set for 2D and 3D textures.
		region.imageExtent.height = pCopyInfo->extent.y;
	}
	if (pSrcImage->GetType() == ImageType::Image3D) { // Can only be set for 3D textures.
		region.imageExtent.depth = pCopyInfo->extent.z;
	}

	const FormatDesc* srcDesc = GetFormatDescription(pSrcImage->GetFormat());

	// For depth-stencil images, each component must be copied separately.
	if (srcDesc->aspect == FORMAT_ASPECT_DEPTH_STENCIL) {
		// First copy depth.
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		regions.push_back(region);

		// Compute the total size of the depth part to offset the stencil part.
		// We always copy tightly-packed texels, so we don't have to worry
		// about tiling.
		size_t depthTexelBytes = srcDesc->bytesPerTexel - 1; // Stencil is always 1 byte.
		size_t depthTotalBytes = depthTexelBytes * pCopyInfo->extent.x * pCopyInfo->extent.y;

		// Then copy stencil.
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
		region.bufferOffset = depthTotalBytes;
		regions.push_back(region);
	}
	else {
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions.push_back(region);
	}

	vkCmdCopyImageToBuffer(
		mCommandBuffer,
		pSrcImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDstBuffer->GetVkBuffer(),
		static_cast<uint32_t>(regions.size()),
		regions.data());

	ImageToBufferOutputPitch outPitch;
	outPitch.rowPitch = srcDesc->bytesPerTexel * pCopyInfo->extent.x;
	return outPitch;
}

void CommandBuffer::CopyImageToImage(
	const ImageToImageCopyInfo* pCopyInfo,
	Image* pSrcImage,
	Image* pDstImage)
{
	bool isSourceDepthStencil = GetFormatDescription(pSrcImage->GetFormat())->aspect == FORMAT_ASPECT_DEPTH_STENCIL;
	bool isDestDepthStencil = GetFormatDescription(pDstImage->GetFormat())->aspect == FORMAT_ASPECT_DEPTH_STENCIL;
	ASSERT_MSG(isSourceDepthStencil == isDestDepthStencil, "both images in an image copy must be depth-stencil if one is depth-stencil");

	VkImageSubresourceLayers srcSubresource = {};
	srcSubresource.aspectMask = DetermineAspectMask(pSrcImage->GetVkFormat());
	srcSubresource.baseArrayLayer = pCopyInfo->srcImage.arrayLayer;
	srcSubresource.layerCount = pCopyInfo->srcImage.arrayLayerCount;
	srcSubresource.mipLevel = pCopyInfo->srcImage.mipLevel;

	VkImageSubresourceLayers dstSubresource = {};
	dstSubresource.aspectMask = DetermineAspectMask(pDstImage->GetVkFormat());
	dstSubresource.baseArrayLayer = pCopyInfo->dstImage.arrayLayer;
	dstSubresource.layerCount = pCopyInfo->dstImage.arrayLayerCount;
	dstSubresource.mipLevel = pCopyInfo->dstImage.mipLevel;

	VkImageCopy region = {};
	region.srcOffset = {
		static_cast<int32_t>(pCopyInfo->srcImage.offset.x),
		static_cast<int32_t>(pCopyInfo->srcImage.offset.y),
		static_cast<int32_t>(pCopyInfo->srcImage.offset.z) };
	region.srcSubresource = srcSubresource;
	region.dstOffset = {
		static_cast<int32_t>(pCopyInfo->dstImage.offset.x),
		static_cast<int32_t>(pCopyInfo->dstImage.offset.y),
		static_cast<int32_t>(pCopyInfo->dstImage.offset.z) };
	region.dstSubresource = dstSubresource;
	region.extent = { 0, 1, 1 };
	region.extent.width = pCopyInfo->extent.x;
	if (pSrcImage->GetType() != ImageType::Image1D) { // Can only be set for 2D and 3D textures.
		region.extent.height = pCopyInfo->extent.y;
	}
	if (pSrcImage->GetType() == ImageType::Image3D) { // Can only be set for 3D textures.
		region.extent.height = pCopyInfo->extent.z;
	}

	vkCmdCopyImage(
		mCommandBuffer,
		pSrcImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDstImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);
}

void CommandBuffer::BlitImage(
	const ImageBlitInfo* pCopyInfo,
	Image* pSrcImage,
	Image* pDstImage)
{
	bool isSourceDepthOrStencil = GetFormatDescription(pSrcImage->GetFormat())->aspect & FORMAT_ASPECT_DEPTH_STENCIL;
	bool isDestDepthOrStencil = GetFormatDescription(pDstImage->GetFormat())->aspect & FORMAT_ASPECT_DEPTH_STENCIL;
	if (isSourceDepthOrStencil || isDestDepthOrStencil) {
		ASSERT_MSG(pSrcImage->GetFormat() == pDstImage->GetFormat(), "both images in an image copy must have the same format if either has depth or stencil");
	}

	VkImageSubresourceLayers srcSubresource = {};
	srcSubresource.aspectMask = DetermineAspectMask(pSrcImage->GetVkFormat());
	srcSubresource.baseArrayLayer = pCopyInfo->srcImage.arrayLayer;
	srcSubresource.layerCount = pCopyInfo->srcImage.arrayLayerCount;
	srcSubresource.mipLevel = pCopyInfo->srcImage.mipLevel;

	VkImageSubresourceLayers dstSubresource = {};
	dstSubresource.aspectMask = DetermineAspectMask(pDstImage->GetVkFormat());
	dstSubresource.baseArrayLayer = pCopyInfo->dstImage.arrayLayer;
	dstSubresource.layerCount = pCopyInfo->dstImage.arrayLayerCount;
	dstSubresource.mipLevel = pCopyInfo->dstImage.mipLevel;

	VkImageBlit region = {};
	region.srcSubresource = srcSubresource;
	region.dstSubresource = dstSubresource;
	for (int i = 0; i < 2; ++i) {
		region.srcOffsets[i] = {
			static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].x),
			static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].y),
			static_cast<int32_t>(pCopyInfo->srcImage.offsets[i].z) };
		region.dstOffsets[i] = {
			static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].x),
			static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].y),
			static_cast<int32_t>(pCopyInfo->dstImage.offsets[i].z) };
	}

	VkFilter filter = ToVkEnum(pCopyInfo->filter);

	vkCmdBlitImage(
		mCommandBuffer,
		pSrcImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		pDstImage->GetVkImage(),
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region,
		filter);
}

void CommandBuffer::BeginQuery(
	const Query* pQuery,
	uint32_t           queryIndex)
{
	ASSERT_NULL_ARG(pQuery);
	ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

	VkQueryControlFlags flags = 0;
	if (pQuery->GetType() == QueryType::Occlusion) {
		flags = VK_QUERY_CONTROL_PRECISE_BIT;
	}

	vkCmdBeginQuery(
		mCommandBuffer,
		pQuery->GetVkQueryPool(),
		queryIndex,
		flags);
}

void CommandBuffer::EndQuery(
	const Query* pQuery,
	uint32_t           queryIndex)
{
	ASSERT_NULL_ARG(pQuery);
	ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");

	vkCmdEndQuery(
		mCommandBuffer,
		pQuery->GetVkQueryPool(),
		queryIndex);
}

void CommandBuffer::WriteTimestamp(
	const Query* pQuery,
	PipelineStage pipelineStage,
	uint32_t            queryIndex)
{
	ASSERT_MSG(queryIndex <= pQuery->GetCount(), "invalid query index");
	vkCmdWriteTimestamp(
		mCommandBuffer,
		ToVkPipelineStage(pipelineStage),
		pQuery->GetVkQueryPool(),
		queryIndex);
}

void CommandBuffer::ResolveQueryData(
	Query* pQuery,
	uint32_t     startIndex,
	uint32_t     numQueries)
{
	ASSERT_MSG((startIndex + numQueries) <= pQuery->GetCount(), "invalid query index/number");
	const VkQueryResultFlags flags = VK_QUERY_RESULT_WAIT_BIT | VK_QUERY_RESULT_64_BIT;
	vkCmdCopyQueryPoolResults(mCommandBuffer, pQuery->GetVkQueryPool(), startIndex, numQueries, pQuery->GetReadBackBuffer(), 0, pQuery->GetQueryTypeSize(), flags);
}

#pragma endregion

#pragma region Queue

Result Queue::CreateCommandBuffer(
	CommandBuffer** ppCommandBuffer,
	uint32_t              resourceDescriptorCount,
	uint32_t              samplerDescriptorCount)
{
	std::lock_guard<std::mutex> lock(mCommandSetMutex);

	CommandSet set = {};

	CommandPoolCreateInfo ci = {};
	ci.pQueue = this;

	Result ppxres = GetDevice()->CreateCommandPool(ci, &set.commandPool);
	if (Failed(ppxres)) {
		return ppxres;
	}

	ppxres = GetDevice()->AllocateCommandBuffer(set.commandPool, &set.commandBuffer, resourceDescriptorCount, samplerDescriptorCount);
	if (Failed(ppxres)) {
		GetDevice()->DestroyCommandPool(set.commandPool);
		return ppxres;
	}

	*ppCommandBuffer = set.commandBuffer;

	mCommandSets.push_back(set);

	return SUCCESS;
}

void Queue::DestroyCommandBuffer(const CommandBuffer* pCommandBuffer)
{
	std::lock_guard<std::mutex> lock(mCommandSetMutex);

	auto it = std::find_if(
		std::begin(mCommandSets),
		std::end(mCommandSets),
		[pCommandBuffer](const CommandSet& elem) -> bool {
			bool isSame = (elem.commandBuffer.Get() == pCommandBuffer);
			return isSame; });
	if (it == std::end(mCommandSets)) {
		return;
	}

	CommandSet& set = *it;

	GetDevice()->FreeCommandBuffer(set.commandBuffer);
	GetDevice()->DestroyCommandPool(set.commandPool);

	RemoveElementIf(
		mCommandSets,
		[set](const CommandSet& elem) -> bool {
			bool isSamePool = (elem.commandPool == set.commandPool);
			bool isSameBuffer = (elem.commandBuffer == set.commandBuffer);
			bool isSame = isSamePool && isSameBuffer;
			return isSame; });
}

Result Queue::CopyBufferToBuffer(
	const BufferToBufferCopyInfo* pCopyInfo,
	Buffer* pSrcBuffer,
	Buffer* pDstBuffer,
	ResourceState                 stateBefore,
	ResourceState                 stateAfter)
{
	ScopeDestroyer SCOPED_DESTROYER(GetDevice());

	// Create command buffer
	CommandBufferPtr cmd;
	Result                 ppxres = CreateCommandBuffer(&cmd, 0, 0);
	if (Failed(ppxres)) {
		return ppxres;
	}
	SCOPED_DESTROYER.AddObject(this, cmd);

	// Build command buffer
	{
		ppxres = cmd->Begin();
		if (Failed(ppxres)) {
			return ppxres;
		}

		cmd->BufferResourceBarrier(pDstBuffer, stateBefore, ResourceState::CopyDst);
		cmd->CopyBufferToBuffer(pCopyInfo, pSrcBuffer, pDstBuffer);
		cmd->BufferResourceBarrier(pDstBuffer, ResourceState::CopyDst, stateAfter);

		ppxres = cmd->End();
		if (Failed(ppxres)) {
			return ppxres;
		}
	}

	// Submit command buffer
	SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.ppCommandBuffers = &cmd;
	//
	ppxres = Submit(&submit);
	if (Failed(ppxres)) {
		return ppxres;
	}

	// Wait work completion
	ppxres = WaitIdle();
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result Queue::CopyBufferToImage(
	const std::vector<BufferToImageCopyInfo>& pCopyInfos,
	Buffer* pSrcBuffer,
	Image* pDstImage,
	uint32_t                                        mipLevel,
	uint32_t                                        mipLevelCount,
	uint32_t                                        arrayLayer,
	uint32_t                                        arrayLayerCount,
	ResourceState                             stateBefore,
	ResourceState                             stateAfter)
{
	ScopeDestroyer SCOPED_DESTROYER(GetDevice());

	// Create command buffer
	CommandBufferPtr cmd;
	Result                 ppxres = CreateCommandBuffer(&cmd, 0, 0);
	if (Failed(ppxres)) {
		return ppxres;
	}
	SCOPED_DESTROYER.AddObject(this, cmd);

	// Build command buffer
	{
		ppxres = cmd->Begin();
		if (Failed(ppxres)) {
			return ppxres;
		}

		cmd->TransitionImageLayout(pDstImage, ALL_SUBRESOURCES, stateBefore, ResourceState::CopyDst);
		cmd->CopyBufferToImage(pCopyInfos, pSrcBuffer, pDstImage);
		cmd->TransitionImageLayout(pDstImage, ALL_SUBRESOURCES, ResourceState::CopyDst, stateAfter);

		ppxres = cmd->End();
		if (Failed(ppxres)) {
			return ppxres;
		}
	}

	// Submit command buffer
	SubmitInfo submit;
	submit.commandBufferCount = 1;
	submit.ppCommandBuffers = &cmd;
	//
	ppxres = Submit(&submit);
	if (Failed(ppxres)) {
		return ppxres;
	}

	// Wait work completion
	ppxres = WaitIdle();
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

Result Queue::createApiObjects(const internal::QueueCreateInfo& createInfo)
{
	if (createInfo.commandType == COMMAND_TYPE_UNDEFINED)
		return ERROR_API_FAILURE;
	else if (createInfo.commandType == COMMAND_TYPE_GRAPHICS)
		m_queue = GetDevice()->GetGraphicsDeviceQueue();
	else if (createInfo.commandType == COMMAND_TYPE_COMPUTE)
		m_queue = GetDevice()->GetComputeDeviceQueue();
	else if (createInfo.commandType == COMMAND_TYPE_TRANSFER)
		m_queue = GetDevice()->GetTransferDeviceQueue();
	else if (createInfo.commandType == COMMAND_TYPE_PRESENT)
		m_queue = GetDevice()->GetPresentDeviceQueue();

	VkCommandPoolCreateInfo vkci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	vkci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	vkci.queueFamilyIndex = m_queue->QueueFamily;

	VkResult vkres = vkCreateCommandPool(
		GetDevice()->GetVkDevice(),
		&vkci,
		nullptr,
		&mTransientPool);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkCreateCommandPool failed" + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Queue::destroyApiObjects()
{
	if (mTransientPool) {
		vkDestroyCommandPool(
			GetDevice()->GetVkDevice(),
			mTransientPool,
			nullptr);
		mTransientPool.Reset();
	}

	if (m_queue)
	{
		WaitIdle();
		m_queue.reset();
	}
}

Result Queue::WaitIdle()
{
	// Synchronized queue access
	std::lock_guard<std::mutex> lock(mQueueMutex);

	VkResult vkres = vkQueueWaitIdle(m_queue->Queue);
	if (vkres != VK_SUCCESS) {
		ASSERT_MSG(false, "vkQueueWaitIdle failed" + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

Result Queue::Submit(const SubmitInfo* pSubmitInfo)
{
	// Command buffers
	std::vector<VkCommandBuffer> commandBuffers;
	for (uint32_t i = 0; i < pSubmitInfo->commandBufferCount; ++i) {
		commandBuffers.push_back(pSubmitInfo->ppCommandBuffers[i]->GetVkCommandBuffer());
	}

	// Wait semaphores
	std::vector<VkSemaphore>          waitSemaphores;
	std::vector<VkPipelineStageFlags> waitDstStageMasks;
	for (uint32_t i = 0; i < pSubmitInfo->waitSemaphoreCount; ++i) {
		waitSemaphores.push_back(pSubmitInfo->ppWaitSemaphores[i]->GetVkSemaphore());
		waitDstStageMasks.push_back(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
	}

	// Signal semaphores
	std::vector<VkSemaphore> signalSemaphores;
	for (uint32_t i = 0; i < pSubmitInfo->signalSemaphoreCount; ++i) {
		signalSemaphores.push_back(pSubmitInfo->ppSignalSemaphores[i]->GetVkSemaphore());
	}

	VkTimelineSemaphoreSubmitInfo timelineSubmitInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	timelineSubmitInfo.pNext = nullptr;
	timelineSubmitInfo.waitSemaphoreValueCount = CountU32(pSubmitInfo->waitValues);
	timelineSubmitInfo.pWaitSemaphoreValues = DataPtr(pSubmitInfo->waitValues);
	timelineSubmitInfo.signalSemaphoreValueCount = CountU32(pSubmitInfo->signalValues);
	timelineSubmitInfo.pSignalSemaphoreValues = DataPtr(pSubmitInfo->signalValues);

	VkSubmitInfo vksi = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	vksi.pNext = &timelineSubmitInfo;
	vksi.waitSemaphoreCount = CountU32(waitSemaphores);
	vksi.pWaitSemaphores = DataPtr(waitSemaphores);
	vksi.pWaitDstStageMask = DataPtr(waitDstStageMasks);
	vksi.commandBufferCount = CountU32(commandBuffers);
	vksi.pCommandBuffers = DataPtr(commandBuffers);
	vksi.signalSemaphoreCount = CountU32(signalSemaphores);
	vksi.pSignalSemaphores = DataPtr(signalSemaphores);

	// Fence
	VkFence fence = VK_NULL_HANDLE;
	if (!IsNull(pSubmitInfo->pFence)) {
		fence = pSubmitInfo->pFence->GetVkFence();
	}

	// Synchronized queue access
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);

		VkResult vkres = vkQueueSubmit(
			m_queue->Queue,
			1,
			&vksi,
			fence);
		if (vkres != VK_SUCCESS) {
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

Result Queue::QueueWait(Semaphore* pSemaphore, uint64_t value)
{
	if (IsNull(pSemaphore)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	if (pSemaphore->GetSemaphoreType() != SemaphoreType::Timeline) {
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	VkSemaphore semaphoreHandle = pSemaphore->GetVkSemaphore();

	VkTimelineSemaphoreSubmitInfo timelineSubmitInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	timelineSubmitInfo.pNext = nullptr;
	timelineSubmitInfo.waitSemaphoreValueCount = 1;
	timelineSubmitInfo.pWaitSemaphoreValues = &value;

	VkSubmitInfo vksi = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	vksi.pNext = &timelineSubmitInfo;
	vksi.waitSemaphoreCount = 1;
	vksi.pWaitSemaphores = &semaphoreHandle;

	// Synchronized queue access
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);

		VkResult vkres = vkQueueSubmit(
			m_queue->Queue,
			1,
			&vksi,
			VK_NULL_HANDLE);
		if (vkres != VK_SUCCESS) {
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

Result Queue::QueueSignal(Semaphore* pSemaphore, uint64_t value)
{
	if (IsNull(pSemaphore)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	if (pSemaphore->GetSemaphoreType() != SemaphoreType::Timeline) {
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	VkSemaphore semaphoreHandle = pSemaphore->GetVkSemaphore();

	VkTimelineSemaphoreSubmitInfo timelineSubmitInfo = { VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
	timelineSubmitInfo.pNext = nullptr;
	timelineSubmitInfo.signalSemaphoreValueCount = 1;
	timelineSubmitInfo.pSignalSemaphoreValues = &value;

	VkSubmitInfo vksi = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	vksi.pNext = &timelineSubmitInfo;
	vksi.signalSemaphoreCount = 1;
	vksi.pSignalSemaphores = &semaphoreHandle;

	// Synchronized queue access
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);

		VkResult vkres = vkQueueSubmit(
			m_queue->Queue,
			1,
			&vksi,
			VK_NULL_HANDLE);
		if (vkres != VK_SUCCESS) {
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

Result Queue::GetTimestampFrequency(uint64_t* pFrequency) const
{
	if (IsNull(pFrequency)) {
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	float  timestampPeriod = GetDevice()->GetDeviceTimestampPeriod();
	double ticksPerSecond = 1000000000.0 / static_cast<double>(timestampPeriod);
	*pFrequency = static_cast<uint64_t>(ticksPerSecond);

	return SUCCESS;
}

static VkResult CmdTransitionImageLayout(
	VkCommandBuffer      commandBuffer,
	VkImage              image,
	VkImageAspectFlags   aspectMask,
	uint32_t             baseMipLevel,
	uint32_t             levelCount,
	uint32_t             baseArrayLayer,
	uint32_t             layerCount,
	VkImageLayout        oldLayout,
	VkImageLayout        newLayout,
	VkPipelineStageFlags newPipelineStage)
{
	VkPipelineStageFlags srcStageMask = 0;
	VkPipelineStageFlags dstStageMask = 0;
	VkAccessFlags        srcAccessMask = 0;
	VkAccessFlags        dstAccessMask = 0;
	VkDependencyFlags    dependencyFlags = 0;

	switch (oldLayout) {
	default: {
		ASSERT_MSG(false, "invalid value for oldLayout");
		return VK_ERROR_INITIALIZATION_FAILED;
	} break;

	case VK_IMAGE_LAYOUT_UNDEFINED: {
		srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_GENERAL: {
		// @TODO: This may need tweaking.
		srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
		srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_PREINITIALIZED: {
		srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
		srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
		// @TODO: This may need tweaking.
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
		// @TODO: This may need tweaking.
		srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
		srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT: {
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
		srcAccessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
	} break;

	case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR: {
		srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		srcAccessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
	} break;
	}

	switch (newLayout) {
	default: {
		// Note: VK_IMAGE_LAYOUT_UNDEFINED and VK_IMAGE_LAYOUT_PREINITIALIZED cannot be a destination layout.
		ASSERT_MSG(false, "invalid value for newLayout");
		return VK_ERROR_INITIALIZATION_FAILED;
	} break;

	case VK_IMAGE_LAYOUT_GENERAL: {
		dstStageMask = newPipelineStage;
		dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
		dstStageMask = newPipelineStage;
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	} break;

	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
		dstStageMask = newPipelineStage;
		dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	} break;

	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: {
		dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	} break;

	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
		dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstAccessMask = 0;
	} break;

	case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT: {
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
		dstAccessMask = VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
	} break;

	case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR: {
		dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
		dstAccessMask = VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
	} break;
	}

	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = srcAccessMask;
	barrier.dstAccessMask = dstAccessMask;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.aspectMask = aspectMask;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount = levelCount,
		barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount = layerCount;

	vkCmdPipelineBarrier(
		commandBuffer,   // commandBuffer
		srcStageMask,    // srcStageMask
		dstStageMask,    // dstStageMask
		dependencyFlags, // dependencyFlags
		0,               // memoryBarrierCount
		nullptr,         // pMemoryBarriers
		0,               // bufferMemoryBarrierCount
		nullptr,         // pBufferMemoryBarriers
		1,               // imageMemoryBarrierCount
		&barrier);       // pImageMemoryBarriers);

	return VK_SUCCESS;
}

VkResult Queue::TransitionImageLayout(
	VkImage              image,
	VkImageAspectFlags   aspectMask,
	uint32_t             baseMipLevel,
	uint32_t             levelCount,
	uint32_t             baseArrayLayer,
	uint32_t             layerCount,
	VkImageLayout        oldLayout,
	VkImageLayout        newLayout,
	VkPipelineStageFlags newPipelineStage)
{
	VkCommandBufferAllocateInfo vkai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	vkai.commandPool = mTransientPool;
	vkai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	vkai.commandBufferCount = 1;

	VkCommandBufferPtr commandBuffer;
	// Synchronize command pool access
	{
		std::lock_guard<std::mutex> lock(mCommandPoolMutex);

		VkResult vkres = vkAllocateCommandBuffers(
			GetDevice()->GetVkDevice(),
			&vkai,
			&commandBuffer);
		if (vkres != VK_SUCCESS) {
			ASSERT_MSG(false, "vkAllocateCommandBuffers failed" + ToString(vkres));
			return vkres;
		}
	}

	// Save ourselves from having to write a bunch of mutex locks
	auto FreeCommandBuffer = [this, &commandBuffer] {
		std::lock_guard<std::mutex> lock(mCommandPoolMutex);

		vkFreeCommandBuffers(GetDevice()->GetVkDevice(), this->mTransientPool, 1, commandBuffer);
		};

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	beginInfo.pInheritanceInfo = nullptr;

	VkResult vkres = vkBeginCommandBuffer(commandBuffer, &beginInfo);
	if (vkres != VK_SUCCESS) {
		FreeCommandBuffer();
		ASSERT_MSG(false, "vkBeginCommandBuffer failed" + ToString(vkres));
		return vkres;
	}

	vkres = CmdTransitionImageLayout(
		commandBuffer,
		image,
		aspectMask,
		baseMipLevel,
		levelCount,
		baseArrayLayer,
		layerCount,
		oldLayout,
		newLayout,
		newPipelineStage);
	if (vkres != VK_SUCCESS) {
		FreeCommandBuffer();
		ASSERT_MSG(false, "CmdTransitionImageLayout failed" + ToString(vkres));
		return vkres;
	}

	vkres = vkEndCommandBuffer(commandBuffer);
	if (vkres != VK_SUCCESS) {
		FreeCommandBuffer();
		ASSERT_MSG(false, "vkEndCommandBuffer failed" + ToString(vkres));
		return vkres;
	}

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	// Sychronized queue access
	{
		std::lock_guard<std::mutex> lock(mQueueMutex);

		vkres = vkQueueSubmit(m_queue->Queue, 1, &submitInfo, VK_NULL_HANDLE);
		if (vkres != VK_SUCCESS) {
			FreeCommandBuffer();
			ASSERT_MSG(false, "vkQueueSubmit failed" + ToString(vkres));
			return vkres;
		}

		vkres = vkQueueWaitIdle(m_queue->Queue);
	}

	if (vkres != VK_SUCCESS) {
		FreeCommandBuffer();
		ASSERT_MSG(false, "vkQueueWaitIdle failed" + ToString(vkres));
		return vkres;
	}

	FreeCommandBuffer();

	return VK_SUCCESS;
}

#pragma endregion

#pragma region FullscreenQuad

Result FullscreenQuad::createApiObjects(const FullscreenQuadCreateInfo& pCreateInfo)
{
	Result ppxres = ERROR_FAILED;

	// Pipeline interface
	{
		PipelineInterfaceCreateInfo createInfo = {};
		createInfo.setCount = pCreateInfo.setCount;
		for (uint32_t i = 0; i < createInfo.setCount; ++i) {
			createInfo.sets[i].set = pCreateInfo.sets[i].set;
			createInfo.sets[i].pLayout = pCreateInfo.sets[i].pLayout;
		}

		ppxres = GetDevice()->CreatePipelineInterface(createInfo, &mPipelineInterface);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "failed creating pipeline interface");
			return ppxres;
		}
	}

	// Pipeline
	{
		GraphicsPipelineCreateInfo2 createInfo = {};
		createInfo.VS = { pCreateInfo.VS, "vsmain" };
		createInfo.PS = { pCreateInfo.PS, "psmain" };
		createInfo.depthReadEnable = false;
		createInfo.depthWriteEnable = false;
		createInfo.pPipelineInterface = mPipelineInterface;
		createInfo.outputState.depthStencilFormat = pCreateInfo.depthStencilFormat;
		// Render target formats
		createInfo.outputState.renderTargetCount = pCreateInfo.renderTargetCount;
		for (uint32_t i = 0; i < createInfo.outputState.renderTargetCount; ++i) {
			createInfo.blendModes[i] = BLEND_MODE_NONE;
			createInfo.outputState.renderTargetFormats[i] = pCreateInfo.renderTargetFormats[i];
		}

		ppxres = GetDevice()->CreateGraphicsPipeline(createInfo, &mPipeline);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating graphics pipeline");
			return ppxres;
		}
	}

	return SUCCESS;
}

void FullscreenQuad::destroyApiObjects()
{
	if (mPipeline)
	{
		GetDevice()->DestroyGraphicsPipeline(mPipeline);
		mPipeline.Reset();
	}

	if (mPipelineInterface)
	{
		GetDevice()->DestroyPipelineInterface(mPipelineInterface);
		mPipelineInterface.Reset();
	}
}

#pragma endregion

#pragma region Scope

#define NULL_ARGUMENT_MSG   "unexpected null argument"
#define WRONG_OWNERSHIP_MSG "object has invalid ownership value"

ScopeDestroyer::ScopeDestroyer(RenderDevice* pDevice)
	: mDevice(pDevice)
{
	ASSERT_MSG(!IsNull(pDevice), NULL_ARGUMENT_MSG);
}

ScopeDestroyer::~ScopeDestroyer()
{
	for (auto& object : mImages) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroyImage(object);
		}
	}
	mImages.clear();

	for (auto& object : mBuffers) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroyBuffer(object);
		}
	}
	mBuffers.clear();

	for (auto& object : mMeshes) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroyMesh(object);
		}
	}
	mMeshes.clear();

	for (auto& object : mTextures) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroyTexture(object);
		}
	}
	mTextures.clear();

	for (auto& object : mSamplers) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroySampler(object);
		}
	}
	mSamplers.clear();

	for (auto& object : mSampledImageViews) {
		if (object->GetOwnership() == Ownership::Exclusive) {
			mDevice->DestroySampledImageView(object);
		}
	}
	mSampledImageViews.clear();

	for (auto& object : mTransientCommandBuffers) {
		if (object.second->GetOwnership() == Ownership::Exclusive) {
			object.first->DestroyCommandBuffer(object.second);
		}
	}
}

Result ScopeDestroyer::AddObject(Image* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mImages.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Buffer* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mBuffers.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Mesh* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mMeshes.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Texture* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mTextures.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Sampler* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mSamplers.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(SampledImageView* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mSampledImageViews.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Queue* pParent, CommandBuffer* pObject)
{
	if (IsNull(pParent) || IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != Ownership::Reference) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(Ownership::Exclusive);
	mTransientCommandBuffers.push_back(std::make_pair(pParent, pObject));
	return SUCCESS;
}

void ScopeDestroyer::ReleaseAll()
{
	mImages.clear();
	mBuffers.clear();
	mMeshes.clear();
	mTextures.clear();
	mSamplers.clear();
	mSampledImageViews.clear();
	mTransientCommandBuffers.clear();
}

#pragma endregion

#pragma region Text Draw

std::string TextureFont::GetDefaultCharacters()
{
	std::string characters;
	for (char c = 32; c < 127; ++c) {
		characters.push_back(c);
	}
	return characters;
}

Result TextureFont::createApiObjects(const TextureFontCreateInfo& pCreateInfo)
{
	std::string characters = pCreateInfo.characters;
	if (characters.empty()) {
		characters = GetDefaultCharacters();
		m_createInfo.characters = characters;
	}

	if (!utf8::is_valid(characters.cbegin(), characters.cend())) {
		return ERROR_INVALID_UTF8_STRING;
	}

	// Font metrics
	pCreateInfo.font.GetFontMetrics(pCreateInfo.size, &mFontMetrics);

	// Subpixel shift
	float subpixelShiftX = 0.5f;
	float subpixelShiftY = 0.5f;

	// Get glyph metrics and max bounds
	utf8::iterator<std::string::iterator> it(characters.begin(), characters.begin(), characters.end());
	utf8::iterator<std::string::iterator> it_end(characters.end(), characters.begin(), characters.end());
	bool                                  hasSpace = false;
	while (it != it_end) {
		const uint32_t codepoint = utf8::next(it, it_end);
		GlyphMetrics   metrics = {};
		pCreateInfo.font.GetGlyphMetrics(pCreateInfo.size, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
		mGlyphMetrics.emplace_back(TextureFontGlyphMetrics{ codepoint, metrics });

		if (!hasSpace) {
			hasSpace = (codepoint == 32);
		}
	}
	if (!hasSpace) {
		const uint32_t codepoint = 32;
		GlyphMetrics   metrics = {};
		pCreateInfo.font.GetGlyphMetrics(pCreateInfo.size, codepoint, subpixelShiftX, subpixelShiftY, &metrics);
		mGlyphMetrics.emplace_back(TextureFontGlyphMetrics{ codepoint, metrics });
	}

	// Figure out a squarish somewhat texture size
	const size_t  nc = characters.size();
	const int32_t sqrtnc = static_cast<int32_t>(sqrtf(static_cast<float>(nc)) + 0.5f) + 1;
	int32_t       bitmapWidth = 0;
	int32_t       bitmapHeight = 0;
	size_t        glyphIndex = 0;
	for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
		int32_t height = 0;
		int32_t width = 0;
		for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
			const GlyphMetrics& metrics = mGlyphMetrics[glyphIndex].glyphMetrics;
			int32_t             w = (metrics.box.x1 - metrics.box.x0) + 1;
			int32_t             h = (metrics.box.y1 - metrics.box.y0) + 1;
			width = width + w;
			height = std::max<int32_t>(height, h);
		}
		bitmapWidth = std::max<int32_t>(bitmapWidth, width);
		bitmapHeight = bitmapHeight + height;
	}

	// Storage bitmap
	Bitmap bitmap = Bitmap::Create(bitmapWidth, bitmapHeight, Bitmap::Format::FORMAT_R_UINT8);

	// Render glyph bitmaps
	const float    invBitmapWidth = 1.0f / static_cast<float>(bitmapWidth);
	const float    invBitmapHeight = 1.0f / static_cast<float>(bitmapHeight);
	const uint32_t rowStride = bitmap.GetRowStride();
	const uint32_t pixelStride = bitmap.GetPixelStride();
	uint32_t       y = 0;
	glyphIndex = 0;
	for (int32_t i = 0; (i < sqrtnc) && (glyphIndex < nc); ++i) {
		uint32_t x = 0;
		uint32_t height = 0;
		for (int32_t j = 0; (j < sqrtnc) && (glyphIndex < nc); ++j, ++glyphIndex) {
			uint32_t            codepoint = mGlyphMetrics[glyphIndex].codepoint;
			const GlyphMetrics& metrics = mGlyphMetrics[glyphIndex].glyphMetrics;
			uint32_t            w = static_cast<uint32_t>(metrics.box.x1 - metrics.box.x0) + 1;
			uint32_t            h = static_cast<uint32_t>(metrics.box.y1 - metrics.box.y0) + 1;

			uint32_t offset = (y * rowStride) + (x * pixelStride);
			char* pOutput = bitmap.GetData() + offset;
			pCreateInfo.font.RenderGlyphBitmap(pCreateInfo.size, codepoint, subpixelShiftX, subpixelShiftY, w, h, rowStride, reinterpret_cast<unsigned char*>(pOutput));

			mGlyphMetrics[glyphIndex].size.x = static_cast<float>(w);
			mGlyphMetrics[glyphIndex].size.y = static_cast<float>(h);

			mGlyphMetrics[glyphIndex].uvRect.u0 = x * invBitmapWidth;
			mGlyphMetrics[glyphIndex].uvRect.v0 = y * invBitmapHeight;
			mGlyphMetrics[glyphIndex].uvRect.u1 = (x + w - 1) * invBitmapWidth;
			mGlyphMetrics[glyphIndex].uvRect.v1 = (y + h - 1) * invBitmapHeight;

			x = x + w;
			height = std::max<int32_t>(height, h);
		}
		y += height;
	}

	Result ppxres = vkrUtil::CreateTextureFromBitmap(GetDevice()->GetGraphicsQueue(), &bitmap, &mTexture);
	if (Failed(ppxres)) {
		return ppxres;
	}

	// Release the font since we don't need it anymore
	m_createInfo.font = Font();

	return SUCCESS;
}

void TextureFont::destroyApiObjects()
{
	if (mTexture) {
		GetDevice()->DestroyTexture(mTexture);
		mTexture.Reset();
	}
}

const TextureFontGlyphMetrics* TextureFont::GetGlyphMetrics(uint32_t codepoint) const
{
	const TextureFontGlyphMetrics* ptr = nullptr;
	auto                                 it = FindIf(
		mGlyphMetrics,
		[codepoint](const TextureFontGlyphMetrics& elem) -> bool {
			bool match = (elem.codepoint == codepoint);
			return match; });
	if (it != std::end(mGlyphMetrics)) {
		ptr = &(*it);
	}
	return ptr;
}

struct Vertex
{
	float2   position;
	float2   uv;
	uint32_t rgba;
};

constexpr size_t kGlyphIndicesSize = 6 * sizeof(uint32_t);
constexpr size_t kGlyphVerticesSize = 4 * sizeof(Vertex);

static SamplerPtr sSampler;

Result TextDraw::createApiObjects(const TextDrawCreateInfo& pCreateInfo)
{
	if (IsNull(pCreateInfo.pFont))
	{
		ASSERT_MSG(false, "Pointer to texture font object is null");
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}

	// Index buffer
	{
		uint64_t size = pCreateInfo.maxTextLength * kGlyphIndicesSize;

		BufferCreateInfo createInfo = {};
		createInfo.size = size;
		createInfo.usageFlags.bits.transferSrc = true;
		createInfo.memoryUsage = MemoryUsage::CPUToGPU;
		createInfo.initialState = ResourceState::CopySrc;

		Result ppxres = GetDevice()->CreateBuffer(createInfo, &mCpuIndexBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating CPU index buffer");
			return ppxres;
		}

		createInfo.usageFlags.bits.transferSrc = false;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.usageFlags.bits.indexBuffer = true;
		createInfo.memoryUsage = MemoryUsage::GPUOnly;
		createInfo.initialState = ResourceState::IndexBuffer;

		ppxres = GetDevice()->CreateBuffer(createInfo, &mGpuIndexBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating GPU index buffer");
			return ppxres;
		}

		mIndexBufferView.pBuffer = mGpuIndexBuffer;
		mIndexBufferView.indexType = IndexType::Uint32;
		mIndexBufferView.offset = 0;
	}

	// Vertex buffer
	{
		uint64_t size = pCreateInfo.maxTextLength * kGlyphVerticesSize;

		BufferCreateInfo createInfo = {};
		createInfo.size = size;
		createInfo.usageFlags.bits.transferSrc = true;
		createInfo.memoryUsage = MemoryUsage::CPUToGPU;
		createInfo.initialState = ResourceState::CopySrc;

		Result ppxres = GetDevice()->CreateBuffer(createInfo, &mCpuVertexBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating CPU vertex buffer");
			return ppxres;
		}

		createInfo.usageFlags.bits.transferSrc = false;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.usageFlags.bits.vertexBuffer = true;
		createInfo.memoryUsage = MemoryUsage::GPUOnly;
		createInfo.initialState = ResourceState::VertexBuffer;

		ppxres = GetDevice()->CreateBuffer(createInfo, &mGpuVertexBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating GPU vertex buffer");
			return ppxres;
		}

		mVertexBufferView.pBuffer = mGpuVertexBuffer;
		mVertexBufferView.stride = sizeof(Vertex);
		mVertexBufferView.offset = 0;
	}

	if (!sSampler)
	{
		SamplerCreateInfo createInfo = {};
		createInfo.magFilter = Filter::Linear;
		createInfo.minFilter = Filter::Linear;
		createInfo.mipmapMode = SamplerMipmapMode::Linear;
		createInfo.addressModeU = SamplerAddressMode::ClampToEdge;
		createInfo.addressModeV = SamplerAddressMode::ClampToEdge;
		createInfo.addressModeW = SamplerAddressMode::ClampToEdge;
		createInfo.mipLodBias = 0.0f;
		createInfo.anisotropyEnable = false;
		createInfo.maxAnisotropy = 0.0f;
		createInfo.compareEnable = false;
		createInfo.compareOp = CompareOp::Never;
		createInfo.minLod = 0.0f;
		createInfo.maxLod = 1.0f;
		createInfo.borderColor = BorderColor::FloatTransparentBlack;

		Result ppxres = GetDevice()->CreateSampler(createInfo, &sSampler);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating sampler");
			return ppxres;
		}
	}

	// Constant buffer
	{
		uint64_t size = vkr::MINIMUM_CONSTANT_BUFFER_SIZE;

		BufferCreateInfo createInfo = {};
		createInfo.size = size;
		createInfo.usageFlags.bits.transferSrc = true;
		createInfo.memoryUsage = MemoryUsage::CPUToGPU;
		createInfo.initialState = ResourceState::CopySrc;

		Result ppxres = GetDevice()->CreateBuffer(createInfo, &mCpuConstantBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating CPU constant buffer");
			return ppxres;
		}

		createInfo.usageFlags.bits.transferSrc = false;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.usageFlags.bits.uniformBuffer = true;
		createInfo.memoryUsage = MemoryUsage::GPUOnly;
		createInfo.initialState = ResourceState::ConstantBuffer;

		ppxres = GetDevice()->CreateBuffer(createInfo, &mGpuConstantBuffer);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating GPU constant buffer");
			return ppxres;
		}
	}

	// Descriptor pool
	{
		DescriptorPoolCreateInfo createInfo = {};
		createInfo.sampler = 1;
		createInfo.sampledImage = 1;
		createInfo.uniformBuffer = 1;

		Result ppxres = GetDevice()->CreateDescriptorPool(createInfo, &mDescriptorPool);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating descriptor pool");
			return ppxres;
		}
	}

	// Descriptor set layout
	{
		std::vector<DescriptorBinding> bindings = {
			{0, DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, SHADER_STAGE_ALL_GRAPHICS},
			{1, DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, SHADER_STAGE_ALL_GRAPHICS},
			{2, DESCRIPTOR_TYPE_SAMPLER, 1, SHADER_STAGE_ALL_GRAPHICS},
		};

		DescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.bindings = bindings;

		Result ppxres = GetDevice()->CreateDescriptorSetLayout(createInfo, &mDescriptorSetLayout);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating descriptor set layout");
			return ppxres;
		}
	}

	// Descriptor Set
	{
		Result ppxres = GetDevice()->AllocateDescriptorSet(mDescriptorPool, mDescriptorSetLayout, &mDescriptorSet);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed allocating descriptor set");
			return ppxres;
		}

		mDescriptorSet->UpdateUniformBuffer(0, 0, mGpuConstantBuffer);
		mDescriptorSet->UpdateSampledImage(1, 0, pCreateInfo.pFont->GetTexture());
		mDescriptorSet->UpdateSampler(2, 0, sSampler);
	}

	// Pipeline interface
	{
		PipelineInterfaceCreateInfo createInfo = {};
		createInfo.setCount = 1;
		createInfo.sets[0].set = 0;
		createInfo.sets[0].pLayout = mDescriptorSetLayout;

		Result ppxres = GetDevice()->CreatePipelineInterface(createInfo, &mPipelineInterface);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating pipeline interface");
			return ppxres;
		}
	}

	// Pipeline
	{
		VertexBinding vertexBinding;
		vertexBinding.AppendAttribute({ "POSITION", 0, Format::R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });
		vertexBinding.AppendAttribute({ "TEXCOORD", 1, Format::R32G32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });
		vertexBinding.AppendAttribute({ "COLOR", 2, Format::R8G8B8A8_UNORM, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });

		GraphicsPipelineCreateInfo2 createInfo = {};
		createInfo.VS = { pCreateInfo.VS.pModule, pCreateInfo.VS.entryPoint };
		createInfo.PS = { pCreateInfo.PS.pModule, pCreateInfo.PS.entryPoint };
		createInfo.vertexInputState.bindingCount = 1;
		createInfo.vertexInputState.bindings[0] = vertexBinding;
		createInfo.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		createInfo.polygonMode = POLYGON_MODE_FILL;
		createInfo.cullMode = CULL_MODE_BACK;
		createInfo.frontFace = FRONT_FACE_CCW;
		createInfo.depthReadEnable = false;
		createInfo.depthWriteEnable = false;
		createInfo.blendModes[0] = pCreateInfo.blendMode;
		createInfo.outputState.renderTargetCount = 1;
		createInfo.outputState.renderTargetFormats[0] = pCreateInfo.renderTargetFormat;
		createInfo.outputState.depthStencilFormat = pCreateInfo.depthStencilFormat;
		createInfo.pPipelineInterface = mPipelineInterface;

		Result ppxres = GetDevice()->CreateGraphicsPipeline(createInfo, &mPipeline);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "failed creating pipeline");
			return ppxres;
		}
	}

	return SUCCESS;
}

void TextDraw::destroyApiObjects()
{
	if (mCpuIndexBuffer) {
		GetDevice()->DestroyBuffer(mCpuIndexBuffer);
		mCpuIndexBuffer.Reset();
	}

	if (mGpuIndexBuffer) {
		GetDevice()->DestroyBuffer(mGpuIndexBuffer);
		mGpuIndexBuffer.Reset();
	}

	if (mCpuVertexBuffer) {
		GetDevice()->DestroyBuffer(mCpuVertexBuffer);
		mCpuVertexBuffer.Reset();
	}

	if (mGpuVertexBuffer) {
		GetDevice()->DestroyBuffer(mGpuVertexBuffer);
		mGpuVertexBuffer.Reset();
	}

	if (mCpuConstantBuffer) {
		GetDevice()->DestroyBuffer(mCpuConstantBuffer);
		mCpuConstantBuffer.Reset();
	}

	if (mGpuConstantBuffer) {
		GetDevice()->DestroyBuffer(mGpuConstantBuffer);
		mGpuConstantBuffer.Reset();
	}

	if (mPipeline) {
		GetDevice()->DestroyGraphicsPipeline(mPipeline);
		mPipeline.Reset();
	}

	if (mPipelineInterface) {
		GetDevice()->DestroyPipelineInterface(mPipelineInterface);
		mPipelineInterface.Reset();
	}

	if (mDescriptorSet) {
		GetDevice()->FreeDescriptorSet(mDescriptorSet);
		mDescriptorSet.Reset();
	}

	if (mDescriptorSetLayout) {
		GetDevice()->DestroyDescriptorSetLayout(mDescriptorSetLayout);
		mDescriptorSetLayout.Reset();
	}

	if (mDescriptorPool) {
		GetDevice()->DestroyDescriptorPool(mDescriptorPool);
		mDescriptorPool.Reset();
	}
}

void TextDraw::Clear()
{
	mTextLength = 0;
}

void TextDraw::AddString(
	const float2& position,
	const std::string& string,
	float              tabSpacing,
	float              lineSpacing,
	const float3& color,
	float              opacity)
{
	if (mTextLength >= m_createInfo.maxTextLength) {
		return;
	}

	// Map index buffer
	void* mappedAddress = nullptr;
	Result ppxres = mCpuIndexBuffer->MapMemory(0, &mappedAddress);
	if (Failed(ppxres)) {
		return;
	}
	uint8_t* pIndicesBaseAddr = static_cast<uint8_t*>(mappedAddress);

	// Map vertex buffer
	ppxres = mCpuVertexBuffer->MapMemory(0, &mappedAddress);
	if (Failed(ppxres)) {
		return;
	}
	uint8_t* pVerticesBaseAddr = static_cast<uint8_t*>(mappedAddress);

	// Convert to 8 bit color
	uint32_t r = std::min<uint32_t>(static_cast<uint32_t>(color.r * 255.0f), 255);
	uint32_t g = std::min<uint32_t>(static_cast<uint32_t>(color.g * 255.0f), 255);
	uint32_t b = std::min<uint32_t>(static_cast<uint32_t>(color.b * 255.0f), 255);
	uint32_t a = std::min<uint32_t>(static_cast<uint32_t>(opacity * 255.0f), 255);
	uint32_t rgba = (a << 24) | (b << 16) | (g << 8) | (r << 0);

	utf8::iterator<std::string::const_iterator> it(string.begin(), string.begin(), string.end());
	utf8::iterator<std::string::const_iterator> it_end(string.end(), string.begin(), string.end());
	float2                                      baseline = position;
	float                                       ascent = m_createInfo.pFont->GetAscent();
	float                                       descent = m_createInfo.pFont->GetDescent();
	float                                       lineGap = m_createInfo.pFont->GetLineGap();
	lineSpacing = lineSpacing * (ascent - descent + lineGap);

	while (it != it_end) {
		uint32_t codepoint = utf8::next(it, it_end);
		if (codepoint == '\n') {
			baseline.x = position.x;
			baseline.y += lineSpacing;
			continue;
		}
		else if (codepoint == '\t') {
			const TextureFontGlyphMetrics* pMetrics = m_createInfo.pFont->GetGlyphMetrics(32);
			baseline.x += tabSpacing * pMetrics->glyphMetrics.advance;
			continue;
		}

		const TextureFontGlyphMetrics* pMetrics = m_createInfo.pFont->GetGlyphMetrics(codepoint);
		if (IsNull(pMetrics)) {
			pMetrics = m_createInfo.pFont->GetGlyphMetrics(32);
		}

		size_t indexBufferOffset = mTextLength * kGlyphIndicesSize;
		size_t vertexBufferOffset = mTextLength * kGlyphVerticesSize;
		bool   exceededIndexBuffer = (indexBufferOffset >= mCpuIndexBuffer->GetSize());
		bool   exceededVertexBuffer = (vertexBufferOffset >= mCpuVertexBuffer->GetSize());
		if (exceededIndexBuffer || exceededVertexBuffer) {
			break;
		}

		uint32_t* pIndices = reinterpret_cast<uint32_t*>(pIndicesBaseAddr + indexBufferOffset);
		Vertex* pVertices = reinterpret_cast<Vertex*>(pVerticesBaseAddr + vertexBufferOffset);

		float2 P = baseline + float2(pMetrics->glyphMetrics.box.x0, pMetrics->glyphMetrics.box.y0);
		float2 P0 = P;
		float2 P1 = P + float2(0, pMetrics->size.y);
		float2 P2 = P + pMetrics->size;
		float2 P3 = P + float2(pMetrics->size.x, 0);
		float2 uv0 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v0);
		float2 uv1 = float2(pMetrics->uvRect.u0, pMetrics->uvRect.v1);
		float2 uv2 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v1);
		float2 uv3 = float2(pMetrics->uvRect.u1, pMetrics->uvRect.v0);

		pVertices[0] = Vertex{ P0, uv0, rgba };
		pVertices[1] = Vertex{ P1, uv1, rgba };
		pVertices[2] = Vertex{ P2, uv2, rgba };
		pVertices[3] = Vertex{ P3, uv3, rgba };

		uint32_t vertexCount = mTextLength * 4;
		pIndices[0] = vertexCount + 0;
		pIndices[1] = vertexCount + 1;
		pIndices[2] = vertexCount + 2;
		pIndices[3] = vertexCount + 0;
		pIndices[4] = vertexCount + 2;
		pIndices[5] = vertexCount + 3;

		mTextLength += 1;
		baseline.x += pMetrics->glyphMetrics.advance;
	}

	mCpuIndexBuffer->UnmapMemory();
	mCpuVertexBuffer->UnmapMemory();
}

void TextDraw::AddString(
	const float2& position,
	const std::string& string,
	const float3& color,
	float              opacity)
{
	AddString(position, string, 3.0f, 1.0f, color, opacity);
}

Result TextDraw::UploadToGpu(Queue* pQueue)
{
	BufferToBufferCopyInfo copyInfo = {};
	copyInfo.size = mCpuIndexBuffer->GetSize();
	copyInfo.srcBuffer.offset = 0;
	copyInfo.dstBuffer.offset = 0;

	Result ppxres = pQueue->CopyBufferToBuffer(&copyInfo, mCpuIndexBuffer, mGpuIndexBuffer, ResourceState::IndexBuffer, ResourceState::IndexBuffer);
	if (Failed(ppxres)) {
		return ppxres;
	}

	copyInfo.size = mCpuVertexBuffer->GetSize();
	ppxres = pQueue->CopyBufferToBuffer(&copyInfo, mCpuVertexBuffer, mGpuVertexBuffer, ResourceState::VertexBuffer, ResourceState::VertexBuffer);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

void TextDraw::UploadToGpu(CommandBuffer* pCommandBuffer)
{
	BufferToBufferCopyInfo copyInfo = {};
	copyInfo.size = mTextLength * kGlyphIndicesSize;
	copyInfo.srcBuffer.offset = 0;
	copyInfo.dstBuffer.offset = 0;

	pCommandBuffer->BufferResourceBarrier(mGpuIndexBuffer, ResourceState::IndexBuffer, ResourceState::CopyDst);
	pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuIndexBuffer, mGpuIndexBuffer);
	pCommandBuffer->BufferResourceBarrier(mGpuIndexBuffer, ResourceState::CopyDst, ResourceState::IndexBuffer);

	copyInfo.size = mTextLength * kGlyphVerticesSize;
	pCommandBuffer->BufferResourceBarrier(mGpuVertexBuffer, ResourceState::VertexBuffer, ResourceState::CopyDst);
	pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuVertexBuffer, mGpuVertexBuffer);
	pCommandBuffer->BufferResourceBarrier(mGpuVertexBuffer, ResourceState::CopyDst, ResourceState::VertexBuffer);
}

void TextDraw::PrepareDraw(const float4x4& MVP, CommandBuffer* pCommandBuffer)
{
	void* mappedAddress = nullptr;
	Result ppxres = mCpuConstantBuffer->MapMemory(0, &mappedAddress);
	if (Failed(ppxres)) {
		return;
	}

	std::memcpy(mappedAddress, &MVP, sizeof(float4x4));

	mCpuConstantBuffer->UnmapMemory();

	BufferToBufferCopyInfo copyInfo = {};
	copyInfo.size = mCpuConstantBuffer->GetSize();
	copyInfo.srcBuffer.offset = 0;
	copyInfo.dstBuffer.offset = 0;

	pCommandBuffer->BufferResourceBarrier(mGpuConstantBuffer, ResourceState::ConstantBuffer, ResourceState::CopyDst);
	pCommandBuffer->CopyBufferToBuffer(&copyInfo, mCpuConstantBuffer, mGpuConstantBuffer);
	pCommandBuffer->BufferResourceBarrier(mGpuConstantBuffer, ResourceState::CopyDst, ResourceState::ConstantBuffer);
}

void TextDraw::Draw(CommandBuffer* pCommandBuffer)
{
	pCommandBuffer->BindIndexBuffer(&mIndexBufferView);
	pCommandBuffer->BindVertexBuffers(1, &mVertexBufferView);
	pCommandBuffer->BindGraphicsDescriptorSets(mPipelineInterface, 1, &mDescriptorSet);
	pCommandBuffer->BindGraphicsPipeline(mPipeline);
	pCommandBuffer->DrawIndexed(mTextLength * 6);
}

#pragma endregion

} // namespace vkr