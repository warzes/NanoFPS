#include "Base.h"
#include "Core.h"
#include "RenderResources.h"
#include "RenderContext.h"
#include "EngineWindow.h"
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif


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

#pragma region VulkanInstance

bool VulkanInstance::Create(const RenderContextCreateInfo& createInfo)
{
	if (!createInstanceAndDevice(createInfo)) return false;
	if (!createCommandPool()) return false;
	if (!createDescriptorPool()) return false;
	if (!createAllocator(createInfo.vulkan.requireVersion)) return false;
	if (!createImmediateContext()) return false;

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
	ImmediateFence = nullptr;
	ImmediateCommandBuffer = nullptr;
	CommandPool = nullptr;
	DescriptorPool = nullptr;
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

bool VulkanInstance::createInstanceAndDevice(const RenderContextCreateInfo& createInfo)
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

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
#endif

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

#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Instance(Instance));
#endif

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
#if VULKAN_HPP_DISPATCH_LOADER_DYNAMIC
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vk::Device(Device));
#endif

	vkGetPhysicalDeviceProperties(PhysicalDevice, &PhysicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(PhysicalDevice, &PhysicalDeviceFeatures);

	Print("GPU Used: " + std::string(PhysicalDeviceProperties.deviceName));

	if (!getQueues(vkbDevice)) return false;
	return true;
}

bool VulkanInstance::getQueues(vkb::Device& vkbDevice)
{
	auto graphicsQueueRet = vkbDevice.get_queue(vkb::QueueType::graphics);
	if (!graphicsQueueRet)
	{
		Fatal("failed to get graphics queue: " + graphicsQueueRet.error().message());
		return false;
	}
	GraphicsQueue.Queue = graphicsQueueRet.value();

	auto graphicsQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::graphics);
	if (!graphicsQueueFamilyRet)
	{
		Fatal("failed to get graphics queue index: " + graphicsQueueFamilyRet.error().message());
		return false;
	}
	GraphicsQueue.QueueFamily = graphicsQueueFamilyRet.value();

	auto presentQueueRet = vkbDevice.get_queue(vkb::QueueType::present);
	if (!presentQueueRet)
	{
		Fatal("failed to get present queue: " + presentQueueRet.error().message());
		return false;
	}
	PresentQueue.Queue = presentQueueRet.value();

	auto presentQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::present);
	if (!presentQueueFamilyRet)
	{
		Fatal("failed to get present queue index: " + presentQueueFamilyRet.error().message());
		return false;
	}
	PresentQueue.QueueFamily = presentQueueFamilyRet.value();

	auto transferQueueRet = vkbDevice.get_queue(vkb::QueueType::transfer);
	if (!transferQueueRet)
	{
		Fatal("failed to get transfer queue: " + transferQueueRet.error().message());
		return false;
	}
	TransferQueue.Queue = transferQueueRet.value();

	auto transferQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::transfer);
	if (!transferQueueFamilyRet)
	{
		Fatal("failed to get transfer queue index: " + transferQueueFamilyRet.error().message());
		return false;
	}
	TransferQueue.QueueFamily = transferQueueFamilyRet.value();

	auto computeQueueRet = vkbDevice.get_queue(vkb::QueueType::compute);
	if (!computeQueueRet)
	{
		Fatal("failed to get compute queue: " + computeQueueRet.error().message());
		return false;
	}
	PresentQueue.Queue = computeQueueRet.value();

	auto computeQueueFamilyRet = vkbDevice.get_queue_index(vkb::QueueType::compute);
	if (!computeQueueFamilyRet)
	{
		Fatal("failed to get compute queue index: " + computeQueueFamilyRet.error().message());
		return false;
	}
	PresentQueue.QueueFamily = computeQueueFamilyRet.value();

	return true;
}

bool VulkanInstance::createCommandPool()
{
	VkCommandPoolCreateInfo createInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = GraphicsQueue.QueueFamily;

	if (vkCreateCommandPool(Device, &createInfo, nullptr, &CommandPool) != VK_SUCCESS)
	{
		Fatal("Failed to create Vulkan command pool.");
		return false;
	}

	return true;
}

bool VulkanInstance::createDescriptorPool()
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
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
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

bool VulkanInstance::createImmediateContext()
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

std::optional<VkFence> VulkanInstance::createFence(VkFenceCreateFlags flags)
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

std::optional<VkCommandBuffer> VulkanInstance::allocateCommandBuffer()
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

void VulkanInstance::temp()
{
	m_graphicsQueue = GraphicsQueue.Queue;
	m_physicalDevice = PhysicalDevice;
	m_device = Device;
	m_surface = Surface;
	m_commandPool = CommandPool;
	m_descriptorPool = DescriptorPool;
	m_immediateFence = ImmediateFence;
	m_immediateCommandBuffer = ImmediateCommandBuffer;
}

#pragma endregion


bool RenderContext::Create(const RenderContextCreateInfo& createInfo)
{
	return true;
}

void RenderContext::Destroy()
{
}

void RenderContext::BeginFrame()
{
}

void RenderContext::EndFrame()
{
}