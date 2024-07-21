#include "Base.h"
#include "Core.h"
#include "Renderer.h"
#include "EngineWindow.h"

namespace
{
	bool UseValidationLayers{ false };

	VkInstance Instance{ nullptr };
	VkDebugUtilsMessengerEXT DebugMessenger{ nullptr };

	VkPhysicalDevice PhysicalDevice{ nullptr };
	VkDevice Device{ nullptr };

	VkQueue GraphicsQueue{ nullptr };
	uint32_t GraphicsQueueFamily{ 0 };
	VkQueue PresentQueue{ nullptr };
	uint32_t PresentQueueFamily{ 0 };

	VkDeviceSize BufferMemoryAlignment{ 0 };

	VkSurfaceKHR Surface{ nullptr };
	VmaAllocator Allocator{ nullptr };

	VkSwapchainKHR SwapChain{ nullptr };
	VkFormat SwapChainImageFormat{};
	std::vector<VkImage> SwapChainImages;
	std::vector<VkImageView> SwapChainImageViews;
	VkExtent2D SwapChainExtent{};

	VkCommandPool CommandPool{ nullptr };
	std::vector<VkCommandBuffer> CommandBuffers;

	constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	size_t CurrentFrame{ 0 };
	uint32_t ImageIndex{ 0 };
	VkSemaphore ImageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore RenderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence InFlightFences[MAX_FRAMES_IN_FLIGHT];
	std::vector<VkFence> InFlightImages;

	std::vector<VkRenderPass> RenderPasses;
	std::vector<VkPipelineLayout> PipelineLayouts;
	std::vector<VkPipeline> GraphicsPipelines;
	std::vector<VkFramebuffer> Framebuffers;

	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;

	// temp: render resources
	VkRenderPass RenderPass{ nullptr };
	VkPipelineLayout PipelineLayout{ nullptr };
	VkPipeline GraphicsPipeline{ nullptr };
}

bool getQueues(vkb::Device& vkbDevice)
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

	return true;
}

bool initVulkan()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}

	{
		uint32_t version{ 0 };
		vkEnumerateInstanceVersion(&version);

		std::string text = "System can support vulkan: ";
		text += std::to_string(VK_API_VERSION_MAJOR(version));
		text += ".";
		text += std::to_string(VK_API_VERSION_MINOR(version));
		text += ".";
		text += std::to_string(VK_API_VERSION_PATCH(version));
		Print(text);

		// TODO: делать проверку на версию
	}

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder
		.request_validation_layers(UseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();
	if (!instanceRet)
	{
		Fatal(instanceRet.error().message());
		return false;
	}

	vkb::Instance vkbInstance = instanceRet.value();

	Instance = vkbInstance.instance;
	DebugMessenger = vkbInstance.debug_messenger;

	volkLoadInstanceOnly(Instance);

	result = glfwCreateWindowSurface(Instance, Window::GetWindow(), nullptr, &Surface);
	if (result != VK_SUCCESS)
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
	features10.sampleRateShading = VK_TRUE;
	features10.fillModeNonSolid = VK_TRUE;
	features10.wideLines = VK_TRUE;
	features10.depthClamp = VK_TRUE;

	vkb::PhysicalDeviceSelector physicalDeviceSelector{ vkbInstance };
	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
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

	if (!getQueues(vkbDevice))
		return false;

	// initialize the memory allocator
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

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;
		result = vmaCreateAllocator(&allocatorInfo, &Allocator);
		if (result != VK_SUCCESS)
		{
			Fatal("Cannot create allocator");
			return false;
		}
	}

	return true;
}

bool createSwapChain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapChainBuilder{ PhysicalDevice,Device,Surface };

	SwapChainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	auto swapChainRet = swapChainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = SwapChainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	if (!swapChainRet)
	{
		Fatal(swapChainRet.error().message() + " " + std::string(string_VkResult(swapChainRet.vk_result())));
		return false;
	}

	vkb::Swapchain vkbSwapchain = swapChainRet.value();

	SwapChainExtent = vkbSwapchain.extent;
	SwapChain = vkbSwapchain.swapchain;
	SwapChainImages = vkbSwapchain.get_images().value();
	SwapChainImageViews = vkbSwapchain.get_image_views().value();

	return true;
}

void destroySwapChain()
{
	if (Device && SwapChain)
	{
		vkDestroySwapchainKHR(Device, SwapChain, nullptr);
		for (int i = 0; i < SwapChainImageViews.size(); i++)
			vkDestroyImageView(Device, SwapChainImageViews[i], nullptr);
	}
	SwapChain = nullptr;
	SwapChainImageViews.clear();
}

bool createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = GraphicsQueueFamily;

	if (vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
	{
		Fatal("Failed to Create Command Pool");
		return false;
	}
	return true;
}

bool createCommandBuffers()
{
	CommandBuffers.resize(SwapChainImages.size());

	VkCommandBufferAllocateInfo AllocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	AllocateInfo.commandPool = CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = (uint32_t)SwapChainImages.size();

	if (vkAllocateCommandBuffers(Device, &AllocateInfo, CommandBuffers.data()) != VK_SUCCESS)
	{
		Fatal("Failed to Allocate Command Buffer");
		return false;
	}

	return true;
}

bool createSyncObjects()
{
	InFlightImages.resize(SwapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &ImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(Device, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(Device, &fenceInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS)
		{
			Fatal("Failed to Create Semaphores or Fence");
			return false;
		}
	}
	return true;
}

bool createRenderPass()
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = SwapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(Device, &renderPassInfo, nullptr, &RenderPass) != VK_SUCCESS)
	{
		Fatal("failed to create render pass");
		return false;
	}

	return true;
}

std::vector<char> readFile(const std::string& filename) 
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) 
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t file_size = (size_t)file.tellg();
	std::vector<char> buffer(file_size);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(file_size));

	file.close();

	return buffer;
}

VkShaderModule createShaderModule(const std::vector<char>& code) 
{
	VkShaderModuleCreateInfo create_info = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(Device, &create_info, nullptr, &shaderModule) != VK_SUCCESS) 
	{
		return VK_NULL_HANDLE; // failed to create shader module
	}

	return shaderModule;
}

bool createGraphicsPipeline()
{
	auto vertCode = readFile("Data/Shaders/vert.spv");
	auto fragCode = readFile("Data/Shaders/frag.spv");

	VkShaderModule vertModule = createShaderModule(vertCode);
	VkShaderModule fragModule = createShaderModule(fragCode);
	if (vertModule == VK_NULL_HANDLE || fragModule == VK_NULL_HANDLE) 
	{
		Fatal("failed to create shader module");
		return false;
	}

	VkPipelineShaderStageCreateInfo vertStageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStageInfo.module = vertModule;
	vertStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragStageInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStageInfo.module = fragModule;
	fragStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertStageInfo, fragStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)SwapChainExtent.width;
	viewport.height = (float)SwapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = SwapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState = { .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = { .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling = { .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending = { .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	if (vkCreatePipelineLayout(Device, &pipelineLayoutInfo, nullptr, &PipelineLayout) != VK_SUCCESS)
	{
		Fatal("failed to create pipeline layout");
		return false;
	}

	std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

	VkPipelineDynamicStateCreateInfo dynamicInfo = { .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicInfo.pDynamicStates = dynamicStates.data();

	VkGraphicsPipelineCreateInfo pipelineInfo = { .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicInfo;
	pipelineInfo.layout = PipelineLayout;
	pipelineInfo.renderPass = RenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	if (vkCreateGraphicsPipelines(Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &GraphicsPipeline) != VK_SUCCESS)
	{
		Fatal("failed to create pipline");
		return false;
	}

	vkDestroyShaderModule(Device, fragModule, nullptr);
	vkDestroyShaderModule(Device, vertModule, nullptr);

	return true;
}

bool createFramebuffers()
{
	Framebuffers.resize(SwapChainImageViews.size());

	for (size_t i = 0; i < SwapChainImageViews.size(); i++)
	{
		VkImageView attachments[] = { SwapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = RenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = SwapChainExtent.width;
		framebufferInfo.height = SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(Device, &framebufferInfo, nullptr, &Framebuffers[i]) != VK_SUCCESS)
		{
			return false;
		}
	}

	return true;
}

bool setCommandBuffers()
{
	for (size_t i = 0; i < CommandBuffers.size(); i++)
	{
		VkCommandBufferBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

		if (vkBeginCommandBuffer(CommandBuffers[i], &beginInfo) != VK_SUCCESS)
		{
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassInfo.renderPass = RenderPass;
		renderPassInfo.framebuffer = Framebuffers[i];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = SwapChainExtent;
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)SwapChainExtent.width;
		viewport.height = (float)SwapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = SwapChainExtent;

		vkCmdSetViewport(CommandBuffers[i], 0, 1, &viewport);
		vkCmdSetScissor(CommandBuffers[i], 0, 1, &scissor);

		vkCmdBeginRenderPass(CommandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(CommandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, GraphicsPipeline);

		vkCmdDraw(CommandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(CommandBuffers[i]);

		if (vkEndCommandBuffer(CommandBuffers[i]) != VK_SUCCESS)
		{
			Fatal("failed to record command buffer");
			return false;
		}
	}

	return true;
}

bool recreateSwapchain()
{
	vkDeviceWaitIdle(Device);

	vkDestroyCommandPool(Device, CommandPool, nullptr);

	for (auto framebuffer : Framebuffers)
	{
		vkDestroyFramebuffer(Device, framebuffer, nullptr);
	}

	destroySwapChain();

	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;
	if (!createFramebuffers()) return false;
	if (!createCommandPool()) return false;
	if (!createCommandBuffers()) return false;
	return true;
}


bool Renderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;
	if (!createCommandPool()) return false;
	if (!createCommandBuffers()) return false;
	if (!createSyncObjects()) return false;

	// create render resources
	if (!createRenderPass()) return false;
	if (!createGraphicsPipeline()) return false;
	if (!createFramebuffers()) return false;
	if (!setCommandBuffers()) return false;

	return true;
}

void Renderer::Close()
{
	if (Device) vkDeviceWaitIdle(Device);

	// delete resources
	{	
		for (auto framebuffer : Framebuffers)
		{
			vkDestroyFramebuffer(Device, framebuffer, nullptr);
		}
		if (GraphicsPipeline) vkDestroyPipeline(Device, GraphicsPipeline, nullptr);
		if (PipelineLayout) vkDestroyPipelineLayout(Device, PipelineLayout, nullptr);
		if (RenderPass) vkDestroyRenderPass(Device, RenderPass, nullptr);
	}

	if (Device)
	{
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(Device, RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Device, ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Device, InFlightFences[i], nullptr);
		}
	}

	if (Device && CommandPool) vkDestroyCommandPool(Device, CommandPool, nullptr);

	destroySwapChain();

	if (Allocator)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(Allocator, &stats);
		Print("Total device memory leaked: " + std::to_string(stats.total.statistics.allocationBytes) + " bytes.");
		vmaDestroyAllocator(Allocator);
		Allocator = VK_NULL_HANDLE;

	}
	if (Device) vkDestroyDevice(Device, nullptr);

	if (Instance)
	{
		if (Surface) vkDestroySurfaceKHR(Instance, Surface, nullptr);
		if (DebugMessenger) vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
		vkDestroyInstance(Instance, nullptr);
	}

	Surface = nullptr;
	Device = nullptr;
	Instance = nullptr;
	volkFinalize();
}

void Renderer::Draw()
{
	vkWaitForFences(Device, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX);

	uint32_t imageIndex = 0;
	VkResult result = vkAcquireNextImageKHR(Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		recreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Fatal("failed to acquire swapchain image. Error " + std::string(string_VkResult(result)));
		return;
	}

	if (InFlightImages[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(Device, 1, &InFlightImages[imageIndex], VK_TRUE, UINT64_MAX);
	}
	InFlightImages[imageIndex] = InFlightFences[CurrentFrame];

	VkSemaphore waitSemaphores[] = { ImageAvailableSemaphores[CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { RenderFinishedSemaphores[CurrentFrame] };

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };	
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffers[imageIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(Device, 1, &InFlightFences[CurrentFrame]);

	if (vkQueueSubmit(GraphicsQueue, 1, &submitInfo, InFlightFences[CurrentFrame]) != VK_SUCCESS)
	{
		Fatal("failed to submit draw command buffer");
		return;
	}

	VkSwapchainKHR swapChains[] = { SwapChain };

	VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapchain();
		return;
	}
	else if (result != VK_SUCCESS) 
	{
		Fatal("failed to present swapchain image");
		return;
	}

	CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VmaAllocator Renderer::GetAllocator()
{
	return Allocator;
}