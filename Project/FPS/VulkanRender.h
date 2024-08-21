#pragma once

#include "RenderResources.h"
#include "RenderContext.h"

#pragma region VulkanRender

struct VulkanRenderPassOptions final
{
	bool ForPresentation = false;
	bool PreserveDepth = false;
	bool ShaderReadsDepth = false;
};

inline std::pair<vk::Viewport, vk::Rect2D> CalcViewportAndScissorFromExtent(const vk::Extent2D& extent, bool flipped = true)
{
	// flipped upside down so that it's consistent with OpenGL
	const auto width = static_cast<float>(extent.width);
	const auto height = static_cast<float>(extent.height);
	return {
		{0.0f, flipped ? height : 0, width, flipped ? -height : height, 0.0f, 1.0f},
		{{0, 0}, extent}
	};
}

struct VulkanFrameInfo final
{
	const vk::RenderPassBeginInfo* PrimaryRenderPassBeginInfo = nullptr;
	uint32_t                       BufferingIndex = 0;
	vk::CommandBuffer              CommandBuffer;
};

void BeginCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandBufferUsageFlags flags = {});
void EndCommandBuffer(vk::CommandBuffer commandBuffer);

class VulkanRender final
{
public:
	static constexpr vk::Format         SURFACE_FORMAT = vk::Format::eB8G8R8A8Unorm;
	static constexpr vk::ColorSpaceKHR  SURFACE_COLOR_SPACE = vk::ColorSpaceKHR::eSrgbNonlinear;
	static constexpr vk::PresentModeKHR PRESENT_MODE = vk::PresentModeKHR::eFifo;

	explicit VulkanRender(GLFWwindow* window);
	VulkanRender(const VulkanRender&) = delete;
	VulkanRender(VulkanRender&&) = delete;
	~VulkanRender();

	VulkanRender& operator=(const VulkanRender&) = delete;
	VulkanRender& operator=(VulkanRender&&) = delete;






















	VulkanBuffer CreateBuffer(vk::DeviceSize size, vk::BufferUsageFlags bufferUsage, VmaAllocationCreateFlags flags, VmaMemoryUsage memoryUsage)
	{
		return { Instance.Allocator, size, bufferUsage, flags, memoryUsage };
	}

	VulkanImage CreateImage(
		vk::Format               format,
		const vk::Extent2D& extent,
		vk::ImageUsageFlags      imageUsage,
		VmaAllocationCreateFlags flags,
		VmaMemoryUsage           memoryUsage,
		uint32_t                 layers = 1
	)
	{
		return { Instance.Allocator, format, extent, imageUsage, flags, memoryUsage, layers };
	}

	vk::ImageView CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layers = 1);

	void DestroyImageView(vk::ImageView imageView);

	vk::Sampler CreateSampler(
		vk::Filter             filter,
		vk::SamplerAddressMode addressMode,
		vk::Bool32             compareEnable = VK_FALSE,
		vk::CompareOp          compareOp = vk::CompareOp::eNever
	);

	void DestroySampler(vk::Sampler sampler);

	vk::RenderPass CreateRenderPass(
		const std::initializer_list<vk::Format>& colorAttachmentFormats,
		vk::Format                               depthStencilAttachmentFormat = vk::Format::eUndefined,
		const VulkanRenderPassOptions& options = {}
	);

	void DestroyRenderPass(vk::RenderPass renderPass);

	vk::Framebuffer CreateFramebuffer(
		vk::RenderPass                                    renderPass,
		const vk::ArrayProxyNoTemporaries<vk::ImageView>& attachments,
		const vk::Extent2D& extent,
		uint32_t                                          layers = 1
	);

	void DestroyFramebuffer(vk::Framebuffer framebuffer);

	vk::DescriptorSetLayout CreateDescriptorSetLayout(const vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayoutBinding>& bindings);

	void DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout);

	vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout descriptorSetLayout);

	void FreeDescriptorSet(vk::DescriptorSet descriptorSet);

	void WriteCombinedImageSamplerToDescriptorSet(vk::Sampler sampler, vk::ImageView imageView, vk::DescriptorSet descriptorSet, uint32_t binding);

	void WriteDynamicUniformBufferToDescriptorSet(vk::Buffer buffer, vk::DeviceSize size, vk::DescriptorSet descriptorSet, uint32_t binding);

	vk::PipelineLayout CreatePipelineLayout(
		const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
		const std::initializer_list<vk::PushConstantRange>& pushConstantRanges
	);

	void DestroyPipelineLayout(vk::PipelineLayout pipelineLayout);

	vk::ShaderModule CreateShaderModule(const std::vector<uint32_t>& spirv);

	void DestroyShaderModule(vk::ShaderModule shaderModule);

	vk::Pipeline CreatePipeline(
		vk::PipelineLayout                                                    pipelineLayout,
		const vk::PipelineVertexInputStateCreateInfo* vertexInput,
		const vk::ArrayProxyNoTemporaries<vk::PipelineShaderStageCreateInfo>& shaderStages,
		const VulkanPipelineOptions& options,
		const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
		vk::RenderPass                                                        renderPass,
		uint32_t                                                              subpass
	);

	void DestroyPipeline(vk::Pipeline pipeline);

	[[nodiscard]] const vk::RenderPass& GetPrimaryRenderPass() const { return m_primaryRenderPass; }
	[[nodiscard]] const vk::Extent2D& GetSwapchainExtent() const { return m_swapchainExtent; }
	[[nodiscard]] vk::Extent2D GetScaledExtent() const { return m_swapchainExtent; }
	[[nodiscard]] size_t GetNumBuffering() const { return m_bufferingObjects.size(); }

	VulkanFrameInfo BeginFrame();
	void EndFrame();

	template<class Func>
	void ImmediateSubmit(Func&& func) {
		Instance.m_immediateCommandBuffer.reset();
		BeginCommandBuffer(Instance.m_immediateCommandBuffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
		func(Instance.m_immediateCommandBuffer);
		EndCommandBuffer(Instance.m_immediateCommandBuffer);

		const vk::SubmitInfo submitInfo({}, {}, Instance.m_immediateCommandBuffer, {});
		SubmitToGraphicsQueue(submitInfo, Instance.m_immediateFence);
		WaitAndResetFence(Instance.m_immediateFence);
	}

	VulkanInstance& GetInstance() { return Instance; }

private:
	void SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence);
	void WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet);
	void WaitAndResetFence(vk::Fence fence, uint64_t timeout = 100'000'000);
	vk::Semaphore CreateSemaphore();
	vk::SwapchainKHR CreateSwapchain(
		vk::SurfaceKHR                  surface,
		uint32_t                        imageCount,
		vk::Format                      imageFormat,
		vk::ColorSpaceKHR               imageColorSpace,
		const vk::Extent2D& imageExtent,
		vk::SurfaceTransformFlagBitsKHR preTransform,
		vk::PresentModeKHR              presentMode,
		vk::SwapchainKHR                oldSwapchain
	);

	VulkanInstance Instance;

private:
	void CreateBufferingObjects();
	void CreateSurfaceSwapchainAndImageViews();
	void CreatePrimaryRenderPass();
	void CreatePrimaryFramebuffers();
	void CleanupSurfaceSwapchainAndImageViews();
	void CleanupPrimaryFramebuffers();
	void RecreateSwapchain();
	vk::Result TryAcquiringNextSwapchainImage();
	void AcquireNextSwapchainImage();

	vk::Fence CreateFence(vk::FenceCreateFlags flags = {}); // TODO: после выделения свапчаина удалить эту функцию
	vk::CommandBuffer AllocateCommandBuffer(); // TODO: после выделения свапчаина удалить эту функцию

	GLFWwindow* m_window = nullptr;

	struct BufferingObjects {
		vk::Fence         RenderFence;
		vk::Semaphore     PresentSemaphore;
		vk::Semaphore     RenderSemaphore;
		vk::CommandBuffer CommandBuffer;
	};

	std::array<BufferingObjects, 3> m_bufferingObjects;

	vk::Extent2D               m_swapchainExtent;
	vk::SwapchainKHR           m_swapchain;
	std::vector<vk::ImageView> m_swapchainImageViews;

	vk::RenderPass m_primaryRenderPass;

	std::vector<vk::Framebuffer>         m_primaryFramebuffers;
	std::vector<vk::RenderPassBeginInfo> m_primaryRenderPassBeginInfos;

	uint32_t m_currentSwapchainImageIndex = 0;
	uint32_t m_currentBufferingIndex = 0;
};



#pragma endregion
