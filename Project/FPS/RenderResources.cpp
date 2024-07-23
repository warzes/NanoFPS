#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"

struct sBufferingObjects
{
	VkFence RenderFence;
	VkSemaphore PresentSemaphore;
	VkSemaphore RenderSemaphore;
	VkCommandBuffer CommandBuffer;
};

namespace
{
	std::array<sBufferingObjects, 3> BufferingObjects;
}

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

#pragma region VulkanUniformBufferSet

VulkanUniformBufferSet::VulkanUniformBufferSet(const std::initializer_list<VulkanUniformBufferInfo>& uniformBufferInfos)
{
	const size_t numBuffering = Render::GetNumBuffering();
	const size_t numUniformBuffers = uniformBufferInfos.size();

	std::vector<VkDescriptorSetLayoutBinding> bindings;
	bindings.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos)
	{
		bindings.emplace_back(uniformBufferInfo.Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, uniformBufferInfo.Stage);
	}
	m_descriptorSetLayout = Render::CreateDescriptorSetLayout(bindings);
	m_descriptorSet = Render::AllocateDescriptorSet(m_descriptorSetLayout);

	m_uniformBufferSizes.reserve(numUniformBuffers);
	m_uniformBuffers.reserve(numUniformBuffers);
	for (const auto& uniformBufferInfo : uniformBufferInfos) {
		m_uniformBufferSizes.push_back(uniformBufferInfo.Size);

		VulkanBuffer uniformBuffer = Render::CreateBuffer(
			numBuffering * uniformBufferInfo.Size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
			VMA_MEMORY_USAGE_AUTO_PREFER_HOST
		);

		Render::WriteDynamicUniformBufferToDescriptorSet(uniformBuffer.Get(), uniformBufferInfo.Size, m_descriptorSet, uniformBufferInfo.Binding);

		m_uniformBuffers.push_back(std::move(uniformBuffer));
	}

	m_dynamicOffsets.reserve(numBuffering);
	for (int i = 0; i < numBuffering; i++)
	{
		std::vector<uint32_t> dynamicOffsets;
		dynamicOffsets.reserve(numUniformBuffers);
		for (int j = 0; j < numUniformBuffers; j++)
		{
			dynamicOffsets.push_back(i * m_uniformBufferSizes[j]);
		}
		m_dynamicOffsets.push_back(dynamicOffsets);
	}
}

VulkanUniformBufferSet::VulkanUniformBufferSet(VulkanUniformBufferSet&& other) noexcept
{
	Swap(other);
}

VulkanUniformBufferSet::~VulkanUniformBufferSet()
{
	Release();
}

VulkanUniformBufferSet& VulkanUniformBufferSet::operator=(VulkanUniformBufferSet&& other) noexcept
{
	if (this != &other)
	{
		Release();
		Swap(other);
	}
	return *this;
}

void VulkanUniformBufferSet::Release()
{
	Render::FreeDescriptorSet(m_descriptorSet);
	Render::DestroyDescriptorSetLayout(m_descriptorSetLayout);

	m_descriptorSetLayout = VK_NULL_HANDLE;
	m_uniformBufferSizes.clear();
	m_uniformBuffers.clear();
	m_descriptorSet = VK_NULL_HANDLE;
	m_dynamicOffsets.clear();
}

void VulkanUniformBufferSet::Swap(VulkanUniformBufferSet& other) noexcept
{
	std::swap(m_descriptorSetLayout, other.m_descriptorSetLayout);
	std::swap(m_descriptorSet, other.m_descriptorSet);
	std::swap(m_uniformBufferSizes, other.m_uniformBufferSizes);
	std::swap(m_uniformBuffers, other.m_uniformBuffers);
	std::swap(m_dynamicOffsets, other.m_dynamicOffsets);
}

void VulkanUniformBufferSet::UpdateAllBuffers(uint32_t bufferingIndex, const std::initializer_list<const void*>& allBuffersData)
{
	int i = 0;
	for (const void* bufferData : allBuffersData)
	{
		const uint32_t size = m_uniformBufferSizes[i];
		const uint32_t offset = bufferingIndex * size;
		m_uniformBuffers[i].UploadRange(offset, size, bufferData);
		i++;
	}
}

#pragma endregion

#pragma region VulkanPipeline

VkPrimitiveTopology TopologyFromString(const std::string& topology) {
	if (topology == "point_list") {
		return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	}
	else if (topology == "line_list") {
		return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	}
	else if (topology == "line_strip") {
		return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
	}
	else if (topology == "triangle_list") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	}
	else if (topology == "triangle_strip") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
	}
	else if (topology == "triangle_fan") {
		return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	}
	Fatal("Invalid topology: " + topology);
	return {}; // TODO: optional
}

VkPolygonMode PolygonModeFromString(const std::string& polygonMode)
{
	if (polygonMode == "fill") {
		return VK_POLYGON_MODE_FILL;
	}
	else if (polygonMode == "line") {
		return VK_POLYGON_MODE_LINE;
	}
	else if (polygonMode == "point") {
		return VK_POLYGON_MODE_POINT;
	}
	Fatal("Invalid polygon mode: " + polygonMode);
	return {}; // TODO: optional
}

VkCullModeFlags CullModeFromString(const std::string& cullMode)
{
	if (cullMode == "none") {
		return VK_CULL_MODE_NONE;
	}
	else if (cullMode == "back") {
		return VK_CULL_MODE_BACK_BIT;
	}
	else if (cullMode == "front") {
		return VK_CULL_MODE_FRONT_BIT;
	}
	else if (cullMode == "front_and_back") {
		return VK_CULL_MODE_FRONT_AND_BACK;
	}
	Fatal("Invalid cull mode: " + cullMode);
	return {};
}

VkCompareOp CompareOpFromString(const std::string& compareOp) {
	if (compareOp == "never") {
		return VK_COMPARE_OP_NEVER;
	}
	else if (compareOp == "less") {
		return VK_COMPARE_OP_LESS;
	}
	else if (compareOp == "equal") {
		return VK_COMPARE_OP_EQUAL;
	}
	else if (compareOp == "less_or_equal") {
		return VK_COMPARE_OP_LESS_OR_EQUAL;
	}
	else if (compareOp == "greater") {
		return VK_COMPARE_OP_GREATER;
	}
	else if (compareOp == "not_equal") {
		return VK_COMPARE_OP_NOT_EQUAL;
	}
	else if (compareOp == "greater_or_equal") {
		return VK_COMPARE_OP_GREATER_OR_EQUAL;
	}
	else if (compareOp == "always") {
		return VK_COMPARE_OP_ALWAYS;
	}
	Fatal("Invalid compare op: " + compareOp);
	return {};
}

VulkanPipelineConfig::VulkanPipelineConfig(const std::string& jsonFilename)
{
	JsonFile pipelineJson(jsonFilename);
	VertexShader = pipelineJson.GetString("vertex");
	GeometryShader = pipelineJson.GetString("geometry", {});
	FragmentShader = pipelineJson.GetString("fragment");
	const std::string topology = pipelineJson.GetString("topology", {});
	Options.Topology = topology.empty() ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST : TopologyFromString(topology);
	const std::string polygonMode = pipelineJson.GetString("polygon_mode", {});
	Options.PolygonMode = polygonMode.empty() ? VK_POLYGON_MODE_FILL : PolygonModeFromString(polygonMode);
	const std::string cullMode = pipelineJson.GetString("cull_mode", {});
	Options.CullMode = cullMode.empty() ? VK_CULL_MODE_BACK_BIT : CullModeFromString(cullMode);
	Options.DepthTestEnable = pipelineJson.GetBoolean("depth_test", true) ? VK_TRUE : VK_FALSE;
	Options.DepthWriteEnable = pipelineJson.GetBoolean("depth_write", true) ? VK_TRUE : VK_FALSE;
	const std::string compareOp = pipelineJson.GetString("compare_op", {});
	Options.DepthCompareOp = compareOp.empty() ? VK_COMPARE_OP_LESS : CompareOpFromString(compareOp);
}


#pragma endregion

#pragma region Renderer

size_t Render::GetNumBuffering()
{
	return BufferingObjects.size();
}

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

VkDescriptorSetLayout Render::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VkDescriptorSetLayout descriptorSetLayout;
	if (vkCreateDescriptorSetLayout(RenderContext::GetInstance().Device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan descriptor set layout.");
		return {}; // TODO: return optional<VkDescriptorSetLayout>
	}
	return descriptorSetLayout;
}

VkDescriptorSet Render::AllocateDescriptorSet(VkDescriptorSetLayout descriptorSetLayout)
{
	VkDescriptorSetAllocateInfo allocateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = RenderContext::GetInstance().DescriptorPool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &descriptorSetLayout;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(RenderContext::GetInstance().Device, &allocateInfo, &descriptorSet) != VK_SUCCESS)
	{
		Fatal("Failed to allocate Vulkan descriptor set.");
		return {}; // TODO: return optional<VkDescriptorSet>
	}
	return descriptorSet;
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

void Render::WriteDynamicUniformBufferToDescriptorSet(VkBuffer buffer, VkDeviceSize size, VkDescriptorSet descriptorSet, uint32_t binding)
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = buffer;
	bufferInfo.offset = 0;
	bufferInfo.range = size;

	VkWriteDescriptorSet writeDescSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescSet.dstSet = descriptorSet;
	writeDescSet.dstBinding = binding;
	writeDescSet.dstArrayElement = 0;
	writeDescSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	writeDescSet.descriptorCount = 1;
	writeDescSet.pBufferInfo = &bufferInfo;
	writeDescriptorSet(writeDescSet);
}

void Render::DestroySampler(VkSampler sampler)
{
	vkDestroySampler(RenderContext::GetInstance().Device, sampler, nullptr);
}

void Render::DestroyImageView(VkImageView imageView)
{
	vkDestroyImageView(RenderContext::GetInstance().Device, imageView, nullptr);
}

void Render::FreeDescriptorSet(VkDescriptorSet descriptorSet)
{
	vkFreeDescriptorSets(RenderContext::GetInstance().Device, RenderContext::GetInstance().DescriptorPool, 1, &descriptorSet);
}

void Render::DestroyDescriptorSetLayout(VkDescriptorSetLayout descriptorSetLayout)
{
	vkDestroyDescriptorSetLayout(RenderContext::GetInstance().Device, descriptorSetLayout, nullptr);
}

#pragma endregion