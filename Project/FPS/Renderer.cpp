#include "Base.h"
#include "Core.h"
#include "Renderer.h"
#include "EngineWindow.h"

#pragma region Vulkan

VkCommandPoolCreateInfo Vulkan::CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
	VkCommandPoolCreateInfo info = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	info.queueFamilyIndex = queueFamilyIndex;
	info.flags = flags;
	return info;
}

VkCommandBufferAllocateInfo Vulkan::CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count)
{
	VkCommandBufferAllocateInfo info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	info.commandPool = pool;
	info.commandBufferCount = count;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	return info;
}

VkFenceCreateInfo Vulkan::FenceCreateInfo(VkFenceCreateFlags flags)
{
	VkFenceCreateInfo info = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	info.flags = flags;
	return info;
}

VkSemaphoreCreateInfo Vulkan::SemaphoreCreateInfo(VkSemaphoreCreateFlags flags)
{
	VkSemaphoreCreateInfo info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	info.flags = flags;
	return info;
}

VkCommandBufferBeginInfo Vulkan::CommandBufferBeginInfo(VkCommandBufferUsageFlags flags)
{
	VkCommandBufferBeginInfo info = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	info.flags = flags;
	return info;
}

#pragma endregion


#pragma region Renderer

namespace
{
	int FrameNumber{ 0 };

	VkInstance Instance;                     // Vulkan library handle
	bool UseValidationLayers = false;
	VkDebugUtilsMessengerEXT DebugMessenger; // Vulkan debug output handle
	VkPhysicalDevice ChosenGPU;              // GPU chosen as the default device
	VkDevice Device;                         // Vulkan device for commands
	VkSurfaceKHR Surface;                    // Vulkan window surface

	VkSwapchainKHR Swapchain;
	VkFormat SwapchainImageFormat;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;
	VkExtent2D SwapchainExtent;

	struct FrameData 
	{
		VkCommandPool commandPool;
		VkCommandBuffer mainCommandBuffer;

		VkSemaphore swapchainSemaphore, renderSemaphore;
		VkFence renderFence;

	};
	constexpr unsigned int FRAME_OVERLAP = 2;
	FrameData Frames[FRAME_OVERLAP];
	VkQueue GraphicsQueue;
	uint32_t GraphicsQueueFamily;
}

FrameData& getCurrentFrame() { return Frames[FrameNumber % FRAME_OVERLAP]; };

bool initVulkan()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS)
	{
		Fatal("volkInitialize failed!");
		return false;
	}

	vkb::InstanceBuilder builder;

	auto inst_ret = builder.set_app_name("FPS Game")
		.request_validation_layers(UseValidationLayers)
		.use_default_debug_messenger()
		.require_api_version(1, 3, 0)
		.build();

	vkb::Instance vkbInst = inst_ret.value();

	Instance = vkbInst.instance;
	DebugMessenger = vkbInst.debug_messenger;

	volkLoadInstanceOnly(Instance);

	result = glfwCreateWindowSurface(Instance, Window::GetWindow(), nullptr, &Surface);
	if (result != VK_SUCCESS)
	{
		Fatal("glfwCreateWindowSurface failed!");
		return false;
	}

	//vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features.dynamicRendering = true;
	features.synchronization2 = true;

	//vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = true;
	features12.descriptorIndexing = true;

	//use vkbootstrap to select a gpu. 
	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features)
		.set_required_features_12(features12)
		.set_surface(Surface)
		.select()
		.value();

	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	Device = vkbDevice.device;
	ChosenGPU = physicalDevice.physical_device;

	volkLoadDevice(Device);

	GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	return true;
}

void createSwapChain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapchainBuilder{ ChosenGPU,Device,Surface };

	SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
		//use vsync present mode
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(width, height)
		.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
		.build()
		.value();

	SwapchainExtent = vkbSwapchain.extent;
	Swapchain = vkbSwapchain.swapchain;
	SwapchainImages = vkbSwapchain.get_images().value();
	SwapchainImageViews = vkbSwapchain.get_image_views().value();
}

void destroySwapChain()
{
	if (Device && Swapchain)
	{
		vkDestroySwapchainKHR(Device, Swapchain, nullptr);
		for (int i = 0; i < SwapchainImageViews.size(); i++)
			vkDestroyImageView(Device, SwapchainImageViews[i], nullptr);
	}
	Swapchain = nullptr;
	SwapchainImageViews.clear();
}

bool initSwapChain()
{
	createSwapChain(Window::GetWidth(), Window::GetHeight());
	return true;
}

bool initCommands()
{
	//create a command pool for commands submitted to the graphics queue.
	//we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = Vulkan::CommandPoolCreateInfo(GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++) 
	{

		VkResult result = vkCreateCommandPool(Device, &commandPoolInfo, nullptr, &Frames[i].commandPool);
		if (result != VK_SUCCESS)
		{
			Fatal("vkCreateCommandPool failed!");
			return false;
		}

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = Vulkan::CommandBufferAllocateInfo(Frames[i].commandPool, 1);

		result = vkAllocateCommandBuffers(Device, &cmdAllocInfo, &Frames[i].mainCommandBuffer);
		if (result != VK_SUCCESS)
		{
			Fatal("vkAllocateCommandBuffers failed!");
			return false;
		}
	}

	return true;
}

bool initSyncStructures()
{
	//create syncronization structures
	//one fence to control when the gpu has finished rendering the frame,
	//and 2 semaphores to syncronize rendering with swapchain
	//we want the fence to start signalled so we can wait on it on the first frame
	VkFenceCreateInfo fenceCreateInfo = Vulkan::FenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreCreateInfo = Vulkan::SemaphoreCreateInfo();

	VkResult result = VK_SUCCESS;
	for (int i = 0; i < FRAME_OVERLAP; i++) 
	{
		result = vkCreateFence(Device, &fenceCreateInfo, nullptr, &Frames[i].renderFence);
		if (result != VK_SUCCESS)
		{
			Fatal("vkCreateFence failed!");
			return false;
		}
		result = vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &Frames[i].swapchainSemaphore);
		if (result != VK_SUCCESS)
		{
			Fatal("vkCreateSemaphore::swapchainSemaphore failed!");
			return false;
		}
		result = vkCreateSemaphore(Device, &semaphoreCreateInfo, nullptr, &Frames[i].renderSemaphore);
		if (result != VK_SUCCESS)
		{
			Fatal("vkCreateSemaphore::renderSemaphore failed!");
			return false;
		}
	}
	
	return true;
}

bool Renderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;
	if (!initSwapChain()) return false;
	if (!initCommands()) return false;
	if (!initSyncStructures()) return false;

	return true;
}

void Renderer::Close()
{
	//make sure the gpu has stopped doing its things
	vkDeviceWaitIdle(Device);

	if (Instance)
	{
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vkDestroyCommandPool(Device, Frames[i].commandPool, nullptr);
		}

		destroySwapChain();

		vkDestroySurfaceKHR(Instance, Surface, nullptr);
		vkDestroyDevice(Device, nullptr);

		vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
		vkDestroyInstance(Instance, nullptr);

	}
	Surface = nullptr;
	Device = nullptr;
	Instance = nullptr;
	volkFinalize();
}

void Renderer::Draw()
{
	// wait until the gpu has finished rendering the last frame. Timeout of 1 second
	VkResult result = vkWaitForFences(Device, 1, &getCurrentFrame().renderFence, true, 1000000000);
	if (result != VK_SUCCESS)
	{
		Fatal("vkWaitForFences failed!");
		return;
	}
	result = vkResetFences(Device, 1, &getCurrentFrame().renderFence);
	if (result != VK_SUCCESS)
	{
		Fatal("vkResetFences failed!");
		return;
	}

	//request image from the swapchain
	uint32_t swapchainImageIndex;
	result = vkAcquireNextImageKHR(Device, Swapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
	if (result != VK_SUCCESS)
	{
		Fatal("vkAcquireNextImageKHR failed!");
		return;
	}

	//naming it cmd for shorter writing
	VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

	// now that we are sure that the commands finished executing, we can safely
	// reset the command buffer to begin recording again.
	result = vkResetCommandBuffer(cmd, 0);
	if (result != VK_SUCCESS)
	{
		Fatal("vkResetCommandBuffer failed!");
		return;
	}

	//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = Vulkan::CommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	//start the command buffer recording
	result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
	if (result != VK_SUCCESS)
	{
		Fatal("vkBeginCommandBuffer failed!");
		return;
	}
}

#pragma endregion