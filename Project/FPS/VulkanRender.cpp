#include "Base.h"
#include "Core.h"
#include "VulkanRender.h"
#include "EngineWindow.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#pragma region VulkanRender

void VulkanRender::SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence)
{
	if(Instance.m_graphicsQueue.submit(submitInfo, fence) != vk::Result::eSuccess)
		Fatal("Failed to submit Vulkan command buffer.");
}

void VulkanRender::WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet)
{
	Instance.m_device.updateDescriptorSets(writeDescriptorSet, {});
}

void VulkanRender::WaitAndResetFence(vk::Fence fence, uint64_t timeout)
{
	if(Instance.m_device.waitForFences(fence, true, timeout) != vk::Result::eSuccess)
		Fatal("Failed to wait for Vulkan fence.");
	Instance.m_device.resetFences(fence);
}

vk::Semaphore VulkanRender::CreateSemaphore()
{
	const vk::SemaphoreCreateInfo createInfo;
	const auto [result, semaphore] = Instance.m_device.createSemaphore(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan semaphore.");
	return semaphore;
}

vk::SwapchainKHR VulkanRender::CreateSwapchain(
	vk::SurfaceKHR                  surface,
	uint32_t                        imageCount,
	vk::Format                      imageFormat,
	vk::ColorSpaceKHR               imageColorSpace,
	const vk::Extent2D& imageExtent,
	vk::SurfaceTransformFlagBitsKHR preTransform,
	vk::PresentModeKHR              presentMode,
	vk::SwapchainKHR                oldSwapchain
)
{
	const vk::SwapchainCreateInfoKHR createInfo(
		{},
		surface,
		imageCount,
		imageFormat,
		imageColorSpace,
		imageExtent,
		1,
		vk::ImageUsageFlagBits::eColorAttachment,
		vk::SharingMode::eExclusive,
		{},
		preTransform,
		vk::CompositeAlphaFlagBitsKHR::eOpaque,
		presentMode,
		VK_TRUE,
		oldSwapchain,
		nullptr
	);
	const auto [result, swapchain] = Instance.m_device.createSwapchainKHR(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan swapchain.");
	return swapchain;
}

vk::ImageView VulkanRender::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layers)
{
	const vk::ImageViewCreateInfo
		createInfo({}, image, layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray, format, {}, { aspectMask, 0, 1, 0, layers });
	const auto [result, imageView] = Instance.m_device.createImageView(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan image view.");
	return imageView;
}

void VulkanRender::DestroyImageView(vk::ImageView imageView)
{
	Instance.m_device.destroyImageView(imageView);
}

vk::Sampler VulkanRender::CreateSampler(vk::Filter filter, vk::SamplerAddressMode addressMode, vk::Bool32 compareEnable, vk::CompareOp compareOp)
{
	const vk::SamplerCreateInfo createInfo(
		{},
		filter,
		filter,
		{},
		addressMode,
		addressMode,
		addressMode,
		0.0f,
		VK_FALSE,
		0.0f,
		compareEnable,
		compareOp,
		0.0f,
		0.0f,
		vk::BorderColor::eFloatTransparentBlack,
		{}
	);
	const auto [result, sampler] = Instance.m_device.createSampler(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan sampler");
	return sampler;
}

void VulkanRender::DestroySampler(vk::Sampler sampler)
{
	Instance.m_device.destroySampler(sampler);
}

vk::RenderPass VulkanRender::CreateRenderPass(
	const std::initializer_list<vk::Format>& colorAttachmentFormats,
	vk::Format                               depthStencilAttachmentFormat,
	const VulkanRenderPassOptions& options
)
{
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::AttachmentReference>   colorAttachmentRefs;
	vk::AttachmentReference                depthStencilAttachmentRef;

	uint32_t attachmentIndex = 0;
	for (const vk::Format& colorAttachmentFormat : colorAttachmentFormats) {
		attachments.emplace_back(
			vk::AttachmentDescriptionFlags{},
			colorAttachmentFormat,
			vk::SampleCountFlagBits::e1,
			vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			vk::ImageLayout::eUndefined,
			options.ForPresentation ? vk::ImageLayout::ePresentSrcKHR : vk::ImageLayout::eShaderReadOnlyOptimal
		);

		colorAttachmentRefs.emplace_back(attachmentIndex++, vk::ImageLayout::eColorAttachmentOptimal);
	}

	if (depthStencilAttachmentFormat != vk::Format::eUndefined)
	{
		attachments.emplace_back(
			vk::AttachmentDescriptionFlags{},
			depthStencilAttachmentFormat,
			vk::SampleCountFlagBits::e1,
			options.PreserveDepth ? vk::AttachmentLoadOp::eLoad : vk::AttachmentLoadOp::eClear,
			vk::AttachmentStoreOp::eStore,
			vk::AttachmentLoadOp::eDontCare,
			vk::AttachmentStoreOp::eDontCare,
			options.PreserveDepth ? vk::ImageLayout::eDepthStencilAttachmentOptimal : vk::ImageLayout::eUndefined,
			options.ShaderReadsDepth ? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eDepthStencilAttachmentOptimal
		);

		depthStencilAttachmentRef = { attachmentIndex, vk::ImageLayout::eDepthStencilAttachmentOptimal };
	}

	vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, {}, colorAttachmentRefs);

	if (depthStencilAttachmentFormat != vk::Format::eUndefined) {
		subpass.setPDepthStencilAttachment(&depthStencilAttachmentRef);
	}

	const vk::RenderPassCreateInfo createInfo({}, attachments, subpass);
	const auto [result, renderPass] = Instance.m_device.createRenderPass(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan render pass.");
	return renderPass;
}

void VulkanRender::DestroyRenderPass(vk::RenderPass renderPass)
{
	Instance.m_device.destroyRenderPass(renderPass);
}

vk::Framebuffer VulkanRender::CreateFramebuffer(
	vk::RenderPass                                    renderPass,
	const vk::ArrayProxyNoTemporaries<vk::ImageView>& attachments,
	const vk::Extent2D& extent,
	uint32_t                                          layers
)
{
	const vk::FramebufferCreateInfo createInfo({}, renderPass, attachments, extent.width, extent.height, layers);
	const auto [result, framebuffer] = Instance.m_device.createFramebuffer(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan framebuffer.");
	return framebuffer;
}

void VulkanRender::DestroyFramebuffer(vk::Framebuffer framebuffer)
{
	Instance.m_device.destroyFramebuffer(framebuffer);
}

vk::DescriptorSetLayout VulkanRender::CreateDescriptorSetLayout(const vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayoutBinding>& bindings)
{
	const vk::DescriptorSetLayoutCreateInfo createInfo({}, bindings);
	const auto [result, descriptorSetLayout] = Instance.m_device.createDescriptorSetLayout(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan descriptor set layout.");
	return descriptorSetLayout;
}

void VulkanRender::DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout)
{
	Instance.m_device.destroyDescriptorSetLayout(descriptorSetLayout);
}

vk::DescriptorSet VulkanRender::AllocateDescriptorSet(vk::DescriptorSetLayout descriptorSetLayout)
{
	const vk::DescriptorSetAllocateInfo allocateInfo(Instance.m_descriptorPool, descriptorSetLayout);
	vk::DescriptorSet                   descriptorSet;
	if(Instance.m_device.allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan descriptor set.");
	return descriptorSet;
}

void VulkanRender::FreeDescriptorSet(vk::DescriptorSet descriptorSet)
{
	Instance.m_device.freeDescriptorSets(Instance.m_descriptorPool, descriptorSet);
}

void VulkanRender::WriteCombinedImageSamplerToDescriptorSet(
	vk::Sampler       sampler,
	vk::ImageView     imageView,
	vk::DescriptorSet descriptorSet,
	uint32_t          binding
)
{
	const vk::DescriptorImageInfo imageInfo(sampler, imageView, vk::ImageLayout::eShaderReadOnlyOptimal);
	const vk::WriteDescriptorSet  writeDescriptorSet(descriptorSet, binding, 0, vk::DescriptorType::eCombinedImageSampler, imageInfo);
	WriteDescriptorSet(writeDescriptorSet);
}

void VulkanRender::WriteDynamicUniformBufferToDescriptorSet(
	vk::Buffer        buffer,
	vk::DeviceSize    size,
	vk::DescriptorSet descriptorSet,
	uint32_t          binding
)
{
	const vk::DescriptorBufferInfo bufferInfo(buffer, 0, size);
	const vk::WriteDescriptorSet   writeDescriptorSet(descriptorSet, binding, 0, vk::DescriptorType::eUniformBufferDynamic, {}, bufferInfo);
	WriteDescriptorSet(writeDescriptorSet);
}

vk::PipelineLayout VulkanRender::CreatePipelineLayout(
	const std::initializer_list<vk::DescriptorSetLayout>& descriptorSetLayouts,
	const std::initializer_list<vk::PushConstantRange>& pushConstantRanges
)
{
	const vk::PipelineLayoutCreateInfo createInfo({}, descriptorSetLayouts, pushConstantRanges);
	const auto [result, pipelineLayout] = Instance.m_device.createPipelineLayout(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan pipeline layout.");
	return pipelineLayout;
}

void VulkanRender::DestroyPipelineLayout(vk::PipelineLayout pipelineLayout)
{
	Instance.m_device.destroyPipelineLayout(pipelineLayout);
}

vk::ShaderModule VulkanRender::CreateShaderModule(const std::vector<uint32_t>& spirv)
{
	const vk::ShaderModuleCreateInfo createInfo({}, spirv.size() * sizeof(uint32_t), spirv.data());
	const auto [result, shaderModule] = Instance.m_device.createShaderModule(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan shader module.");
	return shaderModule;
}

void VulkanRender::DestroyShaderModule(vk::ShaderModule shaderModule)
{
	Instance.m_device.destroyShaderModule(shaderModule);
}

vk::Pipeline VulkanRender::CreatePipeline(
	vk::PipelineLayout                                                    pipelineLayout,
	const vk::PipelineVertexInputStateCreateInfo* vertexInput,
	const vk::ArrayProxyNoTemporaries<vk::PipelineShaderStageCreateInfo>& shaderStages,
	const VulkanPipelineOptions& options,
	const std::initializer_list<vk::PipelineColorBlendAttachmentState>& attachmentColorBlends,
	vk::RenderPass                                                        renderPass,
	uint32_t                                                              subpass
)
{
	const vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState({}, options.Topology);

	const vk::PipelineViewportStateCreateInfo viewportState({}, 1, nullptr, 1, nullptr);

	const vk::PipelineRasterizationStateCreateInfo rasterizationState(
		{},
		VK_FALSE,
		VK_FALSE,
		options.PolygonMode,
		options.CullMode,
		vk::FrontFace::eCounterClockwise,
		VK_FALSE,
		0.0f,
		0.0f,
		0.0f,
		1.0f
	);

	const vk::PipelineMultisampleStateCreateInfo multisampleState({}, vk::SampleCountFlagBits::e1);

	const vk::PipelineDepthStencilStateCreateInfo depthStencilState({}, options.DepthTestEnable, options.DepthWriteEnable, options.DepthCompareOp);

	const vk::PipelineColorBlendStateCreateInfo colorBlendState({}, VK_FALSE, vk::LogicOp::eCopy, attachmentColorBlends);

	const vk::DynamicState                   dynamicStates[] = { vk::DynamicState::eViewport, vk::DynamicState::eScissor };
	const vk::PipelineDynamicStateCreateInfo dynamicState({}, dynamicStates);

	const vk::GraphicsPipelineCreateInfo createInfo(
		{},
		shaderStages,
		vertexInput,
		&inputAssemblyState,
		nullptr,
		&viewportState,
		&rasterizationState,
		&multisampleState,
		&depthStencilState,
		&colorBlendState,
		&dynamicState,
		pipelineLayout,
		renderPass,
		subpass
	);
	const auto [result, pipeline] = Instance.m_device.createGraphicsPipeline({}, createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan graphics pipeline.");
	return pipeline;
}

void VulkanRender::DestroyPipeline(vk::Pipeline pipeline)
{
	Instance.m_device.destroyPipeline(pipeline);
}

void BeginCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandBufferUsageFlags flags)
{
	if(commandBuffer.begin({ flags })!= vk::Result::eSuccess)
		Fatal("Failed to begin Vulkan command buffer.");
}

void EndCommandBuffer(vk::CommandBuffer commandBuffer)
{
	if(commandBuffer.end() != vk::Result::eSuccess)
		Fatal("Failed to end Vulkan command buffer.");
}

static vk::Extent2D CalcSwapchainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max() &&
		capabilities.currentExtent.height == std::numeric_limits<uint32_t>::max()) {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const vk::Extent2D extent(
			std::clamp(static_cast<uint32_t>(width), capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(static_cast<uint32_t>(height), capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		);
		return extent;
	}
	return capabilities.currentExtent;
}

VulkanRender::VulkanRender(GLFWwindow* window)
	: m_window(window)
{
	if (!Instance.Create({}))
	{

	}
	CreateBufferingObjects();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryRenderPass();
	CreatePrimaryFramebuffers();
}

void VulkanRender::CreateBufferingObjects()
{
	for (BufferingObjects& bufferingObject : m_bufferingObjects)
	{
		bufferingObject.RenderFence = CreateFence(vk::FenceCreateFlagBits::eSignaled);
		bufferingObject.PresentSemaphore = CreateSemaphore();
		bufferingObject.RenderSemaphore = CreateSemaphore();
		bufferingObject.CommandBuffer = AllocateCommandBuffer();
	}
}

void VulkanRender::CreateSurfaceSwapchainAndImageViews()
{	const auto [capabilitiesResult, capabilities] = Instance.m_physicalDevice.getSurfaceCapabilitiesKHR(Instance.m_surface);
	if (capabilitiesResult != vk::Result::eSuccess)
		Fatal("Failed to get Vulkan surface capabilities.");
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}
	m_swapchainExtent = CalcSwapchainExtent(capabilities, m_window);

	m_swapchain = CreateSwapchain(
		Instance.m_surface,
		imageCount,
		SURFACE_FORMAT,
		SURFACE_COLOR_SPACE,
		m_swapchainExtent,
		capabilities.currentTransform,
		PRESENT_MODE,
		m_swapchain
	);

	const auto [swapchainImagesResult, swapchainImages] = Instance.m_device.getSwapchainImagesKHR(m_swapchain);
	if (swapchainImagesResult != vk::Result::eSuccess)
		Fatal("Failed to get Vulkan swapchain images.");
	for (const vk::Image& image : swapchainImages)
	{
		m_swapchainImageViews.push_back(CreateImageView(image, SURFACE_FORMAT, vk::ImageAspectFlagBits::eColor));
	}
}

void VulkanRender::CreatePrimaryRenderPass()
{
	VulkanRenderPassOptions options;
	options.ForPresentation = true;

	m_primaryRenderPass = CreateRenderPass(
		{ SURFACE_FORMAT }, //
		vk::Format::eUndefined,
		options
	);
}

void VulkanRender::CreatePrimaryFramebuffers()
{
	const size_t numSwapchainImages = m_swapchainImageViews.size();
	for (int i = 0; i < numSwapchainImages; i++)
	{
		m_primaryFramebuffers.push_back(CreateFramebuffer(m_primaryRenderPass, m_swapchainImageViews[i], m_swapchainExtent));
	}

	const vk::Rect2D renderArea{
		{0, 0},
		m_swapchainExtent
	};
	static const vk::ClearValue clearValues{ vk::ClearColorValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}} };

	vk::RenderPassBeginInfo beginInfo(m_primaryRenderPass, {}, renderArea, clearValues);
	for (const vk::Framebuffer& framebuffer : m_primaryFramebuffers)
	{
		beginInfo.framebuffer = framebuffer;
		m_primaryRenderPassBeginInfos.push_back(beginInfo);
	}
}

void VulkanRender::CleanupSurfaceSwapchainAndImageViews()
{
	for (const vk::ImageView& imageView : m_swapchainImageViews)
	{
		Instance.m_device.destroy(imageView);
	}
	m_swapchainImageViews.clear();
	Instance.m_device.destroy(m_swapchain);
	m_swapchain = VK_NULL_HANDLE;
}

void VulkanRender::CleanupPrimaryFramebuffers()
{
	m_primaryRenderPassBeginInfos.clear();
	for (const vk::Framebuffer& framebuffer : m_primaryFramebuffers)
	{
		DestroyFramebuffer(framebuffer);
	}
	m_primaryFramebuffers.clear();
}

void VulkanRender::RecreateSwapchain()
{
	// block when minimized
	while (true)
	{
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		if (width != 0 && height != 0)
		{
			break;
		}
		glfwWaitEvents();
	}

	Instance.WaitIdle();

	CleanupPrimaryFramebuffers();
	CleanupSurfaceSwapchainAndImageViews();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryFramebuffers();
}

VulkanRender::~VulkanRender()
{
	Instance.WaitIdle();

	for (const BufferingObjects& bufferingObject : m_bufferingObjects)
	{
		Instance.m_device.destroy(bufferingObject.RenderFence);
		Instance.m_device.destroy(bufferingObject.PresentSemaphore);
		Instance.m_device.destroy(bufferingObject.RenderSemaphore);
		Instance.m_device.free(Instance.m_commandPool, bufferingObject.CommandBuffer);
	}

	CleanupPrimaryFramebuffers();
	DestroyRenderPass(m_primaryRenderPass);
	CleanupSurfaceSwapchainAndImageViews();

	Instance.Destroy();
}

vk::Result VulkanRender::TryAcquiringNextSwapchainImage()
{
	const BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	return Instance.m_device.acquireNextImageKHR(m_swapchain, 100'000'000, bufferingObjects.PresentSemaphore, nullptr, &m_currentSwapchainImageIndex);
}

void VulkanRender::AcquireNextSwapchainImage()
{
	vk::Result result = TryAcquiringNextSwapchainImage();
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR)
	{
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			// recreate swapchain and try acquiring again
			RecreateSwapchain();
			result = TryAcquiringNextSwapchainImage();
		}

		if (result != vk::Result::eSuccess)
			Fatal("Failed to acquire next Vulkan swapchain image.");
	}
}

VulkanFrameInfo VulkanRender::BeginFrame()
{
	const BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	WaitAndResetFence(bufferingObjects.RenderFence);

	AcquireNextSwapchainImage();

	bufferingObjects.CommandBuffer.reset();
	BeginCommandBuffer(bufferingObjects.CommandBuffer, vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

	return { &m_primaryRenderPassBeginInfos[m_currentSwapchainImageIndex], m_currentBufferingIndex, bufferingObjects.CommandBuffer };
}

void VulkanRender::EndFrame()
{
	BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	EndCommandBuffer(bufferingObjects.CommandBuffer);

	vk::PipelineStageFlags waitStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
	const vk::SubmitInfo   submitInfo(bufferingObjects.PresentSemaphore, waitStage, bufferingObjects.CommandBuffer, bufferingObjects.RenderSemaphore);
	SubmitToGraphicsQueue(submitInfo, bufferingObjects.RenderFence);

	const vk::PresentInfoKHR presentInfo(bufferingObjects.RenderSemaphore, m_swapchain, m_currentSwapchainImageIndex);
	const vk::Result         result = Instance.m_graphicsQueue.presentKHR(presentInfo);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
	{
		RecreateSwapchain();
	}
	else
	{
		if (result != vk::Result::eSuccess)
			Fatal("Failed to present Vulkan swapchain image.");
	}

	m_currentBufferingIndex = (m_currentBufferingIndex + 1) % m_bufferingObjects.size();
}

vk::CommandBuffer VulkanRender::AllocateCommandBuffer()
{
	const vk::CommandBufferAllocateInfo allocateInfo(Instance.m_commandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer                   commandBuffer;
	if (Instance.m_device.allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan command buffer.");
	return commandBuffer;
}

vk::Fence VulkanRender::CreateFence(vk::FenceCreateFlags flags)
{
	const vk::FenceCreateInfo createInfo(flags);
	const auto [result, fence] = Instance.m_device.createFence(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan fence.");
	return fence;
}

#pragma endregion