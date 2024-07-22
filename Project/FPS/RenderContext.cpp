#include "Base.h"
#include "Core.h"
#include "RenderContext.h"
#include "EngineWindow.h"

namespace
{
	VulkanInstance Instance;
	VulkanSwapChain SwapChain;
}

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

	vkb::PhysicalDeviceSelector physicalDeviceSelector{ vkbInstance };
	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.allow_any_gpu_device_type(false)
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

	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);

	Print("GPU Used: " + std::string(PhysicalDeviceProperties.deviceName));

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
		allocatorInfo.vulkanApiVersion = createInfo.vulkan.requireVersion;
		allocatorInfo.physicalDevice = PhysicalDevice;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		//allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

		if (vmaCreateAllocator(&allocatorInfo, &Allocator) != VK_SUCCESS)
		{
			Fatal("Failed to create Vulkan Memory Allocator.");
			return false;
		}
	}

	if (!createCommandPool()) return false;

	return true;
}

void VulkanInstance::Destroy()
{
	if (Device)
	{
		if (CommandPool) vkDestroyCommandPool(Device, CommandPool, nullptr);

		vkDestroyDevice(Device, nullptr);
	}

	if (Allocator)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(Allocator, &stats);
		Print("Total device memory leaked: " + std::to_string(stats.total.statistics.allocationBytes) + " bytes.");
		vmaDestroyAllocator(Allocator);
		Allocator = VK_NULL_HANDLE;
	}

	if (Instance)
	{
		if (Surface) vkDestroySurfaceKHR(Instance, Surface, nullptr);
		if (DebugMessenger) vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
		vkDestroyInstance(Instance, nullptr);
	}
	CommandPool = nullptr;
	Device = nullptr;
	Allocator = nullptr;
	Surface = nullptr;
	DebugMessenger = nullptr;
	Instance = nullptr;
	volkFinalize();
}

void VulkanInstance::Finish()
{
	if (Device) vkDeviceWaitIdle(Device);
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

bool VulkanInstance::createCommandPool()
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

#pragma endregion

#pragma region VulkanSwapChain

bool VulkanSwapChain::Create()
{
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;

	if (!createCommandBuffers()) return false;
	if (!createSyncObjects()) return false;
	if (!createRenderPass()) return false;
	if (!createDepthResources()) return false;
	if (!createFramebuffers()) return false;

	return true;
}

void VulkanSwapChain::Destroy()
{
	destroySwapChain();
	if (Instance.Device)
	{
		if (Instance.Device && MainRenderPass) vkDestroyRenderPass(Instance.Device, MainRenderPass, nullptr);
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(Instance.Device, RenderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(Instance.Device, ImageAvailableSemaphores[i], nullptr);
			vkDestroyFence(Instance.Device, InFlightFences[i], nullptr);
		}
	}
	
	MainRenderPass = nullptr;
}

bool VulkanSwapChain::Resize()
{
	// TODO: случай, если приложение свернуто.
	{
		int width = 0, height = 0; 
		glfwGetFramebufferSize(Window::GetWindow(), &width, &height);
		while (width == 0 || height == 0)
		{
			glfwGetFramebufferSize(Window::GetWindow(), &width, &height);
			glfwWaitEvents();
		}
	}

	vkDeviceWaitIdle(Instance.Device);

	destroySwapChain();

	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;
	if (!createDepthResources()) return false;
	if (!createFramebuffers()) return false;
	return true;
}

void VulkanSwapChain::BeginFrame()
{
	vkWaitForFences(Instance.Device, 1, &InFlightFences[CurrentFrame], VK_TRUE, UINT64_MAX); // ожидание пред. кадра

	VkResult result = vkAcquireNextImageKHR(Instance.Device, SwapChain, UINT64_MAX, ImageAvailableSemaphores[CurrentFrame], VK_NULL_HANDLE, &m_imageIndex); // получение изображения из SwapChain
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		Resize();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		Fatal("failed to acquire swap chain image!. Error " + std::string(string_VkResult(result)));
		return;
	}

	vkResetFences(Instance.Device, 1, &InFlightFences[CurrentFrame]);
	vkResetCommandBuffer(CommandBuffers[CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
}

void VulkanSwapChain::EndFrame()
{
	VkSemaphore waitSemaphores[] = { ImageAvailableSemaphores[CurrentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { RenderFinishedSemaphores[CurrentFrame] };

	VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CommandBuffers[CurrentFrame];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(Instance.GraphicsQueue, 1, &submitInfo, InFlightFences[CurrentFrame]) != VK_SUCCESS)
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
	presentInfo.pImageIndices = &m_imageIndex;

	VkResult result = vkQueuePresentKHR(Instance.PresentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || FramebufferResized)
	{
		FramebufferResized = false;
		Resize();
	}
	else if (result != VK_SUCCESS)
	{
		Fatal("failed to present swap chain image!");
		return;
	}

	CurrentFrame = (CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

bool VulkanSwapChain::createSwapChain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapChainBuilder{ Instance.PhysicalDevice, Instance.Device, Instance.Surface };

	VkSurfaceFormatKHR surfaceFormat
	{
		.format = SwapChainImageFormat,
		.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
	};

	auto swapChainRet = swapChainBuilder
		//.use_default_format_selection()
		.set_desired_format(surfaceFormat)
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // TODO: VK_PRESENT_MODE_MAILBOX_KHR эффективней но пока крашится
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

bool VulkanSwapChain::createDepthResources()
{
	auto depthFormat = findDepthFormat();
	if (!depthFormat)
	{
		Fatal("failed to find supported format!");
		return false;
	}

	if (!createImage(SwapChainExtent.width, SwapChainExtent.height, 1, depthFormat.value(), VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, DepthImage, DepthImageMemory))
		return false;

	auto imageView = createImageView(DepthImage, depthFormat.value(), VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	if (!imageView) return false;

	DepthImageView = imageView.value();

	return true;
}

std::optional<VkImageView> VulkanSwapChain::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
	VkImageViewCreateInfo viewInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(Instance.Device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
	{
		Fatal("failed to create texture image view!");
		return std::nullopt;
	}

	return imageView;
}

std::optional<VkFormat> VulkanSwapChain::findDepthFormat()
{
	const std::vector<VkFormat> candidates = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
	const VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

	for (VkFormat format : candidates) 
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(Instance.PhysicalDevice, format, &props);

		if ((props.optimalTilingFeatures & features) == features)
			return format;
	}
	return std::nullopt;
}

bool VulkanSwapChain::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo{ .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(Instance.Device, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		Fatal("failed to create image!");
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(Instance.Device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{ .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(Instance.Device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		Fatal("failed to allocate image memory!");
		return false;
	}

	vkBindImageMemory(Instance.Device, image, imageMemory, 0);

	return true;
}

uint32_t VulkanSwapChain::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(Instance.PhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	Fatal("failed to find suitable memory type!");
	return 0;
}

bool VulkanSwapChain::createFramebuffers()
{
	SwapChainFramebuffers.resize(SwapChainImageViews.size());

	for (size_t i = 0; i < SwapChainImageViews.size(); i++)
	{
		std::array<VkImageView, 2> attachments = {
			SwapChainImageViews[i],
			DepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = { .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
		framebufferInfo.renderPass = MainRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = SwapChainExtent.width;
		framebufferInfo.height = SwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(Instance.Device, &framebufferInfo, nullptr, &SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			Fatal("failed to create framebuffer!");
			return false;
		}
	}

	return true;
}

bool VulkanSwapChain::createRenderPass()
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

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat().value();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(Instance.Device, &renderPassInfo, nullptr, &MainRenderPass) != VK_SUCCESS)
	{
		Fatal("failed to create render pass");
		return false;
	}

	return true;
}

void VulkanSwapChain::destroySwapChain()
{
	if (Instance.Device)
	{
		if (DepthImageView) vkDestroyImageView(Instance.Device, DepthImageView, nullptr);
		if (DepthImage) vkDestroyImage(Instance.Device, DepthImage, nullptr);
		if (DepthImageMemory) vkFreeMemory(Instance.Device, DepthImageMemory, nullptr);

		for (auto framebuffer : SwapChainFramebuffers)
			vkDestroyFramebuffer(Instance.Device, framebuffer, nullptr);

		for (auto imageView : SwapChainImageViews)
			vkDestroyImageView(Instance.Device, imageView, nullptr);

		if (SwapChain) vkDestroySwapchainKHR(Instance.Device, SwapChain, nullptr);
	}
	DepthImageView = nullptr;
	DepthImage = nullptr;
	DepthImageMemory = nullptr;
	SwapChain = nullptr;
}


bool VulkanSwapChain::createCommandBuffers()
{
	CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo AllocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	AllocateInfo.commandPool = Instance.CommandPool;
	AllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AllocateInfo.commandBufferCount = (uint32_t)CommandBuffers.size();

	if (vkAllocateCommandBuffers(Instance.Device, &AllocateInfo, CommandBuffers.data()) != VK_SUCCESS)
	{
		Fatal("Failed to Allocate Command Buffer");
		return false;
	}

	return true;
}

bool VulkanSwapChain::createSyncObjects()
{
	ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
	RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
	InFlightFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkFenceCreateInfo fenceInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(Instance.Device, &semaphoreInfo, nullptr, &ImageAvailableSemaphores[i]) != VK_SUCCESS
			|| vkCreateSemaphore(Instance.Device, &semaphoreInfo, nullptr, &RenderFinishedSemaphores[i]) != VK_SUCCESS
			|| vkCreateFence(Instance.Device, &fenceInfo, nullptr, &InFlightFences[i]) != VK_SUCCESS)
		{
			Fatal("Failed to create synchronization objects for a frame!");
			return false;
		}
	}
	return true;
}


#pragma endregion

bool RenderContext::Create(const RenderContextCreateInfo& createInfo)
{
	if (!Instance.Create(createInfo)) return false;
	if (!SwapChain.Create()) return false;

	return true;
}

void RenderContext::Destroy()
{
	Instance.Finish();
	//...
	SwapChain.Destroy();
	Instance.Destroy();
}

void RenderContext::BeginFrame()
{
	SwapChain.BeginFrame();
}

void RenderContext::EndFrame()
{
	SwapChain.EndFrame();
}

VulkanInstance& RenderContext::GetInstance()
{
	return Instance;
}

VulkanSwapChain& RenderContext::GetSwapChain()
{
	return SwapChain;
}