#include "Base.h"
#include "Core.h"
#include "VulkanRender.h"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#pragma region VulkanRender

static std::vector<const char*> GetEnabledInstanceLayers()
{
	return { "VK_LAYER_KHRONOS_validation" };
}

static std::vector<const char*> GetEnabledInstanceExtensions()
{
	uint32_t                  numGlfwExtensions = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&numGlfwExtensions);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + numGlfwExtensions);
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	return extensions;
}

void VulkanRender::CreateInstance()
{
	// See https://github.com/KhronosGroup/Vulkan-Hpp/pull/1755
   // Currently Vulkan-Hpp doesn't check for libvulkan.1.dylib
   // Which affects vkcube installation on Apple platforms.
	VkResult err = volkInitialize();
	if (err != VK_SUCCESS) {
		/*ERR_EXIT(
			"Unable to find the Vulkan runtime on the system.\n\n"
			"This likely indicates that no Vulkan capable drivers are installed.",
			"Installation Failure");*/
	}

	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	const vk::ApplicationInfo applicationInfo("Game", VK_MAKE_VERSION(1, 0, 0), "Game", VK_MAKE_VERSION(1, 0, 0), VK_API_VERSION_1_3);

	const std::vector<const char*> enabledLayers = GetEnabledInstanceLayers();
	const std::vector<const char*> enabledExtensions = GetEnabledInstanceExtensions();

	const vk::InstanceCreateInfo
		instanceCreateInfo({}, &applicationInfo, enabledLayers.size(), enabledLayers.data(), enabledExtensions.size(), enabledExtensions.data());

	const auto [result, instance] = vk::createInstance(instanceCreateInfo);
	if(result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan instance.");
	m_instance = instance;

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
	volkLoadInstanceOnly(m_instance);

}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT        messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData
) 
{
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		Error("Vulkan: " + std::string(pCallbackData->pMessage));
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		Warning("Vulkan: " + std::string(pCallbackData->pMessage));
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		Print("Vulkan: " + std::string(pCallbackData->pMessage));
	else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		Print("Vulkan: " + std::string(pCallbackData->pMessage));
	else
	{
		Error("Vulkan debug message with unknown severity level " + std::string(pCallbackData->pMessage));
	}
	return VK_FALSE;
}

void VulkanRender::CreateDebugMessenger()
{
	const vk::DebugUtilsMessengerCreateInfoEXT createInfo(
		{},
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
		vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
		VulkanDebugCallback
	);
	const auto [result, debugMessenger] = m_instance.createDebugUtilsMessengerEXT(createInfo);
	if(result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan debug messenger.");
	m_debugMessenger = debugMessenger;
}

void VulkanRender::PickPhysicalDevice()
{
	const auto [result, physicalDevices] = m_instance.enumeratePhysicalDevices();
	if (result != vk::Result::eSuccess)
		Fatal("Failed to enumerate Vulkan physical devices.");
	if(physicalDevices.empty())
		Fatal("Can't find any Vulkan physical device.");

	// select the first discrete gpu or the first physical device
	vk::PhysicalDevice selected = physicalDevices.front();
	for (const vk::PhysicalDevice& physicalDevice : physicalDevices)
	{
		if (physicalDevice.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
		{
			selected = physicalDevice;
			break;
		}
	}
	m_physicalDevice = selected;

	const std::vector<vk::QueueFamilyProperties> queueFamilies = m_physicalDevice.getQueueFamilyProperties();
	m_graphicsQueueFamily = 0;
	for (const vk::QueueFamilyProperties& queueFamily : queueFamilies)
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			break;
		}
		m_graphicsQueueFamily++;
	}
	if(!(m_graphicsQueueFamily != queueFamilies.size()))
		Fatal("Failed to find a Vulkan graphics queue family.");
}

static std::vector<const char*> GetEnabledDeviceExtensions()
{
	return { VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };
}

void VulkanRender::CreateDevice()
{
	const float                     queuePriority = 1.0f;
	const vk::DeviceQueueCreateInfo queueCreateInfo({}, m_graphicsQueueFamily, 1, &queuePriority);

	const std::vector<const char*> enabledExtensions = GetEnabledDeviceExtensions();

	vk::PhysicalDeviceFeatures features;
	features.geometryShader = VK_TRUE;
	features.fillModeNonSolid = VK_TRUE;

	const vk::DeviceCreateInfo createInfo({}, 1, &queueCreateInfo, 0, nullptr, enabledExtensions.size(), enabledExtensions.data(), &features);
	const auto [result, device] = m_physicalDevice.createDevice(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan device.");
	m_device = device;

	m_graphicsQueue = m_device.getQueue(m_graphicsQueueFamily, 0);
	if(!m_device)
		Fatal("Failed to get Vulkan graphics queue.");

	VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
	volkLoadDevice(m_device);
}

void VulkanRender::SubmitToGraphicsQueue(const vk::SubmitInfo& submitInfo, vk::Fence fence)
{
	if(m_graphicsQueue.submit(submitInfo, fence) != vk::Result::eSuccess)
		Fatal("Failed to submit Vulkan command buffer.");
}

void VulkanRender::CreateCommandPool()
{
	const vk::CommandPoolCreateInfo createInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_graphicsQueueFamily);
	const auto [result, commandPool] = m_device.createCommandPool(createInfo);
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
	const auto [result, descriptorPool] = m_device.createDescriptorPool(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan descriptor pool.");
	m_descriptorPool = descriptorPool;
}

void VulkanRender::WriteDescriptorSet(const vk::WriteDescriptorSet& writeDescriptorSet)
{
	m_device.updateDescriptorSets(writeDescriptorSet, {});
}

void VulkanRender::CreateAllocator()
{
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

	VmaAllocatorCreateInfo createInfo{};
	createInfo.physicalDevice = m_physicalDevice;
	createInfo.device = m_device;
	createInfo.instance = m_instance;
	createInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	createInfo.pVulkanFunctions = &vmaVulkanFunc;
	if(vmaCreateAllocator(&createInfo, &m_allocator) != VK_SUCCESS)
		Fatal("Failed to create Vulkan Memory Allocator.");
}

void VulkanRender::WaitIdle()
{
	if(m_device.waitIdle() != vk::Result::eSuccess)
		Fatal("Failed when waiting for Vulkan device to be idle.");
}

vk::Fence VulkanRender::CreateFence(vk::FenceCreateFlags flags)
{
	const vk::FenceCreateInfo createInfo(flags);
	const auto [result, fence] = m_device.createFence(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan fence.");
	return fence;
}

void VulkanRender::WaitAndResetFence(vk::Fence fence, uint64_t timeout)
{
	if(m_device.waitForFences(fence, true, timeout) != vk::Result::eSuccess)
		Fatal("Failed to wait for Vulkan fence.");
	m_device.resetFences(fence);
}

vk::Semaphore VulkanRender::CreateSemaphore()
{
	const vk::SemaphoreCreateInfo createInfo;
	const auto [result, semaphore] = m_device.createSemaphore(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan semaphore.");
	return semaphore;
}

vk::CommandBuffer VulkanRender::AllocateCommandBuffer()
{
	const vk::CommandBufferAllocateInfo allocateInfo(m_commandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer                   commandBuffer;
	if(m_device.allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan command buffer.");
	return commandBuffer;
}

vk::SurfaceKHR VulkanRender::CreateSurface(GLFWwindow* window)
{
	VkSurfaceKHR surface;
	if(glfwCreateWindowSurface(m_instance, window, nullptr, &surface) != VK_SUCCESS)
		Fatal("Failed to create Vulkan surface.");
	return surface;
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
	const auto [result, swapchain] = m_device.createSwapchainKHR(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan swapchain.");
	return swapchain;
}

vk::ImageView VulkanRender::CreateImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectMask, uint32_t layers)
{
	const vk::ImageViewCreateInfo
		createInfo({}, image, layers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray, format, {}, { aspectMask, 0, 1, 0, layers });
	const auto [result, imageView] = m_device.createImageView(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan image view.");
	return imageView;
}

void VulkanRender::DestroyImageView(vk::ImageView imageView)
{
	m_device.destroyImageView(imageView);
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
	const auto [result, sampler] = m_device.createSampler(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan sampler");
	return sampler;
}

void VulkanRender::DestroySampler(vk::Sampler sampler)
{
	m_device.destroySampler(sampler);
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
	const auto [result, renderPass] = m_device.createRenderPass(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan render pass.");
	return renderPass;
}

void VulkanRender::DestroyRenderPass(vk::RenderPass renderPass)
{
	m_device.destroyRenderPass(renderPass);
}

vk::Framebuffer VulkanRender::CreateFramebuffer(
	vk::RenderPass                                    renderPass,
	const vk::ArrayProxyNoTemporaries<vk::ImageView>& attachments,
	const vk::Extent2D& extent,
	uint32_t                                          layers
)
{
	const vk::FramebufferCreateInfo createInfo({}, renderPass, attachments, extent.width, extent.height, layers);
	const auto [result, framebuffer] = m_device.createFramebuffer(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan framebuffer.");
	return framebuffer;
}

void VulkanRender::DestroyFramebuffer(vk::Framebuffer framebuffer)
{
	m_device.destroyFramebuffer(framebuffer);
}

vk::DescriptorSetLayout VulkanRender::CreateDescriptorSetLayout(const vk::ArrayProxyNoTemporaries<vk::DescriptorSetLayoutBinding>& bindings)
{
	const vk::DescriptorSetLayoutCreateInfo createInfo({}, bindings);
	const auto [result, descriptorSetLayout] = m_device.createDescriptorSetLayout(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan descriptor set layout.");
	return descriptorSetLayout;
}

void VulkanRender::DestroyDescriptorSetLayout(vk::DescriptorSetLayout descriptorSetLayout)
{
	m_device.destroyDescriptorSetLayout(descriptorSetLayout);
}

vk::DescriptorSet VulkanRender::AllocateDescriptorSet(vk::DescriptorSetLayout descriptorSetLayout)
{
	const vk::DescriptorSetAllocateInfo allocateInfo(m_descriptorPool, descriptorSetLayout);
	vk::DescriptorSet                   descriptorSet;
	if(m_device.allocateDescriptorSets(&allocateInfo, &descriptorSet) != vk::Result::eSuccess)
		Fatal("Failed to allocate Vulkan descriptor set.");
	return descriptorSet;
}

void VulkanRender::FreeDescriptorSet(vk::DescriptorSet descriptorSet)
{
	m_device.freeDescriptorSets(m_descriptorPool, descriptorSet);
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
	const auto [result, pipelineLayout] = m_device.createPipelineLayout(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan pipeline layout.");
	return pipelineLayout;
}

void VulkanRender::DestroyPipelineLayout(vk::PipelineLayout pipelineLayout)
{
	m_device.destroyPipelineLayout(pipelineLayout);
}

vk::ShaderModule VulkanRender::CreateShaderModule(const std::vector<uint32_t>& spirv)
{
	const vk::ShaderModuleCreateInfo createInfo({}, spirv.size() * sizeof(uint32_t), spirv.data());
	const auto [result, shaderModule] = m_device.createShaderModule(createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan shader module.");
	return shaderModule;
}

void VulkanRender::DestroyShaderModule(vk::ShaderModule shaderModule)
{
	m_device.destroyShaderModule(shaderModule);
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
	const auto [result, pipeline] = m_device.createGraphicsPipeline({}, createInfo);
	if (result != vk::Result::eSuccess)
		Fatal("Failed to create Vulkan graphics pipeline.");
	return pipeline;
}

void VulkanRender::DestroyPipeline(vk::Pipeline pipeline)
{
	m_device.destroyPipeline(pipeline);
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
	CreateInstance();
	CreateDebugMessenger();
	PickPhysicalDevice();
	CreateDevice();
	CreateCommandPool();
	CreateDescriptorPool();
	CreateAllocator();


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
{
	m_surface = CreateSurface(m_window);

	const auto [capabilitiesResult, capabilities] = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
	if (capabilitiesResult != vk::Result::eSuccess)
		Fatal("Failed to get Vulkan surface capabilities.");
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
	{
		imageCount = capabilities.maxImageCount;
	}
	m_swapchainExtent = CalcSwapchainExtent(capabilities, m_window);

	m_swapchain = CreateSwapchain(
		m_surface,
		imageCount,
		SURFACE_FORMAT,
		SURFACE_COLOR_SPACE,
		m_swapchainExtent,
		capabilities.currentTransform,
		PRESENT_MODE,
		m_swapchain
	);

	const auto [swapchainImagesResult, swapchainImages] = m_device.getSwapchainImagesKHR(m_swapchain);
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
		m_device.destroy(imageView);
	}
	m_swapchainImageViews.clear();
	m_device.destroy(m_swapchain);
	m_swapchain = VK_NULL_HANDLE;
	m_instance.destroy(m_surface);
	m_surface = VK_NULL_HANDLE;
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
		m_device.destroy(bufferingObject.RenderFence);
		m_device.destroy(bufferingObject.PresentSemaphore);
		m_device.destroy(bufferingObject.RenderSemaphore);
		m_device.free(m_commandPool, bufferingObject.CommandBuffer);
	}

	CleanupPrimaryFramebuffers();
	DestroyRenderPass(m_primaryRenderPass);
	CleanupSurfaceSwapchainAndImageViews();

	m_device.free(m_commandPool, m_immediateCommandBuffer);
	m_device.destroy(m_immediateFence);

	vmaDestroyAllocator(m_allocator);
	m_device.destroy(m_descriptorPool);
	m_device.destroy(m_commandPool);
	m_device.destroy();
	m_instance.destroy(m_debugMessenger);
	m_instance.destroy();
}

vk::Result VulkanRender::TryAcquiringNextSwapchainImage()
{
	const BufferingObjects& bufferingObjects = m_bufferingObjects[m_currentBufferingIndex];

	return m_device.acquireNextImageKHR(m_swapchain, 100'000'000, bufferingObjects.PresentSemaphore, nullptr, &m_currentSwapchainImageIndex);
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
	const vk::Result         result = m_graphicsQueue.presentKHR(presentInfo);
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