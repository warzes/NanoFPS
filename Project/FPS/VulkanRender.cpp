#include "Base.h"
#include "Core.h"
#include "VulkanRender.h"
#include "EngineWindow.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#pragma region VulkanInstance

bool VulkanInstance::Create(const RenderContextCreateInfo& createInfo)
{
	bool useValidationLayers = createInfo.vulkan.useValidationLayers;
#if defined(_DEBUG)
	useValidationLayers = true;
#endif

	if (volkInitialize() != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder
		.set_app_name(createInfo.vulkan.appName.data())
		.set_engine_name(createInfo.vulkan.engineName.data())
		.set_app_version(createInfo.vulkan.appVersion)
		.set_engine_version(createInfo.vulkan.engineVersion)
		.require_api_version(createInfo.vulkan.requireVersion)
		.request_validation_layers(useValidationLayers)
		.use_default_debug_messenger()
		.build();
	if (!instanceRet)
	{
		Fatal(instanceRet.error().message());
		return false;
	}

	vkb::Instance vkbInstance = instanceRet.value();

	Instance = vkbInstance.instance;
	DebugMessenger = vkbInstance.debug_messenger;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(Instance));
	volkLoadInstanceOnly(Instance);

	if (glfwCreateWindowSurface(Instance, Window::GetWindow(), nullptr, &Surface) != VK_SUCCESS)
	{
		const char* errorMsg = nullptr;
		int ret = glfwGetError(&errorMsg);
		if (ret != 0)
		{
			std::string text = std::to_string(ret) + " ";
			if (errorMsg != nullptr) text += std::string(errorMsg);
			Fatal(text);
		}
		return false;
	}

	// vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	// vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	// vulkan 1.1 features
	VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };

	// vulkan 1.0 features
	VkPhysicalDeviceFeatures features10{};
	features10.samplerAnisotropy = VK_TRUE;
	features10.geometryShader = VK_TRUE;
	features10.fillModeNonSolid = VK_TRUE;

	vkb::PhysicalDeviceSelector physicalDeviceSelector{ vkbInstance };
	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.allow_any_gpu_device_type(false)
		.add_required_extension(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
		.set_required_features(features10)
		.set_surface(Surface)
		.select();
	if (!physicalDeviceRet)
	{
		Fatal(physicalDeviceRet.error().message());
		return false;
	}

	vkb::PhysicalDevice physicalDevice = physicalDeviceRet.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	auto deviceRet = deviceBuilder.build();
	if (!deviceRet)
	{
		Fatal(deviceRet.error().message());
		return false;
	}
	vkb::Device vkbDevice = deviceRet.value();

	Device = vkbDevice.device;
	PhysicalDevice = physicalDevice.physical_device;

	volkLoadDevice(Device);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(Device));

	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

	Print("GPU Used: " + std::string(PhysicalDeviceProperties.deviceName));

	if (!getQueues(vkbDevice)) return false;

	if (!createAllocator(createInfo.vulkan.requireVersion)) return false;

	temp();

	return true;
}

void VulkanInstance::Destroy()
{
	if (Allocator)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(Allocator, &stats);
		Print("Total device memory leaked: " + std::to_string(stats.total.statistics.allocationBytes) + " bytes.");
		vmaDestroyAllocator(Allocator);
		Allocator = VK_NULL_HANDLE;
	}

	if (Device)
	{
		vkDestroyDevice(Device, nullptr);
	}

	if (Instance)
	{
		if (Surface) vkDestroySurfaceKHR(Instance, Surface, nullptr);
		if (DebugMessenger) vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
		vkDestroyInstance(Instance, nullptr);
	}
	Device = nullptr;
	Allocator = nullptr;
	Surface = nullptr;
	DebugMessenger = nullptr;
	Instance = nullptr;
	volkFinalize();
}

void VulkanInstance::WaitIdle()
{
	if (Device)
	{
		if (vkDeviceWaitIdle(Device) != VK_SUCCESS)
		{
			Fatal("Failed when waiting for Vulkan device to be idle.");
		}
	}
}

bool VulkanInstance::getQueues(vkb::Device& vkbDevice)
{
	auto graphicsQueueRet = vkbDevice.get_queue(vkb::QueueType::graphics);
	if (!graphicsQueueRet)
	{
		Fatal("failed to get graphics queue: " + graphicsQueueRet.error().message());
		return false;
	}
	GraphicsQueue = graphicsQueueRet.value();

	auto graphicsQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::graphics);
	if (!graphicsQueueFamilyRet)
	{
		Fatal("failed to get graphics queue index: " + graphicsQueueFamilyRet.error().message());
		return false;
	}
	GraphicsQueueFamily = graphicsQueueFamilyRet.value();

	auto presentQueueRet = vkbDevice.get_queue(vkb::QueueType::present);
	if (!presentQueueRet)
	{
		Fatal("failed to get present queue: " + presentQueueRet.error().message());
		return false;
	}
	PresentQueue = presentQueueRet.value();

	auto presentQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::present);
	if (!presentQueueFamilyRet)
	{
		Fatal("failed to get present queue index: " + presentQueueFamilyRet.error().message());
		return false;
	}
	PresentQueueFamily = presentQueueFamilyRet.value();

	auto computeQueueRet = vkbDevice.get_queue(vkb::QueueType::compute);
	if (!computeQueueRet)
	{
		Fatal("failed to get compute queue: " + computeQueueRet.error().message());
		return false;
	}
	ComputeQueue = computeQueueRet.value();

	auto computeQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::compute);
	if (!computeQueueFamilyRet)
	{
		Fatal("failed to get compute queue index: " + computeQueueFamilyRet.error().message());
		return false;
	}
	ComputeQueueFamily = computeQueueFamilyRet.value();

	return true;
}


bool VulkanInstance::createAllocator(uint32_t vulkanApiVersion)
{
	// initialize the memory allocator
	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
	vmaVulkanFunc.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
	vmaVulkanFunc.vkAllocateMemory = vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory = vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory = vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer = vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage = vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer = vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage = vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory = vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory = vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory = vkUnmapMemory;
	vmaVulkanFunc.vkCmdCopyBuffer = vkCmdCopyBuffer;
	vmaVulkanFunc.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0/*vulkanApiVersion*/;
	allocatorInfo.physicalDevice = PhysicalDevice;
	allocatorInfo.device = Device;
	allocatorInfo.instance = Instance;
	//allocatorInfo.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

	if (vmaCreateAllocator(&allocatorInfo, &Allocator) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan Memory Allocator.");
		return false;
	}
	return true;
}


void VulkanInstance::temp()
{
	m_graphicsQueue = GraphicsQueue;
	m_physicalDevice = PhysicalDevice;
	m_device = Device;
	m_surface = Surface;

}

#pragma endregion


#pragma region VulkanRender

void VulkanRender::SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence)
{
	if(Instance.m_graphicsQueue.submit(submitInfo, fence) != vk::Result::eSuccess)
		Fatal("Failed to submit Vulkan command buffer.");
}

void VulkanRender::CreateCommandPool()
{
	const vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, Instance.GraphicsQueueFamily);
	const auto [result, commandPool] = Instance.m_device.createCommandPool(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan command pool.");
	m_commandPool = commandPool;
}

void VulkanRender::CreateDescriptorPool()
{
	const std::vector<vk::DescriptorPoolSize> poolSizes{
	{vk::DescriptorType::eSampler,              1024},
	{vk::DescriptorType::eCombinedImageSampler, 1024},
	{vk::DescriptorType::eSampledImage,         1024},
	{vk::DescriptorType::eStorageImage,         1024},
	{vk::DescriptorType::eUniformTexelBuffer,   1024},
	{vk::DescriptorType::eStorageTexelBuffer,   1024},
	{vk::DescriptorType::eUniformBuffer,        1024},
	{vk::DescriptorType::eStorageBuffer,        1024},
	{vk::DescriptorType::eUniformBufferDynamic, 1024},
	{vk::DescriptorType::eStorageBufferDynamic, 1024},
	{vk::DescriptorType::eInputAttachment,      1024}
	};
	const vk::DescriptorPoolCreateInfo createInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1024, poolSizes);
	const auto [result, descriptorPool] = Instance.m_device.createDescriptorPool(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan descriptor pool.");
	m_descriptorPool = descriptorPool;
}

void VulkanRender::WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet)
{
	Instance.m_device.updateDescriptorSets(writeDescriptorSet, {});
}

void VulkanRender::WaitIdle()
{
	if(Instance.m_device.waitIdle() != vk::Result::eSuccess)
		Fatal("Failed when waiting for Vulkan device to be idle.");
}

vk::Fence VulkanRender::CreateFence(vk::FenceCreateFlags flags)
{
	const vk::FenceCreateInfo createInfo(flags);
	const auto [result, fence] = Instance.m_device.createFence(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan fence.");
	return fence;
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

vk::CommandBuffer VulkanRender::AllocateCommandBuffer()
{
	const vk::CommandBufferAllocateInfo allocateInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer                   commandBuffer;
	if(Instance.m_device.allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan command buffer.");
	return commandBuffer;
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
	const vk::DescriptorSetAllocateInfo allocateInfo(m_descriptorPool, descriptorSetLayout);
	vk::DescriptorSet                   descriptorSet;
	if(Instance.m_device.allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan descriptor set.");
	return descriptorSet;
}

void VulkanRender::FreeDescriptorSet(vk::DescriptorSet descriptorSet)
{
	Instance.m_device.freeDescriptorSets(m_descriptorPool, descriptorSet);
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
	CreateCommandPool();
	CreateDescriptorPool();

	CreateImmediateContext();
	CreateBufferingObjects();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryRenderPass();
	CreatePrimaryFramebuffers();
}

void VulkanRender::CreateImmediateContext()
{
	m_immediateFence = CreateFence();
	m_immediateCommandBuffer = AllocateCommandBuffer();
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

	WaitIdle();

	CleanupPrimaryFramebuffers();
	CleanupSurfaceSwapchainAndImageViews();
	CreateSurfaceSwapchainAndImageViews();
	CreatePrimaryFramebuffers();
}

VulkanRender::~VulkanRender()
{
	WaitIdle();

	for (const BufferingObjects& bufferingObject : m_bufferingObjects)
	{
		Instance.m_device.destroy(bufferingObject.RenderFence);
		Instance.m_device.destroy(bufferingObject.PresentSemaphore);
		Instance.m_device.destroy(bufferingObject.RenderSemaphore);
		Instance.m_device.free(m_commandPool, bufferingObject.CommandBuffer);
	}

	CleanupPrimaryFramebuffers();
	DestroyRenderPass(m_primaryRenderPass);
	CleanupSurfaceSwapchainAndImageViews();

	Instance.m_device.free(m_commandPool, m_immediateCommandBuffer);
	Instance.m_device.destroy(m_immediateFence);

	Instance.m_device.destroy(m_descriptorPool);
	Instance.m_device.destroy(m_commandPool);
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

#pragma endregion