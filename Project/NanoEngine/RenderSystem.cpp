#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"
#include "Application.h"

#pragma region VulkanQueue

bool VulkanQueue::Init(vkb::Device& vkbDevice, vkb::QueueType type)
{
	auto queueRet = vkbDevice.get_queue(type);
	if (!queueRet.has_value())
	{
		Fatal("failed to get queue: " + queueRet.error().message());
		return false;
	}
	Queue = queueRet.value();

	auto queueFamilyRet = vkbDevice.get_queue_index(type);
	if (!queueFamilyRet.has_value())
	{
		Fatal("failed to get queue index: " + queueFamilyRet.error().message());
		return false;
	}
	QueueFamily = queueFamilyRet.value();

	return true;
}

#pragma endregion

#pragma region VulkanInstance

VulkanInstance::VulkanInstance(RenderSystem& render)
	: m_render(render)
{
}

bool VulkanInstance::Setup(const VulkanInstanceCreateInfo& createInfo, GLFWwindow* window)
{
	bool useValidationLayers = createInfo.useValidationLayers;
#if defined(_DEBUG)
	useValidationLayers = true;
#endif

	if (volkInitialize() != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder
		.set_app_name(createInfo.appName.data())
		.set_engine_name(createInfo.engineName.data())
		.set_app_version(createInfo.appVersion)
		.set_engine_version(createInfo.engineVersion)
		.require_api_version(createInfo.requireVersion)
		.request_validation_layers(useValidationLayers)
		.use_default_debug_messenger()
		.build();
	if (!instanceRet)
	{
		Fatal(instanceRet.error().message());
		return false;
	}
	vkb::Instance vkbInstance = instanceRet.value();
	instance = vkbInstance.instance;
	debugMessenger = vkbInstance.debug_messenger;

	volkLoadInstanceOnly(instance);

	surface = createSurfaceGLFW(window);
	if (surface == VK_NULL_HANDLE) return false;

	// vulkan 1.0 features
	VkPhysicalDeviceFeatures features10{};
	features10.samplerAnisotropy = VK_TRUE;
	features10.geometryShader = VK_TRUE;
	features10.fillModeNonSolid = VK_TRUE;
	// vulkan 1.1 features
	VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	// vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;
	// vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = true;
	features13.synchronization2 = true;

	std::vector<const char*> requiredExtensions =
	{
		//VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		//VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
		//VK_KHR_RAY_QUERY_EXTENSION_NAME,
		//VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
		//VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
		//VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE3_EXTENSION_NAME,
		//VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
		VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
		//VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME,
		VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
		//VK_KHR_MAINTENANCE1_EXTENSION_NAME,
		//VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME,
		//VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
		//VK_NV_MESH_SHADER_EXTENSION_NAME,
		//VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
	};

	vkb::PhysicalDeviceSelector physicalDeviceSelector{ vkbInstance };

	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.allow_any_gpu_device_type(false)
		.add_required_extensions(requiredExtensions)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
		.set_required_features(features10)
		.set_surface(surface)
		.select();
	if (!physicalDeviceRet)
	{
		Fatal(physicalDeviceRet.error().message());
		return false;
	}
	vkb::PhysicalDevice vkbPhysicalDevice = physicalDeviceRet.value();
	physicalDevice = vkbPhysicalDevice.physical_device;

	vkb::DeviceBuilder deviceBuilder{ vkbPhysicalDevice };
	auto deviceRet = deviceBuilder.build();
	if (!deviceRet)
	{
		Fatal(deviceRet.error().message());
		return false;
	}
	vkb::Device vkbDevice = deviceRet.value();
	device = vkbDevice.device;

	volkLoadDevice(device);

	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

	Print("GPU Used: " + std::string(physicalDeviceProperties.deviceName));

	if (!getQueues(vkbDevice)) return false;

	return true;
}

void VulkanInstance::Shutdown()
{
	if (device) vkDestroyDevice(device, nullptr);
	if (instance && surface) vkDestroySurfaceKHR(instance, surface, nullptr);
	if (instance && debugMessenger) vkb::destroy_debug_utils_messenger(instance, debugMessenger);
	if (instance) vkDestroyInstance(instance, nullptr);

	device = nullptr;
	surface = nullptr;
	debugMessenger = nullptr;
	instance = nullptr;
	volkFinalize();
}

VkSurfaceKHR VulkanInstance::createSurfaceGLFW(GLFWwindow* window, VkAllocationCallbacks* allocator)
{
	VkSurfaceKHR surface{ VK_NULL_HANDLE };
	VkResult err = glfwCreateWindowSurface(instance, window, allocator, &surface);
	if (err)
	{
		const char* errorMsg = nullptr;
		int ret = glfwGetError(&errorMsg);
		if (ret != 0)
		{
			std::string text = std::to_string(ret) + " ";
			if (errorMsg != nullptr) text += std::string(errorMsg);
			Fatal(text);
		}
		surface = VK_NULL_HANDLE;
	}
	return surface;
}

bool VulkanInstance::getQueues(vkb::Device& vkbDevice)
{
	if (!graphicsQueue.Init(vkbDevice, vkb::QueueType::graphics)) return false;
	if (!presentQueue.Init(vkbDevice, vkb::QueueType::present)) return false;
	if (!transferQueue.Init(vkbDevice, vkb::QueueType::transfer)) return false;
	if (!computeQueue.Init(vkbDevice, vkb::QueueType::compute)) return false;

	return true;
}

#pragma endregion

#pragma region VulkanSwapchain

VulkanSwapchain::VulkanSwapchain(RenderSystem& render)
	: m_render(render)
{
}

bool VulkanSwapchain::Setup(uint32_t width, uint32_t height)
{
	Shutdown();

	vkb::SwapchainBuilder swapChainBuilder{ m_render.GetVkPhysicalDevice(), m_render.GetVkDevice(), m_render.GetVkSurface() };

	const VkSurfaceFormatKHR surfaceFormat
	{
		.format = m_swapChainImageFormat,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	};

	auto swapChainRet = swapChainBuilder
		//.use_default_format_selection()
		.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // TODO: VK_PRESENT_MODE_MAILBOX_KHR эффективней но пока крашится
		.set_desired_extent(width, height)
		//.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build();
	if (!swapChainRet)
	{
		Fatal(swapChainRet.error().message() + " " + std::string(string_VkResult(swapChainRet.vk_result())));
		return false;
	}

	vkb::Swapchain vkbSwapchain = swapChainRet.value();

	m_swapChain = vkbSwapchain.swapchain;
	m_swapChainExtent = vkbSwapchain.extent;
	m_swapChainImages = vkbSwapchain.get_images().value();
	m_swapChainImageViews = vkbSwapchain.get_image_views().value();

	return true;
}

void VulkanSwapchain::Shutdown()
{
	if (m_render.GetVkDevice())
	{
		for (const VkImageView& imageView : m_swapChainImageViews)
		{
			if (imageView) vkDestroyImageView(m_render.GetVkDevice(), imageView, nullptr);
		}
	}
	m_swapChainImageViews.clear();

	if (m_swapChain && m_render.GetVkDevice()) vkDestroySwapchainKHR(m_render.GetVkDevice(), m_swapChain, nullptr);
	m_swapChain = nullptr;
}

VkFormat VulkanSwapchain::GetFormat()
{
	return m_swapChainImageFormat;
}

VkImageView& VulkanSwapchain::GetImageView(size_t i)
{
	assert(i < m_swapChainImageViews.size());
	return m_swapChainImageViews[i];
}

#pragma endregion

#pragma region RenderSystem

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

std::vector<VkFramebuffer> framebuffers;

VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;

std::vector<VkSemaphore> available_semaphores;
std::vector<VkSemaphore> finished_semaphore;
//std::vector<VkFence> in_flight_fences;
std::vector<VulkanFencePtr> inFlightFences;
//std::vector<VkFence> image_in_flight;
std::vector<VulkanFencePtr> imageInFlight;
size_t current_frame = 0;

const int MAX_FRAMES_IN_FLIGHT = 2;

bool createFramebuffers(VkDevice device, VulkanSwapchain& swapchain)
{
	framebuffers.resize(swapchain.GetImageViewNum());

	for (size_t i = 0; i < swapchain.GetImageViewNum(); i++)
	{
		VkImageView attachments[] = { swapchain.GetImageView(i) };

		VkFramebufferCreateInfo framebuffer_info = {};
		framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebuffer_info.renderPass = renderPass;
		framebuffer_info.attachmentCount = 1;
		framebuffer_info.pAttachments = attachments;
		framebuffer_info.width = swapchain.GetExtent().width;
		framebuffer_info.height = swapchain.GetExtent().height;
		framebuffer_info.layers = 1;

		if (vkCreateFramebuffer(device, &framebuffer_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
		{
			return false; // failed to create framebuffer
		}
	}
	return true;
}

bool createCommandPool(VkDevice device, VulkanQueue graphicsQueue)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = graphicsQueue.QueueFamily;

	if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
	{
		Fatal("failed to create command pool");
		return false; // failed to create command pool
	}
	return true;
}

bool createCommandBuffers(VkDevice device, VulkanSwapchain& swapchain)
{
	command_buffers.resize(framebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = command_pool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)command_buffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, command_buffers.data()) != VK_SUCCESS)
	{
		return false; // failed to allocate command buffers;
	}

	for (size_t i = 0; i < command_buffers.size(); i++)
	{
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(command_buffers[i], &begin_info) != VK_SUCCESS)
		{
			return false; // failed to begin recording command buffer
		}

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.renderPass = renderPass;
		render_pass_info.framebuffer = framebuffers[i];
		render_pass_info.renderArea.offset = { 0, 0 };
		render_pass_info.renderArea.extent = swapchain.GetExtent();
		VkClearValue clearColor{ { { 0.0f, 0.0f, 0.0f, 1.0f } } };
		render_pass_info.clearValueCount = 1;
		render_pass_info.pClearValues = &clearColor;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapchain.GetExtent().width;
		viewport.height = (float)swapchain.GetExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = swapchain.GetExtent();

		vkCmdSetViewport(command_buffers[i], 0, 1, &viewport);
		vkCmdSetScissor(command_buffers[i], 0, 1, &scissor);

		vkCmdBeginRenderPass(command_buffers[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdDraw(command_buffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(command_buffers[i]);

		if (vkEndCommandBuffer(command_buffers[i]) != VK_SUCCESS)
		{
			Fatal("failed to record command buffer");
			return false; // failed to record command buffer!
		}
	}
	return true;
}

VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo create_info = {};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = code.size();
	create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &create_info, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return VK_NULL_HANDLE; // failed to create shader module
	}

	return shaderModule;
}

int recreateSwapchain(VulkanSwapchain& swapchain, uint32_t width, uint32_t height, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, VulkanQueue graphicsQueue)
{
	vkDeviceWaitIdle(device);
	vkDestroyCommandPool(device, command_pool, nullptr);

	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	if (!swapchain.Setup(width, height)) return -1;
	if (!createFramebuffers(device, swapchain)) return -1;
	if (!createCommandPool(device, graphicsQueue)) return -1;
	if (!createCommandBuffers(device, swapchain)) return -1;
	return 0;
}

RenderSystem::RenderSystem(EngineApplication& engine)
	: m_engine(engine)
{
}

bool RenderSystem::Setup(const RenderCreateInfo& createInfo)
{
	if (!m_instance.Setup(createInfo.vulkan, m_engine.GetWindow().GetWindow()))
		return false;
	if (!m_swapChain.Setup(m_engine.GetWindow().GetWidth(), m_engine.GetWindow().GetHeight()))
		return false;

	//=======================================================================
	// create_render_pass
	//=======================================================================
	{
		VkAttachmentDescription color_attachment = {};
		color_attachment.format = m_swapChain.GetFormat();
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0;
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency dependency = {};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &dependency;

		if (vkCreateRenderPass(m_instance.device, &render_pass_info, nullptr, &renderPass) != VK_SUCCESS)
		{
			Fatal("failed to create render pass");
			return false; // failed to create render pass!
		}
	}

	//=======================================================================
	// create_graphics_pipeline
	//=======================================================================
	{
		auto vert_code = LoadFile("vert.spv").value();
		auto frag_code = LoadFile("frag.spv").value();

		VkShaderModule vert_module = createShaderModule(m_instance.device, vert_code);
		VkShaderModule frag_module = createShaderModule(m_instance.device, frag_code);
		if (vert_module == VK_NULL_HANDLE || frag_module == VK_NULL_HANDLE)
		{
			Fatal("failed to create shader module");
			return false; // failed to create shader modules
		}

		VkPipelineShaderStageCreateInfo vert_stage_info = {};
		vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_stage_info.module = vert_module;
		vert_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo frag_stage_info = {};
		frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_stage_info.module = frag_module;
		frag_stage_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = { vert_stage_info, frag_stage_info };

		VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.vertexBindingDescriptionCount = 0;
		vertex_input_info.vertexAttributeDescriptionCount = 0;

		VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
		input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		input_assembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)m_swapChain.GetExtent().width;
		viewport.height = (float)m_swapChain.GetExtent().height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = {};
		scissor.offset = { 0, 0 };
		scissor.extent = m_swapChain.GetExtent();

		VkPipelineViewportStateCreateInfo viewport_state = {};
		viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_state.viewportCount = 1;
		viewport_state.pViewports = &viewport;
		viewport_state.scissorCount = 1;
		viewport_state.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo color_blending = {};
		color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blending.logicOpEnable = VK_FALSE;
		color_blending.logicOp = VK_LOGIC_OP_COPY;
		color_blending.attachmentCount = 1;
		color_blending.pAttachments = &colorBlendAttachment;
		color_blending.blendConstants[0] = 0.0f;
		color_blending.blendConstants[1] = 0.0f;
		color_blending.blendConstants[2] = 0.0f;
		color_blending.blendConstants[3] = 0.0f;

		VkPipelineLayoutCreateInfo pipeline_layout_info = {};
		pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_info.setLayoutCount = 0;
		pipeline_layout_info.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(m_instance.device, &pipeline_layout_info, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			Fatal("failed to create pipeline layout");
			return -1; // failed to create pipeline layout
		}

		std::vector<VkDynamicState> dynamic_states = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamic_info = {};
		dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_info.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size());
		dynamic_info.pDynamicStates = dynamic_states.data();

		VkGraphicsPipelineCreateInfo pipeline_info = {};
		pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_info.stageCount = 2;
		pipeline_info.pStages = shader_stages;
		pipeline_info.pVertexInputState = &vertex_input_info;
		pipeline_info.pInputAssemblyState = &input_assembly;
		pipeline_info.pViewportState = &viewport_state;
		pipeline_info.pRasterizationState = &rasterizer;
		pipeline_info.pMultisampleState = &multisampling;
		pipeline_info.pColorBlendState = &color_blending;
		pipeline_info.pDynamicState = &dynamic_info;
		pipeline_info.layout = pipelineLayout;
		pipeline_info.renderPass = renderPass;
		pipeline_info.subpass = 0;
		pipeline_info.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(m_instance.device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			Fatal("failed to create pipline");
			return false; // failed to create graphics pipeline
		}

		vkDestroyShaderModule(m_instance.device, frag_module, nullptr);
		vkDestroyShaderModule(m_instance.device, vert_module, nullptr);
	}

	//=======================================================================
	// create_framebuffers
	//=======================================================================
	if (!createFramebuffers(m_instance.device, m_swapChain))
		return false;

	//=======================================================================
	// create_command_pool
	//=======================================================================
	if (!createCommandPool(m_instance.device, m_instance.graphicsQueue))
		return false;

	//=======================================================================
	// create_command_buffers
	//=======================================================================
	if (!createCommandBuffers(m_instance.device, m_swapChain))
		return false;

	//=======================================================================
	// create_sync_objects
	//=======================================================================
	{
		available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		finished_semaphore.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imageInFlight.resize(m_swapChain.GetImageNum(), nullptr);

		VkSemaphoreCreateInfo semaphore_info = {};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			inFlightFences[i] = m_device.CreateFence({ .signaled = true });

			if (vkCreateSemaphore(m_instance.device, &semaphore_info, nullptr, &available_semaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(m_instance.device, &semaphore_info, nullptr, &finished_semaphore[i]) != VK_SUCCESS ||
				inFlightFences[i] == nullptr)
			{
				Fatal("failed to create sync objects");
				return false; // failed to create synchronization objects for a frame
			}
		}
	}

	return true;
}

void RenderSystem::Shutdown()
{
	vkDeviceWaitIdle(m_instance.device);

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_instance.device, finished_semaphore[i], nullptr);
		vkDestroySemaphore(m_instance.device, available_semaphores[i], nullptr);
		m_device.DestroyFence(inFlightFences[i]);
	}

	vkDestroyCommandPool(m_instance.device, command_pool, nullptr);

	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(m_instance.device, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_instance.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_instance.device, pipelineLayout, nullptr);
	vkDestroyRenderPass(m_instance.device, renderPass, nullptr);

	m_swapChain.Shutdown();
	m_instance.Shutdown();	
}

void RenderSystem::TestDraw()
{
	inFlightFences[current_frame]->Wait();

	uint32_t image_index = UINT32_MAX;
	VkResult result = vkAcquireNextImageKHR(m_instance.device, m_swapChain.Get(), UINT64_MAX, available_semaphores[current_frame], VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		if (recreateSwapchain(m_swapChain, m_engine.GetWindow().GetWidth(), m_engine.GetWindow().GetHeight(), m_instance.physicalDevice, m_instance.device, m_instance.surface, m_instance.graphicsQueue) != 0)
			Fatal("recreateSwapchain failed");
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) 
	{
		Fatal("failed to acquire swapchain image. Error " + result);
		return;
	}

	if (imageInFlight[image_index] != nullptr)
	{
		imageInFlight[image_index]->Wait();
	}
	imageInFlight[image_index] = inFlightFences[current_frame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore wait_semaphores[] = { available_semaphores[current_frame] };
	VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = wait_semaphores;
	submitInfo.pWaitDstStageMask = wait_stages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &command_buffers[image_index];

	VkSemaphore signal_semaphores[] = { finished_semaphore[current_frame] };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signal_semaphores;

	inFlightFences[current_frame]->Reset();

	if (vkQueueSubmit(m_instance.graphicsQueue.Queue, 1, &submitInfo, inFlightFences[current_frame]->Get()) != VK_SUCCESS)
	{
		Fatal("failed to submit draw command buffer");
		return; //"failed to submit draw command buffer
	}

	VkPresentInfoKHR present_info = {};
	present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	present_info.waitSemaphoreCount = 1;
	present_info.pWaitSemaphores = signal_semaphores;

	VkSwapchainKHR swapChains[] = { m_swapChain.Get()};
	present_info.swapchainCount = 1;
	present_info.pSwapchains = swapChains;

	present_info.pImageIndices = &image_index;

	result = vkQueuePresentKHR(m_instance.presentQueue.Queue, &present_info);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		if (recreateSwapchain(m_swapChain, m_engine.GetWindow().GetWidth(), m_engine.GetWindow().GetHeight(), m_instance.physicalDevice, m_instance.device, m_instance.surface, m_instance.graphicsQueue) != 0)
			Fatal("recreateSwapchain failed");
		return;
	}
	else if (result != VK_SUCCESS)
	{
		Fatal("failed to present swapchain image");
		return ;
	}

	current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

#pragma endregion