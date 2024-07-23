#pragma once

#pragma region VulkanImage

class VulkanImage final
{
public:
	VulkanImage() = default;
	VulkanImage(VmaAllocator allocator, VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers = 1);
	VulkanImage(const VulkanImage&) = delete;
	VulkanImage(VulkanImage&& other) noexcept;
	~VulkanImage();

	VulkanImage& operator=(const VulkanImage&) = delete;
	VulkanImage& operator=(VulkanImage&& other) noexcept;

	void Release();

	void Swap(VulkanImage& other) noexcept;

	[[nodiscard]] const VkImage& Get() const { return m_image; }

private:
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
	VkImage m_image{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanBuffer

class VulkanBuffer final
{
public:
	VulkanBuffer() = default;
	VulkanBuffer(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage);
	VulkanBuffer(const VulkanBuffer&) = delete;
	VulkanBuffer(VulkanBuffer&& other) noexcept;
	~VulkanBuffer();

	VulkanBuffer& operator=(const VulkanBuffer&) = delete;
	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

	void Release();

	void Swap(VulkanBuffer& other) noexcept;

	void UploadRange(size_t offset, size_t size, const void* data) const;

	void Upload(size_t size, const void* data) const { UploadRange(0, size, data); }

	[[nodiscard]] const VkBuffer& Get() const { return m_buffer; }

private:
	VmaAllocator m_allocator{ VK_NULL_HANDLE };
	VkBuffer m_buffer{ VK_NULL_HANDLE };
	VmaAllocation m_allocation{ VK_NULL_HANDLE };
};

#pragma endregion

#pragma region VulkanTexture

class VulkanTexture final
{
public:
	VulkanTexture() = default;
	VulkanTexture(uint32_t width, uint32_t height, const unsigned char* data, VkFilter filter, VkSamplerAddressMode addressMode);
	VulkanTexture(const VulkanTexture&) = delete;
	VulkanTexture(VulkanTexture&& other) noexcept;
	~VulkanTexture();

	VulkanTexture& operator=(const VulkanTexture&) = delete;
	VulkanTexture& operator=(VulkanTexture&& other) noexcept;

	void Release();

	void Swap(VulkanTexture& other) noexcept;

	void BindToDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding);

private:
	void createImage(uint32_t width, uint32_t height, VkDeviceSize size, const void* data, VkFormat format);
	void createImageView(VkFormat format);
	void createSampler(VkFilter filter, VkSamplerAddressMode addressMode);

	VulkanImage m_image{};
	VkImageView m_imageView{ VK_NULL_HANDLE };
	VkSampler m_sampler{ VK_NULL_HANDLE };
};

#pragma endregion


#pragma region Renderer

namespace Render
{
	VulkanImage CreateImage(VkFormat format, const VkExtent2D& extent, VkImageUsageFlags imageUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage, uint32_t layers = 1);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectMask, uint32_t layers = 1);
	VkSampler CreateSampler(VkFilter filter, VkSamplerAddressMode addressMode, VkBool32 compareEnable = VK_FALSE, VkCompareOp compareOp = VK_COMPARE_OP_NEVER);

	VulkanBuffer CreateBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage);

	void BeginImmediateSubmit();
	void EndImmediateSubmit();

	bool BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags flags = {});
	bool EndCommandBuffer(VkCommandBuffer commandBuffer);

	void WaitAndResetFence(VkFence fence, uint64_t timeout = 100'000'000);

	void WriteCombinedImageSamplerToDescriptorSet(VkSampler sampler, VkImageView imageView, VkDescriptorSet descriptorSet, uint32_t binding);

	void DestroySampler(VkSampler sampler);
	void DestroyImageView(VkImageView imageView);
}

#pragma endregion
