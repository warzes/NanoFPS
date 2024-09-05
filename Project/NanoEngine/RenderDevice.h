#pragma once

#include "RenderResources.h"

class EngineApplication;
class RenderSystem;
class DeviceQueue;

#pragma region RenderDevice

class RenderDevice final
{
public:
	RenderDevice(EngineApplication& engine, RenderSystem& render);

	VkDevice& GetVkDevice();
	VmaAllocatorPtr GetVmaAllocator();

	DeviceQueuePtr GetGraphicsQueue() const;
	DeviceQueuePtr GetPresentQueue() const;
	DeviceQueuePtr GetTransferQueue() const;
	DeviceQueuePtr GetComputeQueue() const;
	uint32_t GetGraphicsQueueFamilyIndex() const;
	uint32_t GetComputeQueueFamilyIndex() const;
	uint32_t GetTransferQueueFamilyIndex() const;
	std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

	Result CreateFence(const FenceCreateInfo& createInfo, Fence** ppFence);
	void   DestroyFence(const Fence* pFence);

	//===================================================================
	// OLD
	//===================================================================

	VulkanFencePtr CreateFence(const FenceCreateInfo& createInfo);
	VulkanSemaphorePtr CreateSemaphore(const SemaphoreCreateInfo& createInfo);

	VulkanCommandPoolPtr CreateCommandPool(const CommandPoolCreateInfo& createInfo) { return {}; }

	bool AllocateCommandBuffer(VulkanCommandPoolPtr pool, VulkanCommandBufferPtr& commandBuffer, uint32_t resourceDescriptorCount = DEFAULT_RESOURCE_DESCRIPTOR_COUNT, uint32_t samplerDescriptorCount = DEFAULT_SAMPLE_DESCRIPTOR_COUNT) { return true; }
	void FreeCommandBuffer(VulkanCommandBufferPtr& commandBuffer) {}

private:
	Result allocateObject(Buffer** ppObject);
	Result allocateObject(CommandBuffer** ppObject);
	Result allocateObject(CommandPool** ppObject);
	Result allocateObject(ComputePipeline** ppObject);
	Result allocateObject(DepthStencilView** ppObject);
	Result allocateObject(DescriptorPool** ppObject);
	Result allocateObject(DescriptorSet** ppObject);
	Result allocateObject(DescriptorSetLayout** ppObject);
	Result allocateObject(Fence** ppObject);
	Result allocateObject(GraphicsPipeline** ppObject);
	Result allocateObject(Image** ppObject);
	Result allocateObject(PipelineInterface** ppObject);
	Result allocateObject(Queue** ppObject);
	Result allocateObject(Query** ppObject);
	Result allocateObject(RenderPass** ppObject);
	Result allocateObject(RenderTargetView** ppObject);
	Result allocateObject(SampledImageView** ppObject);
	Result allocateObject(Sampler** ppObject);
	Result allocateObject(SamplerYcbcrConversion** ppObject);
	Result allocateObject(Semaphore** ppObject);
	Result allocateObject(ShaderModule** ppObject);
	Result allocateObject(ShaderProgram** ppObject);
	Result allocateObject(ShadingRatePattern** ppObject);
	Result allocateObject(StorageImageView** ppObject);
	Result allocateObject(Swapchain** ppObject);

	Result allocateObject(DrawPass** ppObject);
	Result allocateObject(FullscreenQuad** ppObject);
	Result allocateObject(Mesh** ppObject);
	Result allocateObject(TextDraw** ppObject);
	Result allocateObject(Texture** ppObject);
	Result allocateObject(TextureFont** ppObject);


	template <typename ObjectT, typename CreateInfoT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	Result createObject(const CreateInfoT& createInfo, ContainerT& container, ObjectT** ppObject);

	template <typename ObjectT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	void destroyObject(ContainerT& container, const ObjectT* pObject);

	template <typename ObjectT>
	void destroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

	EngineApplication& m_engine;
	RenderSystem& m_render;

	std::vector<FencePtr> m_fences;
};

#pragma endregion