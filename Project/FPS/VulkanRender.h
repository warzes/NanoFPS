#pragma once

#pragma region DebugVK

template<typename... Args>
inline void DebugCheckVk(const VkResult result, const spdlog::format_string_t<Args...>& failMessage, Args... args)
{
	DebugCheck(result == VK_SUCCESS, failMessage, args...);
}

template<typename... Args>
inline void DebugCheckVk(const vk::Result result, const spdlog::format_string_t<Args...>& failMessage, Args... args)
{
	DebugCheck(result == vk::Result::eSuccess, failMessage, args...);
}

template<typename... Args>
inline void DebugCheckCriticalVk(const VkResult result, const spdlog::format_string_t<Args...>& failMessage, Args... args)
{
	DebugCheckCritical(result == VK_SUCCESS, failMessage, args...);
}

template<typename... Args>
inline void DebugCheckCriticalVk(const vk::Result result, const spdlog::format_string_t<Args...>& failMessage, Args... args)
{
	DebugCheckCritical(result == vk::Result::eSuccess, failMessage, args...);
}

#pragma endregion

#pragma region VulkanBuffer


class VulkanBuffer {
public:
	VulkanBuffer() = default;

	VulkanBuffer(
		VmaAllocator             allocator,
		vk::DeviceSize           size,
		vk::BufferUsageFlags     bufferUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage
	);

	~VulkanBuffer() { Release(); }

	VulkanBuffer(const VulkanBuffer&) = delete;

	VulkanBuffer& operator=(const VulkanBuffer&) = delete;

	VulkanBuffer(VulkanBuffer&& other) noexcept { Swap(other); }

	VulkanBuffer& operator=(VulkanBuffer&& other) noexcept {
		if (this != &other) {
			Release();
			Swap(other);
		}
		return *this;
	}

	void Release();

	void Swap(VulkanBuffer& other) noexcept;

	void UploadRange(size_t offset, size_t size, const void* data) const;

	void Upload(size_t size, const void* data) const { UploadRange(0, size, data); }

	[[nodiscard]] const vk::Buffer& Get() const { return m_buffer; }

private:
	VmaAllocator m_allocator = VK_NULL_HANDLE;

	vk::Buffer    m_buffer;
	VmaAllocation m_allocation = VK_NULL_HANDLE;
};

#pragma endregion
