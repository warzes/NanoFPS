#include "stdafx.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderDevice.h"

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

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
		ASSERT_MSG(false, "vkCreateFence failed: " + ToString(vkres));
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

Result Semaphore::Wait(uint64_t value, uint64_t timeout) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	auto ppxres = timelineWait(value, timeout);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Semaphore::Signal(uint64_t value) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
		return ERROR_GRFX_INVALID_SEMAPHORE_TYPE;
	}

	auto ppxres = timelineSignal(value);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

uint64_t Semaphore::GetCounterValue() const
{
	if (this->GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		ASSERT_MSG(false, REQUIRES_TIMELINE_MSG);
		return UINT64_MAX;
	}

	uint64_t value = timelineCounterValue();
	return value;
}

Result Semaphore::createApiObjects(const SemaphoreCreateInfo& createInfo)
{
	VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	timelineCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue = createInfo.initialValue;

	VkSemaphoreCreateInfo vkci = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	vkci.pNext = (createInfo.semaphoreType == SEMAPHORE_TYPE_TIMELINE) ? &timelineCreateInfo : nullptr;

	VkResult vkres = vkCreateSemaphore(GetDevice()->GetVkDevice(), &vkci, nullptr, &m_semaphore);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateSemaphore failed: " + ToString(vkres));
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

#pragma region Buffer

Result Buffer::MapMemory(uint64_t offset, void** ppMappedAddress)
{
	if (IsNull(ppMappedAddress)) return ERROR_UNEXPECTED_NULL_ARGUMENT;

	VkResult vkres = vmaMapMemory(GetDevice()->GetVmaAllocator(), m_allocation, ppMappedAddress);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vmaMapMemory failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	return SUCCESS;
}

void Buffer::UnmapMemory()
{
	vmaUnmapMemory(GetDevice()->GetVmaAllocator(), m_allocation);
}

Result Buffer::CopyFromSource(uint32_t dataSize, const void* pSrcData)
{
	if (dataSize > GetSize()) return ERROR_LIMIT_EXCEEDED;

	// Map
	void* pBufferAddress = nullptr;
	Result ppxres = MapMemory(0, &pBufferAddress);
	if (Failed(ppxres)) return ppxres;

	// Copy
	std::memcpy(pBufferAddress, pSrcData, dataSize);

	// Unmap
	UnmapMemory();

	return SUCCESS;
}

Result Buffer::CopyToDest(uint32_t dataSize, void* pDestData)
{
	if (dataSize > GetSize()) return ERROR_LIMIT_EXCEEDED;

	// Map
	void* pBufferAddress = nullptr;
	Result ppxres = MapMemory(0, &pBufferAddress);
	if (Failed(ppxres))  return ppxres;

	// Copy
	std::memcpy(pDestData, pBufferAddress, dataSize);

	// Unmap
	UnmapMemory();

	return SUCCESS;
}

Result Buffer::create(const BufferCreateInfo& createInfo)
{
#ifndef DISABLE_MINIMUM_BUFFER_SIZE_CHECK
	// Constant/uniform buffers need to be at least PPX_CONSTANT_BUFFER_ALIGNMENT in size
	if (createInfo.usageFlags.bits.uniformBuffer && (createInfo.size < CONSTANT_BUFFER_ALIGNMENT))
	{
		ASSERT_MSG(false, "constant/uniform buffer sizes must be at least PPX_CONSTANT_BUFFER_ALIGNMENT (" + std::to_string(CONSTANT_BUFFER_ALIGNMENT) + ")");
		return ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
	}

	// Storage/structured buffers need to be at least STORAGE_BUFFER_ALIGNMENT in size
	if (createInfo.usageFlags.bits.uniformBuffer && (createInfo.size < STUCTURED_BUFFER_ALIGNMENT))
	{
		ASSERT_MSG(false, "storage/structured buffer sizes must be at least PPX_STUCTURED_BUFFER_ALIGNMENT (" + std::to_string(STUCTURED_BUFFER_ALIGNMENT) + ")");
		return ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET;
	}
#endif
	Result ppxres = DeviceObject<BufferCreateInfo>::create(createInfo);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
}

Result Buffer::createApiObjects(const BufferCreateInfo& CreateInfo)
{
	VkDeviceSize alignedSize = static_cast<VkDeviceSize>(CreateInfo.size);
	if (CreateInfo.usageFlags.bits.uniformBuffer)
		alignedSize = RoundUp<VkDeviceSize>(CreateInfo.size, UNIFORM_BUFFER_ALIGNMENT);

	VkBufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size               = alignedSize;
	createInfo.usage              = ToVkBufferUsageFlags(CreateInfo.usageFlags);
	createInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

	VkAllocationCallbacks* pAllocator = nullptr;
	VkResult vkres = vkCreateBuffer(GetDevice()->GetVkDevice(), &createInfo, pAllocator, &m_buffer);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkCreateBuffer failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}

	// Allocate memory
	{
		VmaMemoryUsage memoryUsage = ToVmaMemoryUsage(CreateInfo.memoryUsage);
		if (memoryUsage == VMA_MEMORY_USAGE_UNKNOWN)
		{
			ASSERT_MSG(false, "unknown memory usage");
			return ERROR_API_FAILURE;
		}

		VmaAllocationCreateFlags createFlags = 0;
		if ((memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY) || (memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY))
		{
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

		VkResult vkres = vmaAllocateMemoryForBuffer(
			GetDevice()->GetVmaAllocator(),
			m_buffer,
			&vma_alloc_ci,
			&m_allocation,
			&m_allocationInfo);
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vmaAllocateMemoryForBuffer failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	// Bind memory
	{
		VkResult vkres = vmaBindBufferMemory(
			GetDevice()->GetVmaAllocator(),
			m_allocation,
			m_buffer);
		if (vkres != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vmaBindBufferMemory failed: " + ToString(vkres));
			return ERROR_API_FAILURE;
		}
	}

	return SUCCESS;
}

void Buffer::destroyApiObjects()
{
	if (m_allocation) {
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
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.memoryUsage = memoryUsage;
	ci.initialState = RESOURCE_STATE_SHADER_RESOURCE;
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
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.depthStencilAttachment = true;
	ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ci.initialState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
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
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.colorAttachment = true;
	ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ci.pApiObject = nullptr;
	return ci;
}

Result Image::create(const ImageCreateInfo& pCreateInfo)
{
	if ((pCreateInfo.type == IMAGE_TYPE_CUBE) && (pCreateInfo.arrayLayerCount != 6))
	{
		ASSERT_MSG(false, "arrayLayerCount must be 6 if type is IMAGE_TYPE_CUBE");
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
		case IMAGE_TYPE_1D: return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_1D_ARRAY : IMAGE_VIEW_TYPE_1D; break;
		case IMAGE_TYPE_2D: return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_2D_ARRAY : IMAGE_VIEW_TYPE_2D; break;
		case IMAGE_TYPE_CUBE: return (arrayLayerCount > 6) ? IMAGE_VIEW_TYPE_CUBE_ARRAY : IMAGE_VIEW_TYPE_CUBE; break;
		}
	}

	return IMAGE_VIEW_TYPE_UNDEFINED;
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
	ci.ownership = OWNERSHIP_REFERENCE;
	return ci;
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
	ci.ownership = OWNERSHIP_REFERENCE;
	return ci;
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
	ci.ownership = OWNERSHIP_REFERENCE;
	return ci;
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
	ci.ownership = OWNERSHIP_REFERENCE;
	return ci;
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

	// Yes, mCreateInfo will self overwrite in the following function call.
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

		Result ppxres = GetDevice()->CreateImage(&ci, &m_image);
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

		Result ppxres = GetDevice()->CreateSampledImageView(&ci, &m_sampledImageView);
		if (Failed(ppxres)) 
		{
			ASSERT_MSG(false, "texture create sampled image view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.colorAttachment)
	{
		RenderTargetViewCreateInfo ci = RenderTargetViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.renderTargetViewFormat != FORMAT_UNDEFINED)
		{
			ci.format = pCreateInfo.renderTargetViewFormat;
		}

		Result ppxres = GetDevice()->CreateRenderTargetView(&ci, &m_renderTargetView);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create render target view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.depthStencilAttachment)
	{
		DepthStencilViewCreateInfo ci = DepthStencilViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.depthStencilViewFormat != FORMAT_UNDEFINED)
		{
			ci.format = pCreateInfo.depthStencilViewFormat;
		}

		Result ppxres = GetDevice()->CreateDepthStencilView(&ci, &m_depthStencilView);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "texture create depth stencil view failed");
			return ppxres;
		}
	}

	if (pCreateInfo.usageFlags.bits.storage)
	{
		StorageImageViewCreateInfo ci = StorageImageViewCreateInfo::GuessFromImage(m_image);
		if (pCreateInfo.storageImageViewFormat != FORMAT_UNDEFINED)
		{
			ci.format = pCreateInfo.storageImageViewFormat;
		}

		Result ppxres = GetDevice()->CreateStorageImageView(&ci, &m_storageImageView);
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
	if (m_sampledImageView && (m_sampledImageView->GetOwnership() != OWNERSHIP_REFERENCE))
	{
		GetDevice()->DestroySampledImageView(m_sampledImageView);
		mSampledImageView.Reset();
	}

	if (m_renderTargetView && (m_renderTargetView->GetOwnership() != OWNERSHIP_REFERENCE))
	{
		GetDevice()->DestroyRenderTargetView(m_renderTargetView);
		m_renderTargetView.Reset();
	}

	if (m_depthStencilView && (m_depthStencilView->GetOwnership() != OWNERSHIP_REFERENCE))
	{
		GetDevice()->DestroyDepthStencilView(m_depthStencilView);
		m_depthStencilView.Reset();
	}

	if (m_storageImageView && (m_storageImageView->GetOwnership() != OWNERSHIP_REFERENCE))
	{
		GetDevice()->DestroyStorageImageView(m_storageImageView);
		m_storageImageView.Reset();
	}

	if (m_image && (m_image->GetOwnership() != OWNERSHIP_REFERENCE))
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
	return m_sampledImageView ? m_sampledImageView->GetFormat() : FORMAT_UNDEFINED;
}

Format Texture::GetRenderTargetViewFormat() const
{
	return m_renderTargetView ? m_renderTargetView->GetFormat() : FORMAT_UNDEFINED;
}

Format Texture::GetDepthStencilViewFormat() const
{
	return m_depthStencilView ? m_depthStencilView->GetFormat() : FORMAT_UNDEFINED;
}

Format Texture::GetStorageImageViewFormat() const
{
	return m_storageImageView ? m_storageImageView->GetFormat() : FORMAT_UNDEFINED;
}

#pragma endregion

//===================================================================
// OLD
//===================================================================
#pragma region VulkanFence

VulkanFence::VulkanFence(RenderDevice& device, const FenceCreateInfo& createInfo)
	: m_device(device)
{
	VkFenceCreateInfo fenceInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags             = createInfo.signaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

	VkResult result = vkCreateFence(m_device.GetVkDevice(), &fenceInfo, nullptr, &m_fence);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create fence objects:" + std::string(string_VkResult(result)));
		m_fence = nullptr;
	}
}

VulkanFence::~VulkanFence()
{
	if (m_fence)
		vkDestroyFence(m_device.GetVkDevice(), m_fence, nullptr);
}

bool VulkanFence::Wait(uint64_t timeout)
{
	if (!IsValid()) return false;

	VkResult result = vkWaitForFences(m_device.GetVkDevice(), 1, &m_fence, VK_TRUE, timeout);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to wait fence objects:" + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

bool VulkanFence::Reset()
{
	if (!IsValid()) return false;

	VkResult result = vkResetFences(m_device.GetVkDevice(), 1, &m_fence);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to reset fence objects:" + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

bool VulkanFence::WaitAndReset(uint64_t timeout)
{
	if (!Wait(timeout)) return false;
	if (!Reset()) return false;
	return true;
}

VkResult VulkanFence::Status() const
{
	return vkGetFenceStatus(m_device.GetVkDevice(), m_fence);
}

#pragma endregion

#pragma region VulkanSemaphore

#define REQUIRES_TIMELINE_MSG "invalid semaphore type: operation requires timeline semaphore"

VulkanSemaphore::VulkanSemaphore(RenderDevice& device, const SemaphoreCreateInfo& createInfo)
	: m_device(device)
{
	VkSemaphoreTypeCreateInfo timelineCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO };
	timelineCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	timelineCreateInfo.initialValue              = createInfo.initialValue;

	VkSemaphoreCreateInfo semaphoreInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	semaphoreInfo.pNext = (createInfo.semaphoreType == SEMAPHORE_TYPE_TIMELINE) ? &timelineCreateInfo : nullptr;

	VkResult result = vkCreateSemaphore(m_device.GetVkDevice(), &semaphoreInfo, nullptr, &m_semaphore);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create semaphore objects:" + std::string(string_VkResult(result)));
		m_semaphore = nullptr;
	}
}

VulkanSemaphore::~VulkanSemaphore()
{
	if (m_semaphore)
		vkDestroySemaphore(m_device.GetVkDevice(), m_semaphore, nullptr);
}

uint64_t VulkanSemaphore::GetCounterValue() const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return UINT64_MAX;
	}

	return timelineCounterValue();
}

bool VulkanSemaphore::Wait(uint64_t value, uint64_t timeout) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return false;
	}
	return timelineWait(value, timeout);
}

bool VulkanSemaphore::Signal(uint64_t value) const
{
	if (GetSemaphoreType() != SEMAPHORE_TYPE_TIMELINE)
	{
		assert(false && REQUIRES_TIMELINE_MSG);
		return false;
	}
	return timelineSignal(value);
}

bool VulkanSemaphore::timelineWait(uint64_t value, uint64_t timeout) const
{
	VkSemaphoreWaitInfo waitInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
	waitInfo.semaphoreCount      = 1;
	waitInfo.pSemaphores         = &m_semaphore;
	waitInfo.pValues             = &value;

	VkResult result = vkWaitSemaphores(m_device.GetVkDevice(), &waitInfo, timeout);
	if (result != VK_SUCCESS) return false;

	return true;
}

bool VulkanSemaphore::timelineSignal(uint64_t value) const
{
	// See header for explanation
	if (value > m_timelineSignaledValue)
	{
		VkSemaphoreSignalInfo signalInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO };
		signalInfo.semaphore             = m_semaphore;
		signalInfo.value                 = value;

		VkResult result = vkSignalSemaphore(m_device.GetVkDevice(), &signalInfo);
		if (result != VK_SUCCESS) return false;

		m_timelineSignaledValue = value;
	}

	return true;
}

uint64_t VulkanSemaphore::timelineCounterValue() const
{
	uint64_t value = UINT64_MAX;
	VkResult result = vkGetSemaphoreCounterValue(m_device.GetVkDevice(), m_semaphore, &value);
	// Not clear if value is written to upon failure so prefer safety.
	return (result == VK_SUCCESS) ? value : UINT64_MAX;
}

#pragma endregion

#pragma region VulkanImage

ImageCreateInfo ImageCreateInfo::SampledImage2D(
	uint32_t    width,
	uint32_t    height,
	Format      format,
	SampleCount sampleCount,
	MemoryUsage memoryUsage)
{
	ImageCreateInfo ci = {};
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.memoryUsage = memoryUsage;
	ci.initialState = RESOURCE_STATE_SHADER_RESOURCE;
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
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.depthStencilAttachment = true;
	ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ci.initialState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
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
	ci.type = IMAGE_TYPE_2D;
	ci.width = width;
	ci.height = height;
	ci.depth = 1;
	ci.format = format;
	ci.sampleCount = sampleCount;
	ci.mipLevelCount = 1;
	ci.arrayLayerCount = 1;
	ci.usageFlags.bits.sampled = true;
	ci.usageFlags.bits.colorAttachment = true;
	ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
	ci.pApiObject = nullptr;
	return ci;
}

VulkanImage::VulkanImage(RenderDevice& device, const ImageCreateInfo& createInfo)
	: m_device(device)
{
	if ((createInfo.type == IMAGE_TYPE_CUBE) && (createInfo.arrayLayerCount != 6))
	{
		Fatal("arrayLayerCount must be 6 if type is IMAGE_TYPE_CUBE");
		return;
	}

	m_type = createInfo.type;
	m_width = createInfo.width;
	m_height = createInfo.height;
	m_depth = createInfo.depth;
	m_format = createInfo.format;
	m_sampleCount = createInfo.sampleCount;
	m_mipLevelCount = createInfo.mipLevelCount;
	m_arrayLayerCount = createInfo.arrayLayerCount;
	m_usageFlags = createInfo.usageFlags;
	m_memoryUsage = createInfo.memoryUsage;
	m_initialState = createInfo.initialState;
	m_RTVClearValue = createInfo.RTVClearValue;
	m_DSVClearValue = createInfo.DSVClearValue;
	m_pApiObject = createInfo.pApiObject;
	m_concurrentMultiQueueUsage = createInfo.concurrentMultiQueueUsage;
	m_createFlags = createInfo.createFlags;

	//if (!m_pApiObject)
	//{
	//	// Create image
	//	{
	//		VkExtent3D extent = {};
	//		extent.width = m_width;
	//		extent.height = m_height;
	//		extent.depth = m_depth;

	//		VkImageCreateFlags createFlags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	//		if (m_type == IMAGE_TYPE_CUBE)
	//			createFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

	//		if (m_createFlags.bits.subsampledFormat)
	//			createFlags |= VK_IMAGE_CREATE_SUBSAMPLED_BIT_EXT;

	//		auto queueIndices = m_device.GetAllQueueFamilyIndices();

	//		VkImageCreateInfo vkci = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	//		vkci.flags = createFlags;
	//		vkci.imageType = ToVkImageType(m_type);
	//		vkci.format = ToVkFormat(m_format);
	//		vkci.extent = extent;
	//		vkci.mipLevels = m_mipLevelCount;
	//		vkci.arrayLayers = m_arrayLayerCount;
	//		vkci.samples = ToVkSampleCount(m_sampleCount);
	//		vkci.tiling = m_memoryUsage == MEMORY_USAGE_GPU_TO_CPU ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
	//		vkci.usage = ToVkImageUsageFlags(m_usageFlags);
	//		vkci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//		if (m_concurrentMultiQueueUsage)
	//		{
	//			vkci.sharingMode = VK_SHARING_MODE_CONCURRENT;
	//			vkci.queueFamilyIndexCount = 3;
	//			vkci.pQueueFamilyIndices = queueIndices.data();
	//		}
	//		else
	//		{
	//			vkci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	//			vkci.queueFamilyIndexCount = 0;
	//			vkci.pQueueFamilyIndices = nullptr;
	//		}

	//		VkAllocationCallbacks* pAllocator = nullptr;

	//		VkResult result = vk::CreateImage(ToApi(GetDevice())->GetVkDevice(), &vkci, pAllocator, &m_image);
	//		if (result != VK_SUCCESS)
	//		{
	//			Fatal("vkCreateImage failed: " + std::string(string_VkResult(result)));
	//			return;
	//		}
	//	}

	//	// Allocate memory
	//	{
	//		VmaMemoryUsage memoryUsage = ToVmaMemoryUsage(m_memoryUsage);
	//		if (memoryUsage == VMA_MEMORY_USAGE_UNKNOWN)
	//		{
	//			Fatal("unknown memory usage");
	//			return;
	//		}

	//		VmaAllocationCreateFlags createFlags = 0;

	//		if ((memoryUsage == VMA_MEMORY_USAGE_CPU_ONLY) || (memoryUsage == VMA_MEMORY_USAGE_CPU_TO_GPU))
	//		{
	//			createFlags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	//		}

	//		VmaAllocationCreateInfo vma_alloc_ci = {};
	//		vma_alloc_ci.flags = createFlags;
	//		vma_alloc_ci.usage = memoryUsage;
	//		vma_alloc_ci.requiredFlags = 0;
	//		vma_alloc_ci.preferredFlags = 0;
	//		vma_alloc_ci.memoryTypeBits = 0;
	//		vma_alloc_ci.pool = VK_NULL_HANDLE;
	//		vma_alloc_ci.pUserData = nullptr;

	//		VkResult result = vmaAllocateMemoryForImage(
	//			ToApi(GetDevice())->GetVmaAllocator(),
	//			mImage,
	//			&vma_alloc_ci,
	//			&mAllocation,
	//			&mAllocationInfo);
	//		if (vresultkres != VK_SUCCESS)
	//		{
	//			Fatal("vmaAllocateMemoryForImage failed: " + std::string(string_VkResult(result)));
	//			return;
	//		}
	//	}

	//	// Bind memory
	//	{
	//		VkResult result = vmaBindImageMemory(
	//			ToApi(GetDevice())->GetVmaAllocator(),
	//			mAllocation,
	//			mImage);
	//		if (result != VK_SUCCESS) {
	//			Fatal("vmaBindImageMemory failed: " + std::string(string_VkResult(result)));
	//			return;
	//		}
	//	}
	//}
	//else
	//{
	//	m_image = reinterpret_cast<VkImage>(m_pApiObject);
	//}

	//m_vkFormat = ToVkFormat(m_format);
	//m_imageAspect = DetermineAspectMask(m_vkFormat);

	//if ((m_initialState != RESOURCE_STATE_UNDEFINED) && !m_pApiObject)
	//{
	//	QueuePtr grfxQueue = GetDevice()->GetAnyAvailableQueue();
	//	if (!grfxQueue)
	//		return;

	//	// Determine pipeline stage and layout from the initial state
	//	VkPipelineStageFlags pipelineStage = 0;
	//	VkAccessFlags        accessMask = 0;
	//	VkImageLayout        layout = VK_IMAGE_LAYOUT_UNDEFINED;
	//	Result               ppxres = ToVkBarrierDst(
	//		pCreateInfo->initialState,
	//		grfxQueue->GetCommandType(),
	//		pDevice->GetDeviceFeatures(),
	//		pipelineStage,
	//		accessMask,
	//		layout);
	//	if (Failed(ppxres)) {
	//		PPX_ASSERT_MSG(false, "couldn't determine pipeline stage and layout from initial state");
	//		return ppxres;
	//	}

	//	vk::Queue* pQueue = ToApi(grfxQueue.Get());

	//	VkResult vkres = pQueue->TransitionImageLayout(
	//		mImage,                       // image
	//		mImageAspect,                 // aspectMask
	//		0,                            // baseMipLevel
	//		pCreateInfo->mipLevelCount,   // levelCount
	//		0,                            // baseArrayLayer
	//		pCreateInfo->arrayLayerCount, // layerCount
	//		VK_IMAGE_LAYOUT_UNDEFINED,    // oldLayout
	//		layout,                       // newLayout
	//		pipelineStage);               // newPipelineStage)
	//	if (vkres != VK_SUCCESS)
	//	{
	//		Fatal("Queue::TransitionImageLayout failed: " << ToString(vkres));
	//		return ppx::ERROR_API_FAILURE;
	//	}
	//}
}

VulkanImage::~VulkanImage()
{
	//// Don't destroy image unless we created it
	//if (m_pApiObject)
	//	return;

	//if (m_allocation)
	//{
	//	vmaFreeMemory(ToApi(GetDevice())->GetVmaAllocator(), m_allocation);
	//	m_allocationInfo = {};
	//}

	//if (m_image)
	//{
	//	vkDestroyImage(ToApi(GetDevice())->GetVkDevice(), m_image, nullptr);
	//}
}

ImageViewType VulkanImage::GuessImageViewType(bool isCube) const
{
	const uint32_t arrayLayerCount = GetArrayLayerCount();

	if (isCube)
	{
		return (arrayLayerCount > 0) ? IMAGE_VIEW_TYPE_CUBE_ARRAY : IMAGE_VIEW_TYPE_CUBE;
	}
	else
	{
		switch (m_type)
		{
		default: break;
		case IMAGE_TYPE_1D:   return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_1D_ARRAY :   IMAGE_VIEW_TYPE_1D; break;
		case IMAGE_TYPE_2D:   return (arrayLayerCount > 1) ? IMAGE_VIEW_TYPE_2D_ARRAY :   IMAGE_VIEW_TYPE_2D; break;
		case IMAGE_TYPE_CUBE: return (arrayLayerCount > 6) ? IMAGE_VIEW_TYPE_CUBE_ARRAY : IMAGE_VIEW_TYPE_CUBE; break;
		}
	}

	return IMAGE_VIEW_TYPE_UNDEFINED;
}

bool VulkanImage::MapMemory(uint64_t offset, void** ppMappedAddress)
{
	if (ppMappedAddress) return false;

	//VkResult vkres = vmaMapMemory(
	//	ToApi(GetDevice())->GetVmaAllocator(),
	//	mAllocation,
	//	ppMappedAddress);
	//if (vkres != VK_SUCCESS)
	//{
	//	PPX_ASSERT_MSG(false, "vmaMapMemory failed: " << ToString(vkres));
	//	return ppx::ERROR_API_FAILURE;
	//}

	return true;
}

void VulkanImage::UnmapMemory()
{
	//vmaUnmapMemory(ToApi(GetDevice())->GetVmaAllocator(), m_allocation);
}

#pragma endregion
