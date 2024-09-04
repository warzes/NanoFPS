#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"

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
	//	grfx::QueuePtr grfxQueue = GetDevice()->GetAnyAvailableQueue();
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
