#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"

#pragma region VulkanImage

VulkanImage::VulkanImage(VmaAllocator allocator, VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers)
	: m_allocator(allocator)
{
	VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.format = format;
	imageInfo.extent.width = extent.width;
	imageInfo.extent.height = extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = layers;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.usage = imageUsage;

	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateImage(m_allocator, &imageInfo, &allocationCreateInfo, &m_image, &m_allocation, nullptr) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan 2d image.");
		m_image = nullptr;
		return;
	}
}

VulkanImage::VulkanImage(VulkanImage&& other) noexcept
{
	Swap(other);
}

VulkanImage::~VulkanImage()
{
	Release();
}

VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanImage::Release()
{
	if (m_allocator)
		vmaDestroyImage(m_allocator, m_image, m_allocation);

	m_allocator = VK_NULL_HANDLE;
	m_image = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanImage::Swap(VulkanImage& other) noexcept
{
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_image, other.m_image);
	std::swap(m_allocation, other.m_allocation);
}

#pragma endregion

#pragma region VulkanBuffer

VulkanBuffer::VulkanBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage)
	: m_allocator(allocator)
{
	VkBufferCreateInfo bufferCreateInfo{ .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = bufferUsage;
	
	VmaAllocationCreateInfo allocationCreateInfo{};
	allocationCreateInfo.flags = flags;
	allocationCreateInfo.usage = memoryUsage;

	if (vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_allocation, nullptr) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan buffer.");
		m_buffer = nullptr;
		return;
	}
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
{
	Swap(other);
}

VulkanBuffer::~VulkanBuffer()
{
	Release();
}

void VulkanBuffer::Release()
{
	if (m_allocator)
		vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);

	m_allocator = VK_NULL_HANDLE;
	m_buffer = VK_NULL_HANDLE;
	m_allocation = VK_NULL_HANDLE;
}

void VulkanBuffer::Swap(VulkanBuffer& other) noexcept
{
	std::swap(m_allocator, other.m_allocator);
	std::swap(m_buffer, other.m_buffer);
	std::swap(m_allocation, other.m_allocation);
}

void VulkanBuffer::UploadRange(size_t offset, size_t size, const void* data) const
{
	void* mappedMemory = nullptr;
	if (vmaMapMemory(m_allocator, m_allocation, &mappedMemory) != VK_SUCCESS)
	{
		Fatal("Failed to map Vulkan memory.");
		return;
	}
	memcpy(static_cast<unsigned char*>(mappedMemory) + offset, data, size);
	vmaUnmapMemory(m_allocator, m_allocation);
}

#pragma endregion

#pragma region VulkanTexture

VulkanTexture::VulkanTexture(uint32_t width, uint32_t height, const unsigned char* data, VkFilter filter, VkSamplerAddressMode addressMode)
{
	constexpr size_t PIXEL_SIZE = sizeof(unsigned char) * 4;
	createImage(width, height, width * height * PIXEL_SIZE, data, VK_FORMAT_R8G8B8A8_UNORM);
	createImageView(VK_FORMAT_R8G8B8A8_UNORM);
	createSampler(filter, addressMode);
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
{
	Swap(other);
}

VulkanTexture::~VulkanTexture()
{
	Release();
}

VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanTexture::Release()
{
	Render::DestroySampler(m_sampler);
	Render::DestroyImageView(m_imageView);

	m_image = {};
	m_imageView = VK_NULL_HANDLE;
	m_sampler = VK_NULL_HANDLE;
}

void VulkanTexture::Swap(VulkanTexture& other) noexcept
{
	std::swap(m_image, other.m_image);
	std::swap(m_imageView, other.m_imageView);
	std::swap(m_sampler, other.m_sampler);
}

void VulkanTexture::BindToDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding)
{
	Render::WriteCombinedImageSamplerToDescriptorSet(m_sampler, m_imageView, descriptorSet, binding);
}

void VulkanTexture::createImage(uint32_t width, uint32_t height, VkDeviceSize size, const void* data, VkFormat format)
{
	VulkanBuffer uploadBuffer = Render::CreateBuffer(
		size,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
		VMA_MEMORY_USAGE_AUTO_PREFER_HOST
	);
	uploadBuffer.Upload(size, data);

	VulkanImage image = Render::CreateImage(
		format,
		VkExtent2D{ width, height },
		VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		0,
		VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE
	);

	const VkExtent3D extent{ width, height, 1 };

	Render::BeginImmediateSubmit();
	{
		VkImageMemoryBarrier imageMemoryBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.image = image.Get();
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		vkCmdPipelineBarrier(
			RenderContext::GetInstance().ImmediateCommandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0,
			0, nullptr,
			0, nullptr,
			1, &imageMemoryBarrier);

		VkBufferImageCopy imageCopy{};
		imageCopy.bufferOffset = 0;
		imageCopy.bufferRowLength = 0;
		imageCopy.bufferImageHeight = 0;

		imageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageCopy.imageSubresource.mipLevel = 0;
		imageCopy.imageSubresource.baseArrayLayer = 0;
		imageCopy.imageSubresource.layerCount = 1;

		imageCopy.imageOffset = { 0, 0, 0 };
		imageCopy.imageExtent = extent;

		vkCmdCopyBufferToImage(
			RenderContext::GetInstance().ImmediateCommandBuffer,
			uploadBuffer.Get(),
			image.Get(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageCopy
		);


		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkCmdPipelineBarrier(RenderContext::GetInstance().ImmediateCommandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, 0, 0, 0,
			1, &imageMemoryBarrier);
	}
	Render::EndImmediateSubmit();

	m_image = std::move(image);
}

void VulkanTexture::createImageView(VkFormat format)
{
	m_imageView = Render::CreateImageView(m_image.Get(), format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanTexture::createSampler(VkFilter filter, VkSamplerAddressMode addressMode)
{
	m_sampler = Render::CreateSampler(filter, addressMode);
}

#pragma endregion

#pragma region Renderer

VulkanImage Render::CreateImage(VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers)
{
	return { RenderContext::GetInstance().Allocator, format, extent, imageUsage, flags, memoryUsage, layers };
}

VkImageView Render::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layers)
{
	VkImageViewCreateInfo viewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = image;
	viewInfo.viewType = layers == 1 ? VK_IMAGE_VIEW_TYPE_2D : VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = layers;

	VkImageView imageView;
	if (vkCreateImageView(RenderContext::GetInstance().Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan image view.");
		return {}; // TODO: return optional<VkImageView>
	}
	return imageView;
}

VkSampler Render::CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 compareEnable, VkCompareOp compareOp)
{
	VkSamplerCreateInfo samplerInfo{ .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	samplerInfo.magFilter = filter;
	samplerInfo.minFilter = filter;
	samplerInfo.addressModeU = addressMode;
	samplerInfo.addressModeV = addressMode;
	samplerInfo.addressModeW = addressMode;
	samplerInfo.compareEnable = compareEnable;
	samplerInfo.compareOp = compareOp;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;

	VkSampler sampler;
	if (vkCreateSampler(RenderContext::GetInstance().Device, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan sampler");
		return {}; // TODO: return optional<VkImageView>
	}
	return sampler;
}

VulkanBuffer Render::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage)
{
	return { RenderContext::GetInstance().Allocator, size, bufferUsage, flags, memoryUsage};
}

void Render::BeginImmediateSubmit()
{
	vkResetCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer, 0);
	BeginCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
}

void Render::EndImmediateSubmit()
{
	EndCommandBuffer(RenderContext::GetInstance().ImmediateCommandBuffer);
	
	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &RenderContext::GetInstance().ImmediateCommandBuffer;
	if (vkQueueSubmit(RenderContext::GetInstance().GraphicsQueue, 1, &submitInfo, RenderContext::GetInstance().ImmediateFence) != VK_SUCCESS)
	{
		Fatal("failed to submit draw command buffer");
		return;
	}
	WaitAndResetFence(RenderContext::GetInstance().ImmediateFence);
}

bool Render::BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = flags;
	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		Fatal("failed to begin recording command buffer!");
		return false;
	}

	return true;
}

bool Render::EndCommandBuffer(VkCommandBuffer commandBuffer)
{
	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		Fatal("failed to record command buffer");
		return false;
	}

	return true;
}

void Render::WaitAndResetFence(VkFence fence, uint64_t timeout)
{
	vkWaitForFences(RenderContext::GetInstance().Device, 1, &fence, VK_TRUE, timeout);
	vkResetFences(RenderContext::GetInstance().Device, 1, &fence);
}

void writeDescriptorSet(const VkWriteDescriptorSet& writeDescriptorSet)
{
	vkUpdateDescriptorSets(RenderContext::GetInstance().Device, 1, &writeDescriptorSet, 0, 0);
}

void Render::WriteCombinedImageSamplerToDescriptorSet(VkSampler sampler, VkImageView imageView, VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet writeDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = descriptorSet;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pImageInfo = &imageInfo;
	
	vkUpdateDescriptorSets(RenderContext::GetInstance().Device, 1, &writeDescriptorSet, 0, nullptr);
}

void Render::DestroySampler(VkSampler sampler)
{
	vkDestroySampler(RenderContext::GetInstance().Device, sampler, nullptr);
}

void Render::DestroyImageView(VkImageView imageView)
{
	vkDestroyImageView(RenderContext::GetInstance().Device, imageView, nullptr);
}

#pragma endregion