#include "Base.h"
#include "Core.h"
#include "Renderer.h"
#include "EngineWindow.h"

namespace
{
	bool UseValidationLayers{ false };
	VkInstance Instance{ nullptr };
	VkDebugUtilsMessengerEXT DebugMessenger{ nullptr };

	VkPhysicalDevice ChosenGPU{ nullptr };
	VkDevice Device{ nullptr };
	VkSurfaceKHR Surface{ nullptr };
	VmaAllocator Allocator{ nullptr };
	VkQueue GraphicsQueue{ nullptr };
	uint32_t GraphicsQueueFamily{ 0 };
	VkQueue PresentQueue{ nullptr };
	uint32_t PresentQueueFamily{ 0 };


	VkFormat SwapchainImageFormat;
	VkSwapchainKHR Swapchain;
	std::vector<VkImage> SwapchainImages;
	std::vector<VkImageView> SwapchainImageViews;
	VkExtent2D SwapchainExtent;
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
		Fatal("volkInitialize failed!");
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

	vkb::PhysicalDeviceSelector physicalDeviceSelector{ vkbInstance };
	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
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
	ChosenGPU = physicalDevice.physical_device;

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
		allocatorInfo.physicalDevice = ChosenGPU;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;
		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}

	return true;
}

bool createSwapChain(uint32_t width, uint32_t height)
{
	vkb::SwapchainBuilder swapChainBuilder{ ChosenGPU,Device,Surface };

	SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

	auto swapChainRet = swapChainBuilder
		//.use_default_format_selection()
		.set_desired_format(VkSurfaceFormatKHR{ .format = SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
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

	SwapchainExtent = vkbSwapchain.extent;
	Swapchain = vkbSwapchain.swapchain;
	SwapchainImages = vkbSwapchain.get_images().value();
	SwapchainImageViews = vkbSwapchain.get_image_views().value();

	return true;
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

bool Renderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight())) return false;

	return true;
}

void Renderer::Close()
{
	if (Device) vkDeviceWaitIdle(Device);

	destroySwapChain();

	if (Allocator) vmaDestroyAllocator(Allocator);
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
	Allocator = nullptr;
	volkFinalize();
}

void Renderer::Draw()
{	
}

VmaAllocator Renderer::GetAllocator()
{
	return Allocator;
}