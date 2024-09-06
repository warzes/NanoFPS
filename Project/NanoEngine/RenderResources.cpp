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

#pragma region Query

Result Query::create(const QueryCreateInfo& pCreateInfo)
{
	if (pCreateInfo.type == QUERY_TYPE_UNDEFINED) return ERROR_GRFX_INVALID_QUERY_TYPE;
	if (pCreateInfo.count == 0)  return ERROR_GRFX_INVALID_QUERY_COUNT;

	Result ppxres = DeviceObject<QueryCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) return ppxres;

	return SUCCESS;
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

	RenderPassCreateInfo::RenderPassCreateInfo(const ::RenderPassCreateInfo& obj)
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

	RenderPassCreateInfo::RenderPassCreateInfo(const ::RenderPassCreateInfo2& obj)
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

	RenderPassCreateInfo::RenderPassCreateInfo(const ::RenderPassCreateInfo3& obj)
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
Result RenderPass::CreateImagesAndViewsV1(const internal::RenderPassCreateInfo& pCreateInfo)
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

Result RenderPass::CreateImagesAndViewsV2(const internal::RenderPassCreateInfo& pCreateInfo)
{
	// Create images
	{
		// RTV images
		for (uint32_t i = 0; i < pCreateInfo.renderTargetCount; ++i) {
			ResourceState initialState = RESOURCE_STATE_RENDER_TARGET;
			if (pCreateInfo.V2.renderTargetInitialStates[i] != RESOURCE_STATE_UNDEFINED) {
				initialState = pCreateInfo.V2.renderTargetInitialStates[i];
			}
			ImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.type = IMAGE_TYPE_2D;
			imageCreateInfo.width = pCreateInfo.width;
			imageCreateInfo.height = pCreateInfo.height;
			imageCreateInfo.depth = 1;
			imageCreateInfo.format = pCreateInfo.V2.renderTargetFormats[i];
			imageCreateInfo.sampleCount = pCreateInfo.V2.sampleCount;
			imageCreateInfo.mipLevelCount = 1;
			imageCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			imageCreateInfo.usageFlags = pCreateInfo.V2.renderTargetUsageFlags[i];
			imageCreateInfo.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			imageCreateInfo.initialState = RESOURCE_STATE_RENDER_TARGET;
			imageCreateInfo.RTVClearValue = pCreateInfo.renderTargetClearValues[i];
			imageCreateInfo.ownership = pCreateInfo.ownership;

			ImagePtr image;
			Result         ppxres = GetDevice()->CreateImage(&imageCreateInfo, &image);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV image create failed");
				return ppxres;
			}

			mRenderTargetImages.push_back(image);
		}

		// DSV image
		if (pCreateInfo.V2.depthStencilFormat != FORMAT_UNDEFINED) {
			ResourceState initialState = RESOURCE_STATE_DEPTH_STENCIL_WRITE;
			if (pCreateInfo.V2.depthStencilInitialState != RESOURCE_STATE_UNDEFINED) {
				initialState = pCreateInfo.V2.depthStencilInitialState;
			}

			ImageCreateInfo imageCreateInfo = {};
			imageCreateInfo.type = IMAGE_TYPE_2D;
			imageCreateInfo.width = pCreateInfo.width;
			imageCreateInfo.height = pCreateInfo.height;
			imageCreateInfo.depth = 1;
			imageCreateInfo.format = pCreateInfo.V2.depthStencilFormat;
			imageCreateInfo.sampleCount = pCreateInfo.V2.sampleCount;
			imageCreateInfo.mipLevelCount = 1;
			imageCreateInfo.arrayLayerCount = pCreateInfo.arrayLayerCount;
			imageCreateInfo.usageFlags = pCreateInfo.V2.depthStencilUsageFlags;
			imageCreateInfo.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			imageCreateInfo.initialState = initialState;
			imageCreateInfo.DSVClearValue = pCreateInfo.depthStencilClearValue;
			imageCreateInfo.ownership = pCreateInfo.ownership;

			ImagePtr image;
			Result         ppxres = GetDevice()->CreateImage(&imageCreateInfo, &image);
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
			Result                    ppxres = GetDevice()->CreateRenderTargetView(&rtvCreateInfo, &rtv);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "RTV create failed");
				return ppxres;
			}

			mRenderTargetViews.push_back(rtv);

			mHasLoadOpClear |= (rtvCreateInfo.loadOp == ATTACHMENT_LOAD_OP_CLEAR);
		}

		// DSV
		if (pCreateInfo.V2.depthStencilFormat != FORMAT_UNDEFINED) {
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
			Result                    ppxres = GetDevice()->CreateDepthStencilView(&dsvCreateInfo, &dsv);
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

Result RenderPass::CreateImagesAndViewsV3(const internal::RenderPassCreateInfo& pCreateInfo)
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
			Result                    ppxres = GetDevice()->CreateRenderTargetView(&rtvCreateInfo, &rtv);
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
			Result                    ppxres = GetDevice()->CreateDepthStencilView(&dsvCreateInfo, &dsv);
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
		Result ppxres = CreateImagesAndViewsV1(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
	} break;

	case internal::RenderPassCreateInfo::CREATE_INFO_VERSION_2: {
		Result ppxres = CreateImagesAndViewsV2(pCreateInfo);
		if (Failed(ppxres)) {
			return ppxres;
		}
	} break;

	case internal::RenderPassCreateInfo::CREATE_INFO_VERSION_3: {
		Result ppxres = CreateImagesAndViewsV3(pCreateInfo);
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

void RenderPass::destroy()
{
	for (uint32_t i = 0; i < m_createInfo.renderTargetCount; ++i) {
		RenderTargetViewPtr& rtv = mRenderTargetViews[i];
		if (rtv && (rtv->GetOwnership() != OWNERSHIP_REFERENCE)) {
			GetDevice()->DestroyRenderTargetView(rtv);
			rtv.Reset();
		}

		ImagePtr& image = mRenderTargetImages[i];
		if (image && (image->GetOwnership() != OWNERSHIP_REFERENCE)) {
			GetDevice()->DestroyImage(image);
			image.Reset();
		}
	}
	mRenderTargetViews.clear();
	mRenderTargetImages.clear();

	if (mDepthStencilView && (mDepthStencilView->GetOwnership() != OWNERSHIP_REFERENCE)) {
		GetDevice()->DestroyDepthStencilView(mDepthStencilView);
		mDepthStencilView.Reset();
	}

	if (mDepthStencilImage && (mDepthStencilImage->GetOwnership() != OWNERSHIP_REFERENCE)) {
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
	if (mRenderTargetViews[index]->GetOwnership() == OWNERSHIP_RESTRICTED) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mRenderTargetViews[index]->SetOwnership(OWNERSHIP_REFERENCE);

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
	if (mDepthStencilView->GetOwnership() == OWNERSHIP_RESTRICTED) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mDepthStencilView->SetOwnership(OWNERSHIP_REFERENCE);

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
	if (mRenderTargetImages[index]->GetOwnership() == OWNERSHIP_RESTRICTED) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mRenderTargetImages[index]->SetOwnership(OWNERSHIP_REFERENCE);

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
	if (mDepthStencilImage->GetOwnership() == OWNERSHIP_RESTRICTED) {
		return ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED;
	}

	mDepthStencilImage->SetOwnership(OWNERSHIP_REFERENCE);

	if (!IsNull(ppImage)) {
		*ppImage = mDepthStencilImage;
	}
	return SUCCESS;
}

#pragma endregion

#pragma region DrawPass

namespace internal {

	DrawPassCreateInfo::DrawPassCreateInfo(const ::DrawPassCreateInfo& obj)
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

	DrawPassCreateInfo::DrawPassCreateInfo(const ::DrawPassCreateInfo2& obj)
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

	DrawPassCreateInfo::DrawPassCreateInfo(const ::DrawPassCreateInfo3& obj)
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
			ci.imageType = IMAGE_TYPE_2D;
			ci.width = pCreateInfo.width;
			ci.height = pCreateInfo.height;
			ci.depth = 1;
			ci.imageFormat = pCreateInfo.V1.renderTargetFormats[i];
			ci.sampleCount = pCreateInfo.V1.sampleCount;
			ci.mipLevelCount = 1;
			ci.arrayLayerCount = 1;
			ci.usageFlags = pCreateInfo.V1.renderTargetUsageFlags[i];
			ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			ci.initialState = pCreateInfo.V1.renderTargetInitialStates[i];
			ci.RTVClearValue = pCreateInfo.renderTargetClearValues[i];
			ci.sampledImageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
			ci.sampledImageViewFormat = FORMAT_UNDEFINED;
			ci.renderTargetViewFormat = FORMAT_UNDEFINED;
			ci.depthStencilViewFormat = FORMAT_UNDEFINED;
			ci.storageImageViewFormat = FORMAT_UNDEFINED;
			ci.ownership = OWNERSHIP_EXCLUSIVE;
			ci.imageCreateFlags = pCreateInfo.V1.imageCreateFlags;

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "render target texture create failed");
				return ppxres;
			}

			mRenderTargetTextures.push_back(texture);
		}

		// DSV image
		if (pCreateInfo.V1.depthStencilFormat != FORMAT_UNDEFINED) {
			TextureCreateInfo ci = {};
			ci.pImage = nullptr;
			ci.imageType = IMAGE_TYPE_2D;
			ci.width = pCreateInfo.width;
			ci.height = pCreateInfo.height;
			ci.depth = 1;
			ci.imageFormat = pCreateInfo.V1.depthStencilFormat;
			ci.sampleCount = pCreateInfo.V1.sampleCount;
			ci.mipLevelCount = 1;
			ci.arrayLayerCount = 1;
			ci.usageFlags = pCreateInfo.V1.depthStencilUsageFlags;
			ci.memoryUsage = MEMORY_USAGE_GPU_ONLY;
			ci.initialState = pCreateInfo.V1.depthStencilInitialState;
			ci.DSVClearValue = pCreateInfo.depthStencilClearValue;
			ci.sampledImageViewType = IMAGE_VIEW_TYPE_UNDEFINED;
			ci.sampledImageViewFormat = FORMAT_UNDEFINED;
			ci.renderTargetViewFormat = FORMAT_UNDEFINED;
			ci.depthStencilViewFormat = FORMAT_UNDEFINED;
			ci.storageImageViewFormat = FORMAT_UNDEFINED;
			ci.ownership = OWNERSHIP_EXCLUSIVE;
			ci.imageCreateFlags = pCreateInfo.V1.imageCreateFlags;

			TexturePtr texture;
			Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
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
			Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
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
			Result           ppxres = GetDevice()->CreateTexture(&ci, &texture);
			if (Failed(ppxres)) {
				ASSERT_MSG(false, "dpeth stencil texture create failed");
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
			case RESOURCE_STATE_DEPTH_STENCIL_READ:
			case RESOURCE_STATE_DEPTH_READ_STENCIL_WRITE: {
				skip = true;
			} break;
			}
		}
		if (stencilLoadOp == ATTACHMENT_LOAD_OP_CLEAR) {
			switch (pCreateInfo.depthStencilState) {
			default: break;
			case RESOURCE_STATE_DEPTH_STENCIL_READ:
			case RESOURCE_STATE_DEPTH_WRITE_STENCIL_READ: {
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

		Result ppxres = GetDevice()->CreateRenderPass(&rpCreateInfo, &pass.renderPass);
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
		if (mRenderTargetTextures[i] && (mRenderTargetTextures[i]->GetOwnership() == OWNERSHIP_EXCLUSIVE)) {
			GetDevice()->DestroyTexture(mRenderTargetTextures[i]);
			mRenderTargetTextures[i].Reset();
		}
	}
	mRenderTargetTextures.clear();

	if (mDepthStencilTexture && (mDepthStencilTexture->GetOwnership() == OWNERSHIP_EXCLUSIVE)) {
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

#pragma endregion

#pragma region ShadingRate

std::unique_ptr<Bitmap> ShadingRatePattern::CreateBitmap() const
{
	auto bitmap = std::make_unique<Bitmap>();
	CHECKED_CALL(Bitmap::Create(
		GetAttachmentWidth(), GetAttachmentHeight(), GetBitmapFormat(), bitmap.get()));
	return bitmap;
}

Result ShadingRatePattern::LoadFromBitmap(Bitmap* bitmap)
{
	return grfx_util::CopyBitmapToImage(GetDevice()->GetGraphicsQueue(), bitmap, mAttachmentImage, 0, 0, mAttachmentImage->GetInitialState(), mAttachmentImage->GetInitialState());
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
	this->memoryUsage = MEMORY_USAGE_GPU_ONLY;
	std::memset(&this->vertexBuffers, 0, MAX_VERTEX_BINDINGS * sizeof(MeshVertexBufferDescription));

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
		if ((pCreateInfo.indexType != INDEX_TYPE_UINT16) && (pCreateInfo.indexType != INDEX_TYPE_UINT32)) {
			return Result::ERROR_GRFX_INVALID_INDEX_TYPE;
		}

		BufferCreateInfo createInfo = {};
		createInfo.size = pCreateInfo.indexCount * IndexTypeSize(pCreateInfo.indexType);
		createInfo.usageFlags.bits.indexBuffer = true;
		createInfo.usageFlags.bits.transferDst = true;
		createInfo.memoryUsage = pCreateInfo.memoryUsage;
		createInfo.initialState = RESOURCE_STATE_GENERAL;
		createInfo.ownership = OWNERSHIP_REFERENCE;

		auto ppxres = GetDevice()->CreateBuffer(&createInfo, &mIndexBuffer);
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
				if (format == FORMAT_UNDEFINED) {
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
			createInfo.initialState = RESOURCE_STATE_GENERAL;
			createInfo.ownership = OWNERSHIP_REFERENCE;

			auto ppxres = GetDevice()->CreateBuffer(&createInfo, &mVertexBuffers[vbIdx].first);
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

namespace internal {

	// -------------------------------------------------------------------------------------------------
	// internal
	// -------------------------------------------------------------------------------------------------
	void FillOutGraphicsPipelineCreateInfo(
		const GraphicsPipelineCreateInfo2* pSrcCreateInfo,
		GraphicsPipelineCreateInfo* pDstCreateInfo)
	{
		// Set to default values
		*pDstCreateInfo = {};

		pDstCreateInfo->dynamicRenderPass = pSrcCreateInfo->dynamicRenderPass;

		// Shaders
		pDstCreateInfo->VS = pSrcCreateInfo->VS;
		pDstCreateInfo->PS = pSrcCreateInfo->PS;

		// Vertex input
		{
			pDstCreateInfo->vertexInputState.bindingCount = pSrcCreateInfo->vertexInputState.bindingCount;
			for (uint32_t i = 0; i < pDstCreateInfo->vertexInputState.bindingCount; ++i) {
				pDstCreateInfo->vertexInputState.bindings[i] = pSrcCreateInfo->vertexInputState.bindings[i];
			}
		}

		// Input aasembly
		{
			pDstCreateInfo->inputAssemblyState.topology = pSrcCreateInfo->topology;
		}

		// Raster
		{
			pDstCreateInfo->rasterState.polygonMode = pSrcCreateInfo->polygonMode;
			pDstCreateInfo->rasterState.cullMode = pSrcCreateInfo->cullMode;
			pDstCreateInfo->rasterState.frontFace = pSrcCreateInfo->frontFace;
		}

		// Depth/stencil
		{
			pDstCreateInfo->depthStencilState.depthTestEnable = pSrcCreateInfo->depthReadEnable;
			pDstCreateInfo->depthStencilState.depthWriteEnable = pSrcCreateInfo->depthWriteEnable;
			pDstCreateInfo->depthStencilState.depthCompareOp = pSrcCreateInfo->depthCompareOp;
			pDstCreateInfo->depthStencilState.depthBoundsTestEnable = false;
			pDstCreateInfo->depthStencilState.minDepthBounds = 0.0f;
			pDstCreateInfo->depthStencilState.maxDepthBounds = 1.0f;
			pDstCreateInfo->depthStencilState.stencilTestEnable = false;
			pDstCreateInfo->depthStencilState.front = {};
			pDstCreateInfo->depthStencilState.back = {};
		}

		// Color blend
		{
			pDstCreateInfo->colorBlendState.blendAttachmentCount = pSrcCreateInfo->outputState.renderTargetCount;
			for (uint32_t i = 0; i < pDstCreateInfo->colorBlendState.blendAttachmentCount; ++i) {
				switch (pSrcCreateInfo->blendModes[i]) {
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
			pDstCreateInfo->outputState.renderTargetCount = pSrcCreateInfo->outputState.renderTargetCount;
			for (uint32_t i = 0; i < pDstCreateInfo->outputState.renderTargetCount; ++i) {
				pDstCreateInfo->outputState.renderTargetFormats[i] = pSrcCreateInfo->outputState.renderTargetFormats[i];
			}

			pDstCreateInfo->outputState.depthStencilFormat = pSrcCreateInfo->outputState.depthStencilFormat;
		}

		// Shading rate mode
		pDstCreateInfo->shadingRateMode = pSrcCreateInfo->shadingRateMode;

		// Pipeline internface
		pDstCreateInfo->pPipelineInterface = pSrcCreateInfo->pPipelineInterface;

		// MultiView details
		pDstCreateInfo->multiViewState = pSrcCreateInfo->multiViewState;
	}

} // namespace internal

// -------------------------------------------------------------------------------------------------
// ComputePipeline
// -------------------------------------------------------------------------------------------------
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

// -------------------------------------------------------------------------------------------------
// GraphicsPipeline
// -------------------------------------------------------------------------------------------------
Result GraphicsPipeline::create(const GraphicsPipelineCreateInfo& pCreateInfo)
{
	//// Checked binding range
	// for (uint32_t i = 0; i < pCreateInfo.vertexInputState.attributeCount; ++i) {
	//     const VertexAttribute& attribute = pCreateInfo.vertexInputState.attributes[i];
	//     if (attribute.binding >= MAX_VERTEX_BINDINGS) {
	//         ASSERT_MSG(false, "binding exceeds PPX_MAX_VERTEX_ATTRIBUTES");
	//         return ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED;
	//     }
	//     if (attribute.format == FORMAT_UNDEFINED) {
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

	if (pCreateInfo.dynamicRenderPass && !GetDevice()->DynamicRenderingSupported()) {
		ASSERT_MSG(false, "Cannot create a pipeline with dynamic render pass, dynamic rendering is not supported.");
		return ERROR_GRFX_OPERATION_NOT_PERMITTED;
	}

	Result ppxres = DeviceObject<GraphicsPipelineCreateInfo>::create(pCreateInfo);
	if (Failed(ppxres)) {
		return ppxres;
	}

	return SUCCESS;
}

// -------------------------------------------------------------------------------------------------
// PipelineInterface
// -------------------------------------------------------------------------------------------------
Result PipelineInterface::create(const PipelineInterfaceCreateInfo& pCreateInfo)
{
	if (pCreateInfo.setCount > MAX_BOUND_DESCRIPTOR_SETS) {
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
		if (pCreateInfo.pushConstants.count > MAX_PUSH_CONSTANTS) {
			ASSERT_MSG(false, "push constants count (" + pCreateInfo.pushConstants.count + ") exceeds MAX_PUSH_CONSTANTS (" + MAX_PUSH_CONSTANTS + ")");
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
				ASSERT_MSG(false, "push constants binding and set overlaps with a binding in set " + set.set);
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
	ASSERT_MSG(bufferCount < MAX_VERTEX_BINDINGS, "bufferCount exceeds PPX_MAX_VERTEX_ATTRIBUTES");

	VertexBufferView views[MAX_VERTEX_BINDINGS] = {};
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

	const Buffer* buffers[MAX_VERTEX_BINDINGS] = { nullptr };
	uint32_t            strides[MAX_VERTEX_BINDINGS] = { 0 };

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

	Result ppxres = GetDevice()->CreateCommandPool(&ci, &set.commandPool);
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

		cmd->BufferResourceBarrier(pDstBuffer, stateBefore, RESOURCE_STATE_COPY_DST);
		cmd->CopyBufferToBuffer(pCopyInfo, pSrcBuffer, pDstBuffer);
		cmd->BufferResourceBarrier(pDstBuffer, RESOURCE_STATE_COPY_DST, stateAfter);

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

		cmd->TransitionImageLayout(pDstImage, ALL_SUBRESOURCES, stateBefore, RESOURCE_STATE_COPY_DST);
		cmd->CopyBufferToImage(pCopyInfos, pSrcBuffer, pDstImage);
		cmd->TransitionImageLayout(pDstImage, ALL_SUBRESOURCES, RESOURCE_STATE_COPY_DST, stateAfter);

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

#pragma endregion

#pragma region FullscreenQuad

Result FullscreenQuad::createApiObjects(const FullscreenQuadCreateInfo& pCreateInfo)
{
	ASSERT_NULL_ARG(pCreateInfo);

	Result ppxres = ERROR_FAILED;

	// Pipeline interface
	{
		PipelineInterfaceCreateInfo createInfo = {};
		createInfo.setCount = pCreateInfo.setCount;
		for (uint32_t i = 0; i < createInfo.setCount; ++i) {
			createInfo.sets[i].set = pCreateInfo.sets[i].set;
			createInfo.sets[i].pLayout = pCreateInfo.sets[i].pLayout;
		}

		ppxres = GetDevice()->CreatePipelineInterface(&createInfo, &mPipelineInterface);
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

		ppxres = GetDevice()->CreateGraphicsPipeline(&createInfo, &mPipeline);
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
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroyImage(object);
		}
	}
	mImages.clear();

	for (auto& object : mBuffers) {
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroyBuffer(object);
		}
	}
	mBuffers.clear();

	for (auto& object : mMeshes) {
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroyMesh(object);
		}
	}
	mMeshes.clear();

	for (auto& object : mTextures) {
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroyTexture(object);
		}
	}
	mTextures.clear();

	for (auto& object : mSamplers) {
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroySampler(object);
		}
	}
	mSamplers.clear();

	for (auto& object : mSampledImageViews) {
		if (object->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
			mDevice->DestroySampledImageView(object);
		}
	}
	mSampledImageViews.clear();

	for (auto& object : mTransientCommandBuffers) {
		if (object.second->GetOwnership() == OWNERSHIP_EXCLUSIVE) {
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
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mImages.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Buffer* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mBuffers.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Mesh* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mMeshes.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Texture* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mTextures.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Sampler* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mSamplers.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(SampledImageView* pObject)
{
	if (IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
	mSampledImageViews.push_back(pObject);
	return SUCCESS;
}

Result ScopeDestroyer::AddObject(Queue* pParent, CommandBuffer* pObject)
{
	if (IsNull(pParent) || IsNull(pObject)) {
		ASSERT_MSG(false, NULL_ARGUMENT_MSG);
		return ERROR_UNEXPECTED_NULL_ARGUMENT;
	}
	if (pObject->GetOwnership() != OWNERSHIP_REFERENCE) {
		ASSERT_MSG(false, WRONG_OWNERSHIP_MSG);
		return ERROR_GRFX_INVALID_OWNERSHIP;
	}
	pObject->SetOwnership(OWNERSHIP_EXCLUSIVE);
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
	//		pCreateInfo.initialState,
	//		grfxQueue->GetCommandType(),
	//		pDevice->GetDeviceFeatures(),
	//		pipelineStage,
	//		accessMask,
	//		layout);
	//	if (Failed(ppxres)) {
	//		ASSERT_MSG(false, "couldn't determine pipeline stage and layout from initial state");
	//		return ppxres;
	//	}

	//	vk::Queue* pQueue = ToApi(grfxQueue.Get());

	//	VkResult vkres = pQueue->TransitionImageLayout(
	//		mImage,                       // image
	//		mImageAspect,                 // aspectMask
	//		0,                            // baseMipLevel
	//		pCreateInfo.mipLevelCount,   // levelCount
	//		0,                            // baseArrayLayer
	//		pCreateInfo.arrayLayerCount, // layerCount
	//		VK_IMAGE_LAYOUT_UNDEFINED,    // oldLayout
	//		layout,                       // newLayout
	//		pipelineStage);               // newPipelineStage)
	//	if (vkres != VK_SUCCESS)
	//	{
	//		Fatal("Queue::TransitionImageLayout failed: " << ToString(vkres));
	//		return ERROR_API_FAILURE;
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
	//	ASSERT_MSG(false, "vmaMapMemory failed: " << ToString(vkres));
	//	return ERROR_API_FAILURE;
	//}

	return true;
}

void VulkanImage::UnmapMemory()
{
	//vmaUnmapMemory(ToApi(GetDevice())->GetVmaAllocator(), m_allocation);
}

#pragma endregion
