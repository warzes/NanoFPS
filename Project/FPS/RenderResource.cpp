#include "Base.h"
#include "Core.h"
#include "Renderer.h"
#include "RenderResource.h"

Buffer2::Buffer2(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	VkBufferCreateInfo bufferInfo = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	bufferInfo.size = allocSize;
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = memoryUsage;
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	VkResult result = vmaCreateBuffer(Renderer::GetAllocator(), &bufferInfo, &vmaallocInfo, &m_buffer, &m_allocation, &m_allocationInfo);
	if (result != VK_SUCCESS)
	{
		Fatal("Detected Vulkan error: " + std::string(string_VkResult(result)));
	}
}

Buffer2::~Buffer2()
{
	vmaDestroyBuffer(Renderer::GetAllocator(), m_buffer, m_allocation);
}

void* Buffer2::Map(uint64_t offset)
{
	void* ppMappedAddress{nullptr};
	VkResult result = vmaMapMemory(Renderer::GetAllocator(), m_allocation, &ppMappedAddress);
	if (result != VK_SUCCESS || !ppMappedAddress)
	{
		Fatal("vmaMapMemory failed: " + std::string(string_VkResult(result)));
	}

	return ppMappedAddress;
}

void Buffer2::Unmap()
{
	vmaUnmapMemory(Renderer::GetAllocator(), m_allocation);
}