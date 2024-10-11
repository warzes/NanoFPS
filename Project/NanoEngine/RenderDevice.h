#pragma once

#include "RenderResources.h"

class EngineApplication;

namespace vkr {

class RenderSystem;
class DeviceQueue;

//=============================================================================
#pragma region [ RenderDevice ]

class RenderDevice final
{
public:
	RenderDevice(EngineApplication& engine, RenderSystem& render);

	bool Setup(ShadingRateMode supportShadingRateMode);
	void Shutdown();

	[[nodiscard]] VkDevice& GetVkDevice();
	[[nodiscard]] VmaAllocatorPtr GetVmaAllocator();

	[[nodiscard]] const VkPhysicalDeviceFeatures& GetDeviceFeatures() const;
	[[nodiscard]] const VkPhysicalDeviceLimits& GetDeviceLimits() const;
	[[nodiscard]] float GetDeviceTimestampPeriod() const;

	[[nodiscard]] const ShadingRateCapabilities& GetShadingRateCapabilities() const;

	[[nodiscard]] uint32_t GetMaxPushDescriptors() const;
	[[nodiscard]] bool HasDepthClipEnabled() const;
	[[nodiscard]] bool HasDescriptorIndexingFeatures() const;
	[[nodiscard]] bool HasMultiView() const;
	[[nodiscard]] bool PartialDescriptorBindingsSupported() const;

	[[nodiscard]] bool PipelineStatsAvailable() const;
	[[nodiscard]] bool IndependentBlendingSupported() const;
	[[nodiscard]] bool FragmentStoresAndAtomicsSupported() const;

	[[nodiscard]] DeviceQueuePtr GetGraphicsDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetPresentDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetTransferDeviceQueue() const;
	[[nodiscard]] DeviceQueuePtr GetComputeDeviceQueue() const;
	[[nodiscard]] uint32_t GetGraphicsQueueFamilyIndex() const;
	[[nodiscard]] uint32_t GetComputeQueueFamilyIndex() const;
	[[nodiscard]] uint32_t GetTransferQueueFamilyIndex() const;
	[[nodiscard]] std::array<uint32_t, 3> GetAllQueueFamilyIndices() const;

	[[nodiscard]] QueuePtr GetGraphicsQueue() const;
	[[nodiscard]] QueuePtr GetComputeQueue() const;
	[[nodiscard]] QueuePtr GetAnyAvailableQueue() const;

	std::vector<char> LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName);

	Result CreateShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName, ShaderModule** ppShaderModule);

	Result CreateBuffer(const BufferCreateInfo& createInfo, Buffer** buffer);
	void   DestroyBuffer(const Buffer* buffer);

	Result CreateCommandPool(const CommandPoolCreateInfo& createInfo, CommandPool**commandPool);
	void   DestroyCommandPool(const CommandPool* commandPool);

	Result CreateComputePipeline(const ComputePipelineCreateInfo& createInfo, ComputePipeline** computePipeline);
	void   DestroyComputePipeline(const ComputePipeline* computePipeline);

	Result CreateDepthStencilView(const DepthStencilViewCreateInfo& createInfo, DepthStencilView** depthStencilView);
	void   DestroyDepthStencilView(const DepthStencilView* depthStencilView);

	Result CreateDescriptorPool(const DescriptorPoolCreateInfo& createInfo, DescriptorPool** descriptorPool);
	void   DestroyDescriptorPool(const DescriptorPool* descriptorPool);

	Result CreateDescriptorSetLayout(const DescriptorSetLayoutCreateInfo& createInfo, DescriptorSetLayout** descriptorSetLayout);
	void   DestroyDescriptorSetLayout(const DescriptorSetLayout* descriptorSetLayout);

	Result CreateDrawPass(const DrawPassCreateInfo& createInfo, DrawPass** drawPass);
	Result CreateDrawPass(const DrawPassCreateInfo2& createInfo, DrawPass** drawPass);
	Result CreateDrawPass(const DrawPassCreateInfo3& createInfo, DrawPass** drawPass);
	void   DestroyDrawPass(const DrawPass* drawPass);

	Result CreateFence(const FenceCreateInfo& createInfo, Fence** fence);
	void   DestroyFence(const Fence* fence);

	Result CreateShadingRatePattern(const ShadingRatePatternCreateInfo& createInfo, ShadingRatePattern** shadingRatePattern);
	void   DestroyShadingRatePattern(const ShadingRatePattern* shadingRatePattern);

	Result CreateFullscreenQuad(const FullscreenQuadCreateInfo& createInfo, FullscreenQuad** fullscreenQuad);
	void   DestroyFullscreenQuad(const FullscreenQuad* fullscreenQuad);

	Result CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo, GraphicsPipeline** graphicsPipeline);
	Result CreateGraphicsPipeline(const GraphicsPipelineCreateInfo2& createInfo, GraphicsPipeline** graphicsPipeline);
	void   DestroyGraphicsPipeline(const GraphicsPipeline* graphicsPipeline);

	Result CreateImage(const ImageCreateInfo& createInfo, Image** image);
	void   DestroyImage(const Image* image);

	Result CreateMesh(const MeshCreateInfo& createInfo, Mesh** mesh);
	void   DestroyMesh(const Mesh* mesh);

	Result CreatePipelineInterface(const PipelineInterfaceCreateInfo& createInfo, PipelineInterface** pipelineInterface);
	void   DestroyPipelineInterface(const PipelineInterface* pipelineInterface);

	Result CreateQuery(const QueryCreateInfo& createInfo, Query** query);
	void   DestroyQuery(const Query* query);

	Result CreateRenderPass(const RenderPassCreateInfo& createInfo, RenderPass** renderPass);
	Result CreateRenderPass(const RenderPassCreateInfo2& createInfo, RenderPass** renderPass);
	Result CreateRenderPass(const RenderPassCreateInfo3& createInfo, RenderPass** renderPass);
	void   DestroyRenderPass(const RenderPass* renderPass);

	Result CreateRenderTargetView(const RenderTargetViewCreateInfo& createInfo, RenderTargetView** renderTargetView);
	void   DestroyRenderTargetView(const RenderTargetView* renderTargetView);

	Result CreateSampledImageView(const SampledImageViewCreateInfo& createInfo, SampledImageView** sampledImageView);
	void   DestroySampledImageView(const SampledImageView* sampledImageView);

	Result CreateSampler(const SamplerCreateInfo& createInfo, Sampler** sampler);
	void   DestroySampler(const Sampler* sampler);

	Result CreateSamplerYcbcrConversion(const SamplerYcbcrConversionCreateInfo& createInfo, SamplerYcbcrConversion** conversion);
	void   DestroySamplerYcbcrConversion(const SamplerYcbcrConversion* conversion);

	Result CreateSemaphore(const SemaphoreCreateInfo& createInfo, Semaphore** semaphore);
	void   DestroySemaphore(const Semaphore* semaphore);

	Result CreateShaderModule(const ShaderModuleCreateInfo& createInfo, ShaderModule** shaderModule);
	void   DestroyShaderModule(const ShaderModule* shaderModule);

	Result CreateStorageImageView(const StorageImageViewCreateInfo& createInfo, StorageImageView** storageImageView);
	void   DestroyStorageImageView(const StorageImageView* storageImageView);

	Result CreateTextDraw(const TextDrawCreateInfo& createInfo, TextDraw** textDraw);
	void   DestroyTextDraw(const TextDraw* textDraw);

	Result CreateTexture(const TextureCreateInfo& createInfo, Texture** texture);
	void   DestroyTexture(const Texture* texture);

	Result CreateTextureFont(const TextureFontCreateInfo& createInfo, TextureFont** textureFont);
	void   DestroyTextureFont(const TextureFont* textureFont);

	Result AllocateCommandBuffer(const CommandPool* pool, CommandBuffer** commandBuffer, uint32_t resourceDescriptorCount = DefaultResourceDescriptorCount, uint32_t samplerDescriptorCount = DefaultSampleDescriptorCount);
	void FreeCommandBuffer(const CommandBuffer* commandBuffer);

	Result AllocateDescriptorSet(DescriptorPool* pool, const DescriptorSetLayout* layout, DescriptorSet** set);
	void   FreeDescriptorSet(const DescriptorSet* set);

private:
	Result allocateObject(Buffer** object);
	Result allocateObject(CommandBuffer** object);
	Result allocateObject(CommandPool** object);
	Result allocateObject(ComputePipeline** object);
	Result allocateObject(DepthStencilView** object);
	Result allocateObject(DescriptorPool** object);
	Result allocateObject(DescriptorSet** object);
	Result allocateObject(DescriptorSetLayout** object);
	Result allocateObject(Fence** object);
	Result allocateObject(GraphicsPipeline** object);
	Result allocateObject(Image** object);
	Result allocateObject(PipelineInterface** object);
	Result allocateObject(Queue** object);
	Result allocateObject(Query** object);
	Result allocateObject(RenderPass** object);
	Result allocateObject(RenderTargetView** object);
	Result allocateObject(SampledImageView** object);
	Result allocateObject(Sampler** object);
	Result allocateObject(SamplerYcbcrConversion** object);
	Result allocateObject(Semaphore** object);
	Result allocateObject(ShaderModule** object);
	Result allocateObject(ShadingRatePattern** object);
	Result allocateObject(StorageImageView** object);
	Result allocateObject(DrawPass** object);
	Result allocateObject(FullscreenQuad** object);
	Result allocateObject(Mesh** object);
	Result allocateObject(TextDraw** object);
	Result allocateObject(Texture** object);
	Result allocateObject(TextureFont** object);

	Result createGraphicsQueue(Queue** queue);
	Result createComputeQueue(Queue** queue);
	Result createTransferQueue(Queue** queue);

	template <typename ObjectT, typename CreateInfoT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	Result createObject(const CreateInfoT& createInfo, ContainerT& container, ObjectT** object);

	template <typename ObjectT, typename ContainerT = std::vector<ObjPtr<ObjectT>>>
	void destroyObject(ContainerT& container, const ObjectT* object);

	template <typename ObjectT>
	void destroyAllObjects(std::vector<ObjPtr<ObjectT>>& container);

	std::optional<std::filesystem::path> getShaderPathSuffix(const std::filesystem::path& baseName);

	EngineApplication&                     m_engine;
	RenderSystem&                          m_render;

	std::vector<BufferPtr>                 m_buffers;
	std::vector<CommandBufferPtr>          m_commandBuffers;
	std::vector<CommandPoolPtr>            m_commandPools;
	std::vector<ComputePipelinePtr>        m_computePipelines;
	std::vector<DepthStencilViewPtr>       m_depthStencilViews;
	std::vector<DescriptorPoolPtr>         m_descriptorPools;
	std::vector<DescriptorSetPtr>          m_descriptorSets;
	std::vector<DescriptorSetLayoutPtr>    m_descriptorSetLayouts;
	std::vector<DrawPassPtr>               m_drawPasses;
	std::vector<FencePtr>                  m_fences;
	std::vector<ShadingRatePatternPtr>     m_shadingRatePatterns;
	std::vector<FullscreenQuadPtr>         m_fullscreenQuads;
	std::vector<GraphicsPipelinePtr>       m_graphicsPipelines;
	std::vector<ImagePtr>                  m_images;
	std::vector<MeshPtr>                   m_meshes;
	std::vector<PipelineInterfacePtr>      m_pipelineInterfaces;
	std::vector<QueryPtr>                  mQueries;
	std::vector<RenderPassPtr>             m_renderPasses;
	std::vector<RenderTargetViewPtr>       m_renderTargetViews;
	std::vector<SampledImageViewPtr>       m_sampledImageViews;
	std::vector<SamplerPtr>                m_samplers;
	std::vector<SamplerYcbcrConversionPtr> m_samplerYcbcrConversions;
	std::vector<SemaphorePtr>              m_semaphores;
	std::vector<ShaderModulePtr>           m_shaderModules;
	std::vector<StorageImageViewPtr>       m_storageImageViews;
	std::vector<TextDrawPtr>               m_textDraws;
	std::vector<TexturePtr>                m_textures;
	std::vector<TextureFontPtr>            m_textureFonts;
	std::vector<QueuePtr>                  m_graphicsQueues;
	std::vector<QueuePtr>                  m_computeQueues;
	std::vector<QueuePtr>                  m_transferQueues;

	ShadingRateCapabilities                m_shadingRateCapabilities{};

	QueuePtr                               m_graphicsQueue;
	QueuePtr                               m_computeQueue;
};

#pragma endregion

} // namespace vkr