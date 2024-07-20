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
}

bool initVulkan()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS)
	{
		Fatal("volkInitialize failed!");
		return false;
	}

	// current version vulkan
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

	// use vkbootstrap to select a gpu.
	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 3)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
		.set_surface(Surface)
		.select()
		.value();

	// create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	Device = vkbDevice.device;
	ChosenGPU = physicalDevice.physical_device;

	volkLoadDevice(Device);

	GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

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

bool Renderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;

	return true;
}

void Renderer::Close()
{
	if (Device) vkDeviceWaitIdle(Device);

	if (Allocator) vmaDestroyAllocator(Allocator);

	if (Instance && Surface) vkDestroySurfaceKHR(Instance, Surface, nullptr);
	if (Device) vkDestroyDevice(Device, nullptr);

	if (Instance)
	{
		vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
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