#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"
#include "EngineWindow.h"


#if defined(_MSC_VER)
#	if defined(_DEBUG)
#		pragma comment( lib, "glslang-default-resource-limitsd.lib" )
//#		pragma comment( lib, "SPVRemapperd.lib" )
//#		pragma comment( lib, "MachineIndependentd.lib" )
//#		pragma comment( lib, "OSDependentd.lib" )
//#		pragma comment( lib, "SPIRVd.lib" )
#		pragma comment( lib, "glslangd.lib" )
#		pragma comment( lib, "SPIRV-Tools-optd.lib" )
#		pragma comment( lib, "SPIRV-Toolsd.lib" )
//#		pragma comment( lib, "GenericCodeGend.lib" )
#	else
#		pragma comment( lib, "glslang-default-resource-limits.lib" )
//#		pragma comment( lib, "SPVRemapper.lib" )
//#		pragma comment( lib, "MachineIndependent.lib" )
//#		pragma comment( lib, "OSDependent.lib" )
//#		pragma comment( lib, "SPIRV.lib" )
#		pragma comment( lib, "glslang.lib" )
#		pragma comment( lib, "SPIRV-Tools-opt.lib" )
#		pragma comment( lib, "SPIRV-Tools.lib" )
//#		pragma comment( lib, "GenericCodeGen.lib" )
#	endif
#endif

namespace
{
	VulkanInstance2 Instance;
	VulkanSwapChain SwapChain;
}

#pragma region VulkanInstance

bool VulkanInstance2::Create(const RenderContextCreateInfo2& createInfo)
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
	features10.geometryShader = VK_TRUE;
	features10.fillModeNonSolid = VK_TRUE;

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

	if (!getQueues(vkbDevice)) return false;
	if (!createCommandPool()) return false;
	if (!createDescriptorPool()) return false;
	if (!createAllocator(createInfo.vulkan.requireVersion)) return false;
	if (!createImmediateContext()) return false;
	if (!createBufferingObjects()) return false;

	return true;
}

void VulkanInstance2::Destroy()
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
		for (const sBufferingObjects& bufferingObject : BufferingObjects)
		{
			if (bufferingObject.RenderFence) vkDestroyFence(Device, bufferingObject.RenderFence, nullptr);
			if (bufferingObject.PresentSemaphore) vkDestroySemaphore(Device, bufferingObject.PresentSemaphore, nullptr);
			if (bufferingObject.RenderSemaphore) vkDestroySemaphore(Device, bufferingObject.RenderSemaphore, nullptr);
			if (bufferingObject.CommandBuffer) vkFreeCommandBuffers(Device, CommandPool, 1, &bufferingObject.CommandBuffer);
		}

		if (ImmediateFence) vkDestroyFence(Device, ImmediateFence, nullptr);
		if (ImmediateCommandBuffer) vkFreeCommandBuffers(Device, CommandPool, 1, &ImmediateCommandBuffer);
		if (CommandPool) vkDestroyCommandPool(Device, CommandPool, nullptr);
		if (DescriptorPool) vkDestroyDescriptorPool(Device, DescriptorPool, nullptr);

		vkDestroyDevice(Device, nullptr);
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

void VulkanInstance2::WaitIdle()
{
	if (Device)
	{
		if (vkDeviceWaitIdle(Device) != VK_SUCCESS)
		{
			Fatal("Failed when waiting for Vulkan device to be idle.");
		}
	}
}

size_t VulkanInstance2::GetNumBuffering()
{
	return BufferingObjects.size();
}

bool VulkanInstance2::getQueues(vkb::Device& vkbDevice)
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

bool VulkanInstance2::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = GraphicsQueueFamily;

	if (vkCreateCommandPool(Device, &poolInfo, nullptr, &CommandPool) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan command pool.");
		return false;
	}
	return true;
}

bool VulkanInstance2::createDescriptorPool()
{
	const std::vector<VkDescriptorPoolSize> poolSizes{
		{VK_DESCRIPTOR_TYPE_SAMPLER,                1024},
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024},
		{VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1024},
		{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1024},
		{VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1024},
		{VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1024},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1024},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1024},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024},
		{VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1024}
	};

	VkDescriptorPoolCreateInfo createInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	createInfo.maxSets = 1024;
	createInfo.poolSizeCount = poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();

	if (vkCreateDescriptorPool(Device, &createInfo, nullptr, &DescriptorPool) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan descriptor pool.");
		return false;
	}

	return true;
}

bool VulkanInstance2::createAllocator(uint32_t vulkanApiVersion)
{
	// initialize the memory allocator
	VmaVulkanFunctions vmaVulkanFunc{};
	vmaVulkanFunc.vkGetInstanceProcAddr               = vkGetInstanceProcAddr;
	vmaVulkanFunc.vkGetDeviceProcAddr                 = vkGetDeviceProcAddr;
	vmaVulkanFunc.vkAllocateMemory                    = vkAllocateMemory;
	vmaVulkanFunc.vkBindBufferMemory                  = vkBindBufferMemory;
	vmaVulkanFunc.vkBindImageMemory                   = vkBindImageMemory;
	vmaVulkanFunc.vkCreateBuffer                      = vkCreateBuffer;
	vmaVulkanFunc.vkCreateImage                       = vkCreateImage;
	vmaVulkanFunc.vkDestroyBuffer                     = vkDestroyBuffer;
	vmaVulkanFunc.vkDestroyImage                      = vkDestroyImage;
	vmaVulkanFunc.vkFlushMappedMemoryRanges           = vkFlushMappedMemoryRanges;
	vmaVulkanFunc.vkFreeMemory                        = vkFreeMemory;
	vmaVulkanFunc.vkGetBufferMemoryRequirements       = vkGetBufferMemoryRequirements;
	vmaVulkanFunc.vkGetImageMemoryRequirements        = vkGetImageMemoryRequirements;
	vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
	vmaVulkanFunc.vkGetPhysicalDeviceProperties       = vkGetPhysicalDeviceProperties;
	vmaVulkanFunc.vkInvalidateMappedMemoryRanges      = vkInvalidateMappedMemoryRanges;
	vmaVulkanFunc.vkMapMemory                         = vkMapMemory;
	vmaVulkanFunc.vkUnmapMemory                       = vkUnmapMemory;
	vmaVulkanFunc.vkCmdCopyBuffer                     = vkCmdCopyBuffer;

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.vulkanApiVersion = vulkanApiVersion;
	allocatorInfo.physicalDevice   = PhysicalDevice;
	allocatorInfo.device           = Device;
	allocatorInfo.instance         = Instance;
	//allocatorInfo.flags          = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
	allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;

	if (vmaCreateAllocator(&allocatorInfo, &Allocator) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan Memory Allocator.");
		return false;
	}
	return true;
}

bool VulkanInstance2::createImmediateContext()
{
	auto fence = createFence();
	if (!fence)
	{
		Fatal("Failed to create Vulkan Immediate Fence.");
		return false;
	}
	ImmediateFence = fence.value();

	auto commandBuffer = allocateCommandBuffer();
	if (!commandBuffer)
	{
		Fatal("Failed to create Vulkan Immediate Command Buffer.");
		return false;
	}
	ImmediateCommandBuffer = commandBuffer.value();

	return true;
}

std::optional<VkFence> VulkanInstance2::createFence(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	createInfo.flags = flags;

	VkFence fence;
	if (vkCreateFence(Device, &createInfo, nullptr, &fence) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan fence.");
		return std::nullopt;
	}
	return fence;
}

std::optional<VkCommandBuffer> VulkanInstance2::allocateCommandBuffer()
{
	VkCommandBufferAllocateInfo allocateInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandPool = CommandPool;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	if (vkAllocateCommandBuffers(Device, &allocateInfo, &commandBuffer) != VK_SUCCESS)
	{
		Fatal("Failed to allocate Vulkan command buffer.");
		return std::nullopt;
	}
	return commandBuffer;
}

bool VulkanInstance2::createBufferingObjects()
{
	for (sBufferingObjects& bufferingObject : BufferingObjects)
	{
		auto fence = createFence(VK_FENCE_CREATE_SIGNALED_BIT);
		if (!fence)
		{
			Fatal("Failed to create Vulkan Fence.");
			return false;
		}
		bufferingObject.RenderFence = fence.value();

		auto semaphore = createSemaphore();
		if (!semaphore)
		{
			Fatal("Failed to create Vulkan Semaphore.");
			return false;
		}
		bufferingObject.PresentSemaphore = semaphore.value();
		semaphore = createSemaphore();
		if (!semaphore)
		{
			Fatal("Failed to create Vulkan Semaphore.");
			return false;
		}
		bufferingObject.RenderSemaphore = semaphore.value();

		auto commandBuffer = allocateCommandBuffer();
		if (!commandBuffer)
		{
			Fatal("Failed to create Vulkan Command Buffer.");
			return false;
		}
		bufferingObject.CommandBuffer = commandBuffer.value();
	}
	return true;
}

std::optional<VkSemaphore> VulkanInstance2::createSemaphore()
{
	VkSemaphoreCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	VkSemaphore semaphore;
	if (vkCreateSemaphore(Device, &createInfo, nullptr, &semaphore) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan semaphore.");
		return std::nullopt;
	}
	return semaphore;
}

#pragma endregion

#pragma region VulkanSwapChain

bool VulkanSwapChain::Create()
{
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;
	if (!createPrimaryRenderPass()) return false;
	if (!createPrimaryFramebuffers()) return false;

	return true;
}

void VulkanSwapChain::Destroy()
{
	cleanupPrimaryFramebuffers();
	Render::DestroyRenderPass(PrimaryRenderPass);
	PrimaryRenderPass = nullptr;
	cleanupSurfaceSwapchainAndImageViews();
}

VulkanFrameInfo VulkanSwapChain::BeginFrame()
{
	const sBufferingObjects& bufferingObjects = Instance.BufferingObjects[CurrentBufferingIndex];

	waitAndResetFence(bufferingObjects.RenderFence);

	acquireNextSwapchainImage();

	vkResetCommandBuffer(bufferingObjects.CommandBuffer, 0);
	Render::BeginCommandBuffer(bufferingObjects.CommandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	return { &PrimaryRenderPassBeginInfos[CurrentSwapchainImageIndex], CurrentBufferingIndex, bufferingObjects.CommandBuffer };
}

void VulkanSwapChain::EndFrame()
{
	sBufferingObjects& bufferingObjects = Instance.BufferingObjects[CurrentBufferingIndex];

	Render::EndCommandBuffer(bufferingObjects.CommandBuffer);

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{ .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &bufferingObjects.PresentSemaphore;
	submitInfo.pWaitDstStageMask = &waitStage;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &bufferingObjects.CommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &bufferingObjects.RenderSemaphore;

	submitToGraphicsQueue(submitInfo, bufferingObjects.RenderFence);

	VkPresentInfoKHR presentInfo = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &bufferingObjects.RenderSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &SwapChain;
	presentInfo.pImageIndices = &CurrentSwapchainImageIndex;
	const VkResult result = vkQueuePresentKHR(Instance.GraphicsQueue, &presentInfo); // TODO: может PresentQueue??
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		recreateSwapchain();
	}
	else
	{
		if (result != VK_SUCCESS)
			Fatal("Failed to present Vulkan swapchain image.");
	}

	CurrentBufferingIndex = (CurrentBufferingIndex + 1) % Instance.BufferingObjects.size();
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
		//.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
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

bool VulkanSwapChain::createPrimaryRenderPass()
{
	VulkanRenderPassOptions options;
	options.ForPresentation = true;

	auto renderPass = Render::CreateRenderPass({ SwapChainImageFormat }, VK_FORMAT_UNDEFINED, options);
	if (!renderPass)
	{
		Fatal("Failed to create Render Pass");
		return false;
	}
	PrimaryRenderPass = renderPass.value();

	return true;
}

bool VulkanSwapChain::createPrimaryFramebuffers()
{
	const size_t numSwapchainImages = SwapChainImageViews.size();
	for (int i = 0; i < numSwapchainImages; i++)
	{
		auto fb = Render::CreateFramebuffer(PrimaryRenderPass, { SwapChainImageViews[i] }, SwapChainExtent); // TODO: зачем-то создаю снова вектор, хотя оно и так в векторе
		if (!fb)
		{
			Fatal("Failed to create Framebuffer");
			return false;
		}
		PrimaryFramebuffers.push_back(fb.value());
	}

	VkRect2D renderArea{};
	renderArea.offset = { 0, 0 };
	renderArea.extent = SwapChainExtent;

	VkClearValue clearValues{};
	clearValues.color = { 0.0f, 0.0f, 0.0f, 1.0f };
	clearValues.depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo beginInfo = { .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	beginInfo.renderPass = PrimaryRenderPass;
	beginInfo.renderArea = renderArea;
	beginInfo.clearValueCount = 1;
	beginInfo.pClearValues = &clearValues;
	for (const VkFramebuffer& framebuffer : PrimaryFramebuffers)
	{
		beginInfo.framebuffer = framebuffer;
		PrimaryRenderPassBeginInfos.push_back(beginInfo);
	}
}

void VulkanSwapChain::cleanupSurfaceSwapchainAndImageViews()
{
	if (Instance.Device)
	{
		for (const VkImageView& imageView : SwapChainImageViews)
		{
			if (imageView) vkDestroyImageView(Instance.Device, imageView, nullptr);
		}
		SwapChainImageViews.clear();
		if (SwapChain) vkDestroySwapchainKHR(Instance.Device, SwapChain, nullptr);
		SwapChain = nullptr;
	}
}

void VulkanSwapChain::cleanupPrimaryFramebuffers()
{
	if (Instance.Device)
	{
		PrimaryRenderPassBeginInfos.clear();
		for (const VkFramebuffer& framebuffer : PrimaryFramebuffers)
			vkDestroyFramebuffer(Instance.Device, framebuffer, nullptr);

		PrimaryFramebuffers.clear();
	}
}

bool VulkanSwapChain::recreateSwapchain()
{
	// block when minimized
	int width = 0, height = 0; // TODO: случай, если приложение свернуто.
	glfwGetFramebufferSize(Window::GetWindow(), &width, &height);
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(Window::GetWindow(), &width, &height);
		glfwWaitEvents();
	}

	RenderContext::GetInstance().WaitIdle();

	cleanupPrimaryFramebuffers();
	cleanupSurfaceSwapchainAndImageViews();
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;
	if (!createPrimaryFramebuffers()) return false;

	return true;
}

VkResult VulkanSwapChain::tryAcquiringNextSwapchainImage()
{
	const sBufferingObjects& bufferingObjects = Instance.BufferingObjects[CurrentBufferingIndex];
	VkResult result = vkAcquireNextImageKHR(Instance.Device, SwapChain, 100'000'000, bufferingObjects.PresentSemaphore, VK_NULL_HANDLE, &CurrentSwapchainImageIndex);
	return result;
}

void VulkanSwapChain::acquireNextSwapchainImage()
{
	VkResult result = tryAcquiringNextSwapchainImage();
	if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapchain();
			result = tryAcquiringNextSwapchainImage();
		}

		if (result != VK_SUCCESS)
		{
			Fatal("Failed to acquire next Vulkan swapchain image.");
			return;
		}
	}
}

void VulkanSwapChain::waitAndResetFence(VkFence fence, uint64_t timeout)
{
	if (vkWaitForFences(Instance.Device, 1, &fence, VK_TRUE, timeout) != VK_SUCCESS)
	{
		Fatal("Failed to wait for Vulkan fence.");
		return;
	}
	vkResetFences(Instance.Device, 1, &fence);
}

void VulkanSwapChain::submitToGraphicsQueue(const VkSubmitInfo& submitInfo, VkFence fence)
{
	if (vkQueueSubmit(Instance.GraphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS)
		Fatal("Failed to submit Vulkan command buffer.");
}

#pragma endregion

bool RenderContext::Create(const RenderContextCreateInfo2& createInfo)
{
	if (!Instance.Create(createInfo)) return false;
	if (!SwapChain.Create()) return false;

	return true;
}

void RenderContext::Destroy()
{
	Instance.WaitIdle();
	//...
	SwapChain.Destroy();
	Instance.Destroy();
}

void RenderContext::BeginFrame()
{
	//SwapChain.BeginFrame();
}

void RenderContext::EndFrame()
{
	//SwapChain.EndFrame();
}

VulkanInstance2& RenderContext::GetInstance()
{
	return Instance;
}

VulkanSwapChain& RenderContext::GetSwapChain()
{
	return SwapChain;
}