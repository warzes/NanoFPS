#pragma once

class Buffer2
{
public:
	Buffer2(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	~Buffer2();

	void* Map(uint64_t offset);
	void Unmap();

private:
	VkBuffer m_buffer{ nullptr };
	VmaAllocation m_allocation{ nullptr };
	VmaAllocationInfo m_allocationInfo{};
};

using BufferPtr = std::shared_ptr<Buffer2>;