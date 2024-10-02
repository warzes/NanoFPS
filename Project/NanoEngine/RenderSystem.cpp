#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"
#include "Application.h"

#include "font_inconsolata.h"

namespace vkr {

#pragma region VulkanInstance

VkBool32 VKAPI_PTR DebugUtilsMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	// Ignore these messages because they're nonsense
	if (
		(std::string(pCallbackData->pMessageIdName) == "VUID-VkShaderModuleCreateInfo-pCode-08742") // vkCreateShaderModule(): The SPIR-V Extension (SPV_GOOGLE_hlsl_functionality1) was declared, but none of the requirements were met to use it. The Vulkan spec states: If pCode declares any of the SPIR-V extensions listed in the SPIR-V Environment appendix, one of the corresponding requirements must be satisfied
		) {
		return VK_FALSE;
	}

	// Severity
	std::string severity = "<UNKNOWN MESSAGE SEVERITY>";
	switch (messageSeverity) {
	default: break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = "VERBOSE"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = "INFO"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = "WARNING"; break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = "ERROR"; break;
	}

	// Type
	std::stringstream ssType;
	ssType << "[";
	{
		uint32_t type_count = 0;
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
			ssType << "GENERAL";
			++type_count;
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) {
			if (type_count > 0) {
				ssType << ", ";
			}
			ssType << "VALIDATION";
			++type_count;
		}
		if (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) {
			if (type_count > 0) {
				ssType << ", ";
			}
			ssType << "PERFORMANCE";
		}
	}
	ssType << "]";
	std::string type = ssType.str();
	if (type.empty()) {
		type = "<UNKNOWN MESSAGE TYPE>";
	}

	std::stringstream ss;
	ss << "\n";
	ss << "*** VULKAN VALIDATION " << severity << " MESSAGE ***" << "\n";
	ss << "Severity : " << severity << "\n";
	ss << "Type     : " << type << "\n";

	if (pCallbackData->objectCount > 0)
	{
		ss << "Objects  : ";
		for (uint32_t i = 0; i < pCallbackData->objectCount; ++i)
		{
			auto object_name_info = pCallbackData->pObjects[i];

			std::string name = (object_name_info.pObjectName != nullptr)
				? object_name_info.pObjectName
				: "<UNNAMED OBJECT>";
			if (i > 0)
			{
				ss << "           ";
			}
			ss << "[" << i << "]" << ": " << name << "\n";
		}
	}

	ss << "Message  : " << pCallbackData->pMessage;
	ss << std::endl;

	bool isError = (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
	bool isValidation = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT);
	if (isError && isValidation)
	{
#if defined(_DEBUG)
		Error(ss.str());
#else
		Fatal(ss.str());
#endif
	}
	else
	{
		Error(ss.str());
	}

	return VK_FALSE;
}

VulkanInstance::VulkanInstance(RenderSystem& render)
	: m_render(render)
{
}

bool VulkanInstance::Setup(const InstanceCreateInfo& createInfo, GLFWwindow* window)
{
	if (volkInitialize() != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}

	auto instanceRet = createInstance(createInfo);
	if (!instanceRet) return false;		
	vkb::Instance vkbInstance = instanceRet.value();
	instance = vkbInstance.instance;
	debugMessenger = vkbInstance.debug_messenger;

	volkLoadInstanceOnly(instance);

	surface = createSurfaceGLFW(window);
	if (surface == VK_NULL_HANDLE) return false;

	auto physicalDeviceRet = selectDevice(vkbInstance, createInfo);
	if (!physicalDeviceRet) return false;	
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

	if (!getQueues(vkbDevice)) return false;
	if (!initVma()) return false;
	if (!getDeviceInfo()) return false;

	Print("GPU Used: " + std::string(physicalDeviceProperties.deviceName));

	return true;
}

void VulkanInstance::Shutdown()
{
	if (vmaAllocator)
	{
		VmaTotalStatistics stats;
		vmaCalculateStatistics(vmaAllocator, &stats);
		Print("Total device memory leaked: " + std::to_string(stats.total.statistics.allocationBytes) + " bytes.");
		vmaDestroyAllocator(vmaAllocator);
		vmaAllocator = VK_NULL_HANDLE;
	}

	graphicsQueue.reset();
	presentQueue.reset();
	transferQueue.reset();
	computeQueue.reset();

	if (device)                     vkDestroyDevice(device, nullptr);
	if (instance && surface)        vkDestroySurfaceKHR(instance, surface, nullptr);
	if (instance && debugMessenger) vkb::destroy_debug_utils_messenger(instance, debugMessenger);
	if (instance)                   vkDestroyInstance(instance, nullptr);

	device         = nullptr;
	surface        = nullptr;
	debugMessenger = nullptr;
	instance       = nullptr;
	volkFinalize();
}

std::optional<vkb::Instance> VulkanInstance::createInstance(const InstanceCreateInfo& createInfo)
{
	bool useValidationLayers = createInfo.useValidationLayers;
#if defined(_DEBUG)
	useValidationLayers = true;
#endif

	vkb::InstanceBuilder instanceBuilder;

	auto instanceRet = instanceBuilder
		.set_app_name(createInfo.applicationName.data())
		.set_engine_name(createInfo.engineName.data())
		.set_app_version(createInfo.appVersion)
		.set_engine_version(createInfo.engineVersion)
		.set_minimum_instance_version(createInfo.requireVulkanVersion)
		.require_api_version(createInfo.requireVulkanVersion)
		.enable_extensions(createInfo.instanceExtensions)
		.request_validation_layers(useValidationLayers)
		//.use_default_debug_messenger()
		.set_debug_callback(DebugUtilsMessengerCallback)
		.build();
	if (!instanceRet)
	{
		Fatal(instanceRet.error().message());
		return std::nullopt;
	}

	return instanceRet.value();
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

std::optional<vkb::PhysicalDevice> VulkanInstance::selectDevice(const vkb::Instance& instance, const InstanceCreateInfo& createInfo)
{
	// vulkan 1.0 features
	VkPhysicalDeviceFeatures features10{};
	features10.fillModeNonSolid                        = VK_TRUE;
	features10.fullDrawIndexUint32                     = VK_TRUE;
	features10.imageCubeArray                          = VK_TRUE;
	features10.independentBlend                        = VK_TRUE;
	features10.pipelineStatisticsQuery                 = VK_TRUE;
	features10.geometryShader                          = VK_TRUE;
	features10.tessellationShader                      = VK_TRUE;
	features10.fragmentStoresAndAtomics                = VK_TRUE;
	features10.shaderStorageImageReadWithoutFormat     = VK_TRUE;
	features10.shaderStorageImageWriteWithoutFormat    = VK_TRUE;
	features10.shaderStorageImageMultisample           = VK_TRUE;
	features10.samplerAnisotropy                       = VK_TRUE;
	features10.shaderUniformBufferArrayDynamicIndexing = VK_TRUE;
	features10.shaderSampledImageArrayDynamicIndexing  = VK_TRUE;
	features10.shaderStorageBufferArrayDynamicIndexing = VK_TRUE;
	features10.shaderStorageImageArrayDynamicIndexing  = VK_TRUE;

	// vulkan 1.1 features
	VkPhysicalDeviceVulkan11Features features11{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	// vulkan 1.2 features
	VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	features12.bufferDeviceAddress = VK_TRUE;
	features12.descriptorIndexing = VK_TRUE;
	features12.hostQueryReset = VK_TRUE;
	// vulkan 1.3 features
	VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	features13.dynamicRendering = VK_TRUE;
	features13.synchronization2 = VK_TRUE;
	
	vkb::PhysicalDeviceSelector physicalDeviceSelector{ instance };

	auto physicalDeviceRet = physicalDeviceSelector
		.set_minimum_version(1, 3)
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.allow_any_gpu_device_type(false)
		.add_required_extensions(createInfo.deviceExtensions)
		.set_required_features_13(features13)
		.set_required_features_12(features12)
		.set_required_features_11(features11)
		.set_required_features(features10)
		.set_surface(surface)
		.select();
	if (!physicalDeviceRet)
	{
		Fatal(physicalDeviceRet.error().message());
		return std::nullopt;
	}

	return physicalDeviceRet.value();
}

bool VulkanInstance::getQueues(vkb::Device& vkbDevice)
{
	if (!graphicsQueue->init(vkbDevice, vkb::QueueType::graphics)) return false;
	if (!presentQueue->init(vkbDevice, vkb::QueueType::present)) return false;
	if (!transferQueue->init(vkbDevice, vkb::QueueType::transfer)) return false;
	if (!computeQueue->init(vkbDevice, vkb::QueueType::compute)) return false;

	return true;
}

bool VulkanInstance::initVma()
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

	VmaAllocatorCreateInfo vmaCreateInfo = {};
	vmaCreateInfo.physicalDevice = physicalDevice;
	vmaCreateInfo.device         = device;
	vmaCreateInfo.instance       = instance;
	vmaCreateInfo.pVulkanFunctions = &vmaVulkanFunc;

	VkResult result = vmaCreateAllocator(&vmaCreateInfo, &vmaAllocator);
	if (result != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vmaCreateAllocator failed: " + ToString(result));
		return false;
	}

	return true;
}

bool VulkanInstance::getDeviceInfo()
{
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

	{
		VkPhysicalDevicePushDescriptorPropertiesKHR pushDescriptorProperties = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR };
		VkPhysicalDeviceProperties2 properties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		properties.pNext = &pushDescriptorProperties;

		vkGetPhysicalDeviceProperties2(physicalDevice, &properties);
		maxPushDescriptors = pushDescriptorProperties.maxPushDescriptors;
	}

	{
		VkPhysicalDeviceDescriptorIndexingFeatures foundDiFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES };
		VkPhysicalDeviceFeatures2 foundFeatures{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &foundDiFeatures };
		vkGetPhysicalDeviceFeatures2(physicalDevice, &foundFeatures);

		descriptorIndexingFeatures.shaderInputAttachmentArrayDynamicIndexing = foundDiFeatures.shaderInputAttachmentArrayDynamicIndexing;
		descriptorIndexingFeatures.shaderUniformTexelBufferArrayDynamicIndexing = foundDiFeatures.shaderUniformTexelBufferArrayDynamicIndexing;
		descriptorIndexingFeatures.shaderStorageTexelBufferArrayDynamicIndexing = foundDiFeatures.shaderStorageTexelBufferArrayDynamicIndexing;
		descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = foundDiFeatures.shaderUniformBufferArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = foundDiFeatures.shaderSampledImageArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = foundDiFeatures.shaderStorageBufferArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = foundDiFeatures.shaderStorageImageArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderInputAttachmentArrayNonUniformIndexing = foundDiFeatures.shaderInputAttachmentArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderUniformTexelBufferArrayNonUniformIndexing = foundDiFeatures.shaderUniformTexelBufferArrayNonUniformIndexing;
		descriptorIndexingFeatures.shaderStorageTexelBufferArrayNonUniformIndexing = foundDiFeatures.shaderStorageTexelBufferArrayNonUniformIndexing;
		descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = foundDiFeatures.descriptorBindingUniformBufferUpdateAfterBind;
		descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		descriptorIndexingFeatures.descriptorBindingStorageImageUpdateAfterBind = foundDiFeatures.descriptorBindingStorageImageUpdateAfterBind;
		descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = foundDiFeatures.descriptorBindingStorageBufferUpdateAfterBind;
		descriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = foundDiFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind;
		descriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = foundDiFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind;
		descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending = foundDiFeatures.descriptorBindingUpdateUnusedWhilePending;
		descriptorIndexingFeatures.descriptorBindingPartiallyBound = foundDiFeatures.descriptorBindingPartiallyBound;
		descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = foundDiFeatures.descriptorBindingVariableDescriptorCount;
		descriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
	}

	return true;
}

#pragma endregion

#pragma region VulkanSurface

VulkanSurface::VulkanSurface(RenderSystem& render)
	: m_render(render)
{
}

bool VulkanSurface::Setup()
{
	VkResult result = VK_SUCCESS;
	VkSurfaceCapabilitiesKHR surfaceCaps = {};
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &surfaceCaps);
	if (result != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR failed: " + ToString(result));
		return false;
	}

	Print("Vulkan swapchain surface info");
	Print("   minImageCount : " + std::to_string(surfaceCaps.minImageCount));
	Print("   maxImageCount : " + std::to_string(surfaceCaps.maxImageCount));

	// Surface formats
	{
		uint32_t count = 0;

		result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, nullptr);
		if (result != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceFormatsKHR(0) failed: " + ToString(result));
			return false;
		}

		if (count > 0)
		{
			m_surfaceFormats.resize(count);

			result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, m_surfaceFormats.data());
			if (result != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceFormatsKHR(1) failed: " + ToString(result));
				return false;
			}
		}
	}

	// Present modes
	{
		uint32_t count = 0;

		result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, nullptr);
		if (result != VK_SUCCESS)
		{
			ASSERT_MSG(false, "vkGetPhysicalDeviceSurfacePresentModesKHR(0) failed: " + ToString(result));
			return false;
		}

		if (count > 0)
		{
			m_presentModes.resize(count);

			result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, m_presentModes.data());
			if (result != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vkGetPhysicalDeviceSurfacePresentModesKHR(1) failed: " + ToString(result));
				return false;
			}
		}
	}

	return true;
}

VkSurfaceCapabilitiesKHR VulkanSurface::GetCapabilities() const
{
	VkSurfaceCapabilitiesKHR surfaceCaps = {};
	VkResult                 vkres = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		m_render.GetVkPhysicalDevice(),
		m_render.GetVkSurface(),
		&surfaceCaps);
	if (vkres != VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR(1) failed: " + ToString(vkres));
	}
	return surfaceCaps;
}

uint32_t VulkanSurface::GetMinImageWidth() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.minImageExtent.width;
}

uint32_t VulkanSurface::GetMinImageHeight() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.minImageExtent.height;
}

uint32_t VulkanSurface::GetMinImageCount() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.minImageCount;
}

uint32_t VulkanSurface::GetMaxImageWidth() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.maxImageExtent.width;
}

uint32_t VulkanSurface::GetMaxImageHeight() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.maxImageExtent.height;
}

uint32_t VulkanSurface::GetMaxImageCount() const
{
	auto surfaceCaps = GetCapabilities();
	return surfaceCaps.maxImageCount;
}

uint32_t VulkanSurface::GetCurrentImageWidth() const
{
	auto surfaceCaps = GetCapabilities();
	// When surface size is determined by swapchain size
	//   currentExtent.width == kInvalidExtend
	return surfaceCaps.currentExtent.width;
}

uint32_t VulkanSurface::GetCurrentImageHeight() const
{
	auto surfaceCaps = GetCapabilities();
	// When surface size is determined by swapchain size
	//   currentExtent.height == kInvalidExtend
	return surfaceCaps.currentExtent.height;
}

#pragma endregion

#pragma region VulkanSwapchain

VulkanSwapChain::VulkanSwapChain(RenderSystem& render)
	: m_render(render)
{
}

bool VulkanSwapChain::Setup(const VulkanSwapChainCreateInfo& createInfo)
{
	m_createInfo = createInfo;

	ASSERT_NULL_ARG(createInfo.queue);
	if (IsNull(createInfo.queue)) return false;

	// Vulkan create
	{
		std::vector<VkImage> colorImages;
		std::vector<VkImage> depthImages;

#if 1
		{
			// Currently, we're going to assume IDENTITY for all platforms.
			// On Android we will incur the cost of the compositor doing the rotation for us. We don't currently have the facilities in place to inform the application of orientation changes and supply it with the correct pretransform matrix.
			VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

			// Surface capabilities check
			{
				bool  isImageCountValid = false;
				const VkSurfaceCapabilitiesKHR& caps = m_createInfo.surface->GetCapabilities();
				if (caps.maxImageCount > 0)
				{
					bool isInBoundsMin = (m_createInfo.imageCount >= caps.minImageCount);
					bool isInBoundsMax = (m_createInfo.imageCount <= caps.maxImageCount);
					isImageCountValid = isInBoundsMin && isInBoundsMax;
				}
				else
				{
					isImageCountValid = (m_createInfo.imageCount >= caps.minImageCount);
				}
				if (!isImageCountValid)
				{
					ASSERT_MSG(false, "Invalid swapchain image count");
					return false;
				}
			}

			// Surface format
			VkSurfaceFormatKHR surfaceFormat = {};
			{
				VkFormat format = ToVkEnum(m_createInfo.colorFormat);
				if (format == VK_FORMAT_UNDEFINED)
				{
					ASSERT_MSG(false, "Invalid swapchain format");
					return false;
				}

				const std::vector<VkSurfaceFormatKHR>& surfaceFormats = m_createInfo.surface->GetSurfaceFormats();
				auto it = std::find_if(
					std::begin(surfaceFormats),
					std::end(surfaceFormats),
					[format](const VkSurfaceFormatKHR& elem) -> bool {
						bool isMatch = (elem.format == format);
						return isMatch; });

				if (it == std::end(surfaceFormats))
				{
					ASSERT_MSG(false, "Unsupported swapchain format");
					return false;
				}

				surfaceFormat = *it;
			}

			// Present mode
			VkPresentModeKHR presentMode = ToVkEnum(createInfo.presentMode);
			if (presentMode == InvalidValue<VkPresentModeKHR>())
			{
				ASSERT_MSG(false, "Invalid swapchain present mode");
				return false;
			}
			// Fall back if present mode isn't supported
			{
				uint32_t count = 0;
				VkResult vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, nullptr);
				if (vkres != VK_SUCCESS)
				{
					ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " + ToString(vkres));
					return false;
				}

				std::vector<VkPresentModeKHR> presentModes(count);
				vkres = vkGetPhysicalDeviceSurfacePresentModesKHR(m_render.GetVkPhysicalDevice(), m_render.GetVkSurface(), &count, presentModes.data());
				if (vkres != VK_SUCCESS)
				{
					ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " + ToString(vkres));
					return false;
				}

				bool supported = false;
				for (uint32_t i = 0; i < count; ++i)
				{
					if (presentMode == presentModes[i])
					{
						supported = true;
						break;
					}
				}
				if (!supported)
				{
					Warning("Switching Vulkan present mode to VK_PRESENT_MODE_FIFO_KHR because " + ToString(presentMode) + " is not supported");
					presentMode = VK_PRESENT_MODE_FIFO_KHR;
				}
			}

			// Image usage
			// NOTE: D3D12 support for DXGI_USAGE_UNORDERED_ACCESS is pretty spotty so we'll leave out VK_IMAGE_USAGE_STORAGE_BIT for now to keep the swwapchains between D3D12 and Vulkan as equivalent as possible.
			VkImageUsageFlags usageFlags =
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
				VK_IMAGE_USAGE_TRANSFER_DST_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT |
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

			// Create swapchain
			VkSwapchainCreateInfoKHR vkci = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
			vkci.pNext = nullptr;
			vkci.flags = 0;
			vkci.surface = m_render.GetVkSurface();
			vkci.minImageCount = createInfo.imageCount;
			vkci.imageFormat = surfaceFormat.format;
			vkci.imageColorSpace = surfaceFormat.colorSpace;
			vkci.imageExtent = { createInfo.width, createInfo.height };
			vkci.imageArrayLayers = 1;
			vkci.imageUsage = usageFlags;
			vkci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vkci.queueFamilyIndexCount = 0;
			vkci.pQueueFamilyIndices = nullptr;
			vkci.preTransform = preTransform;
			vkci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			vkci.presentMode = presentMode;
			vkci.clipped = VK_FALSE;
			vkci.oldSwapchain = VK_NULL_HANDLE;

			VkResult vkres = vkCreateSwapchainKHR(m_render.GetVkDevice(), &vkci, nullptr, &m_swapChain);
			if (vkres != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vkCreateSwapchainKHR failed: " + ToString(vkres));
				return false;
			}

			uint32_t imageCount = 0;
			vkres = vkGetSwapchainImagesKHR(m_render.GetVkDevice(), m_swapChain, &imageCount, nullptr);
			if (vkres != VK_SUCCESS)
			{
				ASSERT_MSG(false, "vkGetSwapchainImagesKHR(0) failed: " + ToString(vkres));
				return false;
			}
			Print("Vulkan swapchain image count: " + std::to_string(imageCount));

			if (imageCount > 0)
			{
				colorImages.resize(imageCount);
				vkres = vkGetSwapchainImagesKHR(m_render.GetVkDevice(), m_swapChain, &imageCount, colorImages.data());
				if (vkres != VK_SUCCESS)
				{
					ASSERT_MSG(false, "vkGetSwapchainImagesKHR(1) failed: " + ToString(vkres));
					return false;
				}
			}
		}
#else
		{
			vkb::SwapchainBuilder swapChainBuilder{ m_render.GetVkPhysicalDevice(), m_render.GetVkDevice(), m_render.GetVkSurface() };

			const VkSurfaceFormatKHR surfaceFormat
			{
				.format = m_colorFormat,
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
		}
#endif
		// Transition images from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR.
		{
			VkImageLayout newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			for (uint32_t i = 0; i < colorImages.size(); ++i) 
			{
				VkResult vkres = createInfo.queue->TransitionImageLayout(
					colorImages[i],                     // image
					VK_IMAGE_ASPECT_COLOR_BIT,          // aspectMask
					0,                                  // baseMipLevel
					1,                                  // levelCount
					0,                                  // baseArrayLayer
					createInfo.arrayLayerCount,       // layerCount
					VK_IMAGE_LAYOUT_UNDEFINED,          // oldLayout
					newLayout,                          // newLayout
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT); // newPipelineStage
				if (vkres != VK_SUCCESS)
				{
					ASSERT_MSG(false, "Queue::TransitionImageLayout failed: " + ToString(vkres));
					return false;
				}
			}
		}

		// Create images.
		{
			for (uint32_t i = 0; i < colorImages.size(); ++i)
			{
				ImageCreateInfo imageCreateInfo = {};
				imageCreateInfo.type = ImageType::Image2D;
				imageCreateInfo.width = createInfo.width;
				imageCreateInfo.height = createInfo.height;
				imageCreateInfo.depth = 1;
				imageCreateInfo.format = createInfo.colorFormat;
				imageCreateInfo.sampleCount = SampleCount::Sample1;
				imageCreateInfo.mipLevelCount = 1;
				imageCreateInfo.arrayLayerCount = createInfo.arrayLayerCount;
				imageCreateInfo.usageFlags.bits.transferSrc = true;
				imageCreateInfo.usageFlags.bits.transferDst = true;
				imageCreateInfo.usageFlags.bits.sampled = true;
				imageCreateInfo.usageFlags.bits.storage = true;
				imageCreateInfo.usageFlags.bits.colorAttachment = true;
				imageCreateInfo.ApiObject = (void*)(colorImages[i]);

				ImagePtr image;
				Result ppxres = m_render.GetRenderDevice().CreateImage(imageCreateInfo, &image);
				if (Failed(ppxres))
				{
					ASSERT_MSG(false, "image create failed");
					return false;
				}

				m_colorImages.push_back(image);
			}

			for (size_t i = 0; i < depthImages.size(); ++i)
			{
				ImageCreateInfo imageCreateInfo = ImageCreateInfo::DepthStencilTarget(createInfo.width, createInfo.height, createInfo.depthFormat, SampleCount::Sample1);
				imageCreateInfo.ApiObject = (void*)(depthImages[i]);
				imageCreateInfo.arrayLayerCount = createInfo.arrayLayerCount;
				ImagePtr image;
				Result ppxres = m_render.GetRenderDevice().CreateImage(imageCreateInfo, &image);
				if (Failed(ppxres))
				{
					ASSERT_MSG(false, "image create failed");
					return false;
				}

				m_depthImages.push_back(image);
			}
		}

		// Save queue for presentation.
		m_queue = createInfo.queue;
	}


	// Update the stored create info's image count since the actual number of images might be different (hopefully more) than what was originally requested.
	m_createInfo.imageCount = CountU32(m_colorImages);

	if (m_createInfo.imageCount != createInfo.imageCount)
	{
		Print(std::string("Swapchain actual image count is different from what was requested\n")
			+ "   actual    : " + std::to_string(createInfo.imageCount) + "\n"
			+ "   requested : " + std::to_string(createInfo.imageCount));
	}

	// NOTE: mCreateInfo will be used from this point on.

	// Create color images if needed. This is only needed if we're creating a headless swapchain.
	if (m_colorImages.empty())
	{
		for (uint32_t i = 0; i < m_createInfo.imageCount; ++i)
		{
			ImageCreateInfo rtCreateInfo = ImageCreateInfo::RenderTarget2D(m_createInfo.width, m_createInfo.height, m_createInfo.colorFormat);
			rtCreateInfo.ownership = Ownership::Restricted;
			rtCreateInfo.RTVClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
			rtCreateInfo.initialState = ResourceState::Present;
			rtCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;
			rtCreateInfo.usageFlags =
				IMAGE_USAGE_COLOR_ATTACHMENT |
				IMAGE_USAGE_TRANSFER_SRC |
				IMAGE_USAGE_TRANSFER_DST |
				IMAGE_USAGE_SAMPLED;

			ImagePtr renderTarget;
			auto ppxres = m_render.GetRenderDevice().CreateImage(rtCreateInfo, &renderTarget);
			if (Failed(ppxres)) {
				return false; // TODO: error
			}

			m_colorImages.push_back(renderTarget);
		}
	}

	// Create depth images if needed. This is usually needed for both normal swapchains and headless swapchains, but not needed for XR swapchains which create their own depth images.
	auto ppxres = createDepthImages();
	if (Failed(ppxres)) {
		return false; // TODO: error
	}

	ppxres = createRenderTargets();
	if (Failed(ppxres)) {
		return false; // TODO: error
	}

	ppxres = createRenderPasses();
	if (Failed(ppxres)) {
		return false; // TODO: error
	}

	Print("Swapchain created");
	Print("   " + std::string("resolution  : ") + std::to_string(m_createInfo.width) + "x" + std::to_string(m_createInfo.height));
	Print("   " + std::string("image count : ") + std::to_string(m_createInfo.imageCount));

	return true;
}

void VulkanSwapChain::Shutdown2()
{
	destroyRenderPasses();
	destroyRenderTargets();
	destroyDepthImages();
	destroyColorImages();

	if (m_swapChain)
		vkDestroySwapchainKHR(m_render.GetVkDevice(), m_swapChain, nullptr);
}

Result VulkanSwapChain::GetColorImage(uint32_t imageIndex, Image** ppImage) const
{
	if (!IsIndexInRange(imageIndex, m_colorImages))
	{
		return ERROR_OUT_OF_RANGE;
	}
	*ppImage = m_colorImages[imageIndex];
	return SUCCESS;
}

Result VulkanSwapChain::GetDepthImage(uint32_t imageIndex, Image** ppImage) const
{
	if (!IsIndexInRange(imageIndex, m_depthImages))
	{
		return ERROR_OUT_OF_RANGE;
	}
	*ppImage = m_depthImages[imageIndex];
	return SUCCESS;
}

Result VulkanSwapChain::GetRenderPass(uint32_t imageIndex, AttachmentLoadOp loadOp, RenderPass** ppRenderPass) const
{
	if (!IsIndexInRange(imageIndex, m_clearRenderPasses))
	{
		return ERROR_OUT_OF_RANGE;
	}
	if (loadOp == AttachmentLoadOp::Clear)
	{
		*ppRenderPass = m_clearRenderPasses[imageIndex];
	}
	else {
		*ppRenderPass = m_loadRenderPasses[imageIndex];
	}
	return SUCCESS;
}

Result VulkanSwapChain::GetRenderTargetView(uint32_t imageIndex, AttachmentLoadOp loadOp, RenderTargetView** ppView) const
{
	if (!IsIndexInRange(imageIndex, m_clearRenderTargets))
	{
		return ERROR_OUT_OF_RANGE;
	}
	if (loadOp == AttachmentLoadOp::Clear)
	{
		*ppView = m_clearRenderTargets[imageIndex];
	}
	else
	{
		*ppView = m_loadRenderTargets[imageIndex];
	}
	return SUCCESS;
}

Result VulkanSwapChain::GetDepthStencilView(uint32_t imageIndex, AttachmentLoadOp loadOp, DepthStencilView** ppView) const
{
	if (!IsIndexInRange(imageIndex, m_clearDepthStencilViews))
	{
		return ERROR_OUT_OF_RANGE;
	}
	if (loadOp == AttachmentLoadOp::Clear)
	{
		*ppView = m_clearDepthStencilViews[imageIndex];
	}
	else {
		*ppView = m_loadDepthStencilViews[imageIndex];
	}
	return SUCCESS;
}

ImagePtr VulkanSwapChain::GetColorImage(uint32_t imageIndex) const
{
	ImagePtr object;
	GetColorImage(imageIndex, &object);
	return object;
}

ImagePtr VulkanSwapChain::GetDepthImage(uint32_t imageIndex) const
{
	ImagePtr object;
	GetDepthImage(imageIndex, &object);
	return object;
}

RenderPassPtr VulkanSwapChain::GetRenderPass(uint32_t imageIndex, AttachmentLoadOp loadOp) const
{
	RenderPassPtr object;
	GetRenderPass(imageIndex, loadOp, &object);
	return object;
}

RenderTargetViewPtr VulkanSwapChain::GetRenderTargetView(uint32_t imageIndex, AttachmentLoadOp loadOp) const
{
	RenderTargetViewPtr object;
	GetRenderTargetView(imageIndex, loadOp, &object);
	return object;
}

DepthStencilViewPtr VulkanSwapChain::GetDepthStencilView(uint32_t imageIndex, AttachmentLoadOp loadOp) const
{
	DepthStencilViewPtr object;
	GetDepthStencilView(imageIndex, loadOp, &object);
	return object;
}

Result VulkanSwapChain::AcquireNextImage(uint64_t timeout, Semaphore* pSemaphore, Fence* pFence, uint32_t* pImageIndex)
{
	return acquireNextImageInternal(timeout, pSemaphore, pFence, pImageIndex);
}

Result VulkanSwapChain::Present(uint32_t imageIndex, uint32_t waitSemaphoreCount, const Semaphore* const* ppWaitSemaphores)
{
	return presentInternal(imageIndex, waitSemaphoreCount, ppWaitSemaphores);
}

void VulkanSwapChain::destroyColorImages()
{
	for (auto& elem : m_colorImages)
	{
		if (elem)
		{
			m_render.GetRenderDevice().DestroyImage(elem);
		}
	}
	m_colorImages.clear();
}

Result VulkanSwapChain::createDepthImages()
{
	if ((m_createInfo.depthFormat != Format::Undefined) && m_depthImages.empty())
	{
		for (uint32_t i = 0; i < m_createInfo.imageCount; ++i)
		{
			ImageCreateInfo dpCreateInfo = ImageCreateInfo::DepthStencilTarget(m_createInfo.width, m_createInfo.height, m_createInfo.depthFormat);
			dpCreateInfo.ownership = Ownership::Restricted;
			dpCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;
			dpCreateInfo.DSVClearValue = { 1.0f, 0xFF };

			ImagePtr depthStencilTarget;
			auto ppxres = m_render.GetRenderDevice().CreateImage(dpCreateInfo, &depthStencilTarget);
			if (Failed(ppxres))
			{
				return ppxres;
			}

			m_depthImages.push_back(depthStencilTarget);
		}
	}

	return SUCCESS;
}

void VulkanSwapChain::destroyDepthImages()
{
	for (auto& elem : m_depthImages)
	{
		if (elem) {
			m_render.GetRenderDevice().DestroyImage(elem);
		}
	}
	m_depthImages.clear();
}

Result VulkanSwapChain::createRenderPasses()
{
	uint32_t imageCount = CountU32(m_colorImages);
	ASSERT_MSG((imageCount > 0), "No color images found for swapchain renderpasses");

	// Create render passes with AttachmentLoadOp::Clear for render target.
	for (size_t i = 0; i < imageCount; ++i)
	{
		RenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.width = m_createInfo.width;
		rpCreateInfo.height = m_createInfo.height;
		rpCreateInfo.renderTargetCount = 1;
		rpCreateInfo.renderTargetViews[0] = m_clearRenderTargets[i];
		rpCreateInfo.depthStencilView = m_depthImages.empty() ? nullptr : m_clearDepthStencilViews[i];
		rpCreateInfo.renderTargetClearValues[0] = {0.0f, 0.0f, 0.0f, 0.0f};
		rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
		rpCreateInfo.ownership = Ownership::Restricted;
		rpCreateInfo.shadingRatePattern = m_createInfo.shadingRatePattern;
		rpCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

		RenderPassPtr renderPass;
		auto ppxres = m_render.GetRenderDevice().CreateRenderPass(rpCreateInfo, &renderPass);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "Swapchain::CreateRenderPass(CLEAR) failed");
			return ppxres;
		}

		m_clearRenderPasses.push_back(renderPass);
	}

	// Create render passes with AttachmentLoadOp::Load for render target.
	for (size_t i = 0; i < imageCount; ++i)
	{
		RenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.width = m_createInfo.width;
		rpCreateInfo.height = m_createInfo.height;
		rpCreateInfo.renderTargetCount = 1;
		rpCreateInfo.renderTargetViews[0] = m_loadRenderTargets[i];
		rpCreateInfo.depthStencilView = m_depthImages.empty() ? nullptr : m_loadDepthStencilViews[i];
		rpCreateInfo.renderTargetClearValues[0] = {0.0f, 0.0f, 0.0f, 0.0f};
		rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
		rpCreateInfo.ownership = Ownership::Restricted;
		rpCreateInfo.shadingRatePattern = m_createInfo.shadingRatePattern;

		RenderPassPtr renderPass;
		auto ppxres = m_render.GetRenderDevice().CreateRenderPass(rpCreateInfo, &renderPass);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "Swapchain::CreateRenderPass(LOAD) failed");
			return ppxres;
		}

		m_loadRenderPasses.push_back(renderPass);
	}

	return SUCCESS;
}

void VulkanSwapChain::destroyRenderPasses()
{
	for (auto& elem : m_clearRenderPasses)
	{
		if (elem)
		{
			m_render.GetRenderDevice().DestroyRenderPass(elem);
		}
	}
	m_clearRenderPasses.clear();

	for (auto& elem : m_loadRenderPasses)
	{
		if (elem)
		{
			m_render.GetRenderDevice().DestroyRenderPass(elem);
		}
	}
	m_loadRenderPasses.clear();
}

Result VulkanSwapChain::createRenderTargets()
{
	uint32_t imageCount = CountU32(m_colorImages);
	ASSERT_MSG((imageCount > 0), "No color images found for swapchain renderpasses");
	for (size_t i = 0; i < imageCount; ++i)
	{
		auto imagePtr = m_colorImages[i];
		RenderTargetViewCreateInfo rtvCreateInfo = RenderTargetViewCreateInfo::GuessFromImage(imagePtr);
		rtvCreateInfo.loadOp = AttachmentLoadOp::Clear;
		rtvCreateInfo.ownership = Ownership::Restricted;
		rtvCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

		RenderTargetViewPtr rtv;
		Result ppxres = m_render.GetRenderDevice().CreateRenderTargetView(rtvCreateInfo, &rtv);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for LOAD_OP_CLEAR failed");
			return ppxres;
		}
		m_clearRenderTargets.push_back(rtv);

		rtvCreateInfo.loadOp = AttachmentLoadOp::Load;
		ppxres = m_render.GetRenderDevice().CreateRenderTargetView(rtvCreateInfo, &rtv);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for LOAD_OP_LOAD failed");
			return ppxres;
		}
		m_loadRenderTargets.push_back(rtv);

		if (!m_depthImages.empty())
		{
			auto depthImage = m_depthImages[i];
			DepthStencilViewCreateInfo dsvCreateInfo = DepthStencilViewCreateInfo::GuessFromImage(depthImage);
			dsvCreateInfo.depthLoadOp = AttachmentLoadOp::Clear;
			dsvCreateInfo.stencilLoadOp = AttachmentLoadOp::Clear;
			dsvCreateInfo.ownership = Ownership::Restricted;
			dsvCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

			DepthStencilViewPtr clearDsv;
			ppxres = m_render.GetRenderDevice().CreateDepthStencilView(dsvCreateInfo, &clearDsv);
			if (Failed(ppxres))
			{
				ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for depth stencil view failed");
				return ppxres;
			}

			m_clearDepthStencilViews.push_back(clearDsv);

			dsvCreateInfo.depthLoadOp = AttachmentLoadOp::Load;
			dsvCreateInfo.stencilLoadOp = AttachmentLoadOp::Load;
			DepthStencilViewPtr loadDsv;
			ppxres = m_render.GetRenderDevice().CreateDepthStencilView(dsvCreateInfo, &loadDsv);
			if (Failed(ppxres))
			{
				ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for depth stencil view failed");
				return ppxres;
			}

			m_loadDepthStencilViews.push_back(loadDsv);
		}
	}

	return SUCCESS;
}

void VulkanSwapChain::destroyRenderTargets()
{
	for (auto& rtv : m_clearRenderTargets)
	{
		m_render.GetRenderDevice().DestroyRenderTargetView(rtv);
	}
	m_clearRenderTargets.clear();
	for (auto& rtv : m_loadRenderTargets)
	{
		m_render.GetRenderDevice().DestroyRenderTargetView(rtv);
	}
	m_loadRenderTargets.clear();
	for (auto& rtv : m_clearDepthStencilViews)
	{
		m_render.GetRenderDevice().DestroyDepthStencilView(rtv);
	}
	m_clearDepthStencilViews.clear();
	for (auto& rtv : m_loadDepthStencilViews)
	{
		m_render.GetRenderDevice().DestroyDepthStencilView(rtv);
	}
	m_loadDepthStencilViews.clear();
}

Result VulkanSwapChain::acquireNextImageInternal(uint64_t timeout, Semaphore* pSemaphore, Fence* pFence, uint32_t* pImageIndex)
{
	VkSemaphore semaphore = VK_NULL_HANDLE;
	if (!IsNull(pSemaphore))
	{
		semaphore = pSemaphore->GetVkSemaphore();
	}

	VkFence fence = VK_NULL_HANDLE;
	if (!IsNull(pFence))
	{
		fence = pFence->GetVkFence();
	}

	VkResult vkres = vkAcquireNextImageKHR(m_render.GetVkDevice(), m_swapChain, timeout, semaphore, fence, pImageIndex);

	// Handle failure cases
	if (vkres < VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkAcquireNextImageKHR failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}
	// Handle warning cases
	if (vkres > VK_SUCCESS)
	{
		Warning("vkAcquireNextImageKHR returned: " + ToString(vkres));
	}

	m_currentImageIndex = *pImageIndex;

	return SUCCESS;
}

Result VulkanSwapChain::presentInternal(uint32_t imageIndex, uint32_t waitSemaphoreCount, const Semaphore* const* ppWaitSemaphores)
{
	std::vector<VkSemaphore> semaphores;
	for (uint32_t i = 0; i < waitSemaphoreCount; ++i)
	{
		VkSemaphore semaphore = ppWaitSemaphores[i]->GetVkSemaphore();
		semaphores.push_back(semaphore);
	}

	VkPresentInfoKHR vkpi = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	vkpi.waitSemaphoreCount = CountU32(semaphores);
	vkpi.pWaitSemaphores = DataPtr(semaphores);
	vkpi.swapchainCount = 1;
	vkpi.pSwapchains = &m_swapChain;
	vkpi.pImageIndices = &imageIndex;
	vkpi.pResults = nullptr;

	VkResult vkres = vkQueuePresentKHR(m_queue->GetQueue()->Queue, &vkpi);
	// Handle failure cases
	if (vkres < VK_SUCCESS)
	{
		ASSERT_MSG(false, "vkQueuePresentKHR failed: " + ToString(vkres));
		return ERROR_API_FAILURE;
	}
	// Handle warning cases
	if (vkres > VK_SUCCESS)
	{
		Warning("vkQueuePresentKHR returned: " + ToString(vkres));
	}

	return SUCCESS;
}


// OLD =>

bool VulkanSwapChain::Resize(uint32_t width, uint32_t height)
{



	Close();

	vkb::SwapchainBuilder swapChainBuilder{ m_render.GetVkPhysicalDevice(), m_render.GetVkDevice(), m_render.GetVkSurface() };

	const VkSurfaceFormatKHR surfaceFormat
	{
		.format = m_colorFormat,
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

void VulkanSwapChain::Close()
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

VkFormat VulkanSwapChain::GetFormat()
{
	return m_colorFormat;
}

VkImageView& VulkanSwapChain::GetImageView(size_t i)
{
	assert(i < m_swapChainImageViews.size());
	return m_swapChainImageViews[i];
}

#pragma endregion

#pragma region ImGui

ImGuiImpl::ImGuiImpl(RenderSystem& render)
	: m_render(render)
{
}

bool ImGuiImpl::Setup(bool enableImGuiDynamicRendering)
{
	m_enableImGuiDynamicRendering = enableImGuiDynamicRendering;

	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.LogFilename = nullptr;
	io.IniFilename = nullptr;

	float fontSize = 16.0f;
#if defined(_WIN32)
	HWND activeWindow = GetActiveWindow();
	HMONITOR monitor = MonitorFromWindow(activeWindow, MONITOR_DEFAULTTONEAREST);

	DEVICE_SCALE_FACTOR scale = SCALE_100_PERCENT;
	HRESULT hr = GetScaleFactorForMonitor(monitor, &scale);
	if (FAILED(hr)) return false;

	float fontScale = 1.0f;
	switch (scale) {
	default: break;
	case SCALE_120_PERCENT: fontScale = 1.20f; break;
	case SCALE_125_PERCENT: fontScale = 1.25f; break;
	case SCALE_140_PERCENT: fontScale = 1.40f; break;
	case SCALE_150_PERCENT: fontScale = 1.50f; break;
	case SCALE_160_PERCENT: fontScale = 1.60f; break;
	case SCALE_175_PERCENT: fontScale = 1.75f; break;
	case SCALE_180_PERCENT: fontScale = 1.80f; break;
	case SCALE_200_PERCENT: fontScale = 2.00f; break;
	case SCALE_225_PERCENT: fontScale = 2.25f; break;
	case SCALE_250_PERCENT: fontScale = 2.50f; break;
	case SCALE_300_PERCENT: fontScale = 3.00f; break;
	case SCALE_350_PERCENT: fontScale = 3.50f; break;
	case SCALE_400_PERCENT: fontScale = 4.00f; break;
	case SCALE_450_PERCENT: fontScale = 4.50f; break;
	case SCALE_500_PERCENT: fontScale = 5.00f; break;
	}	
	fontSize *= fontScale;
#endif

	ImFontConfig fontConfig = {};
	fontConfig.FontDataOwnedByAtlas = false;

	ImFont* pFont = io.Fonts->AddFontFromMemoryTTF(
		const_cast<void*>(static_cast<const void*>(imgui::kFontInconsolata)),
		static_cast<int>(imgui::kFontInconsolataSize),
		fontSize,
		&fontConfig);

	ASSERT_MSG(!IsNull(pFont), "imgui add font failed");

	Result ppxres = initApiObjects();
	if (Failed(ppxres)) return false;

	return true;
}

void ImGuiImpl::Shutdown()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (m_pool)
	{
		m_render.GetRenderDevice().DestroyDescriptorPool(m_pool);
		m_pool.Reset();
	}
}

void ImGuiImpl::NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize.x = static_cast<float>(m_render.GetUIWidth());
	io.DisplaySize.y = static_cast<float>(m_render.GetUIHeight());
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiImpl::Render(CommandBuffer* pCommandBuffer)
{
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pCommandBuffer->GetVkCommandBuffer());
}

void ImGuiImpl::ProcessEvents()
{
}

Result ImGuiImpl::initApiObjects()
{
	// Setup GLFW binding
	GLFWwindow* pWindow = m_render.GetEngine().GetWindow().GetWindow();
	ImGui_ImplGlfw_InitForVulkan(pWindow, false);

	// Setup style
	setColorStyle();

	// Create descriptor pool
	{
		DescriptorPoolCreateInfo ci = {};
		ci.combinedImageSampler = 1;

		Result ppxres = m_render.GetRenderDevice().CreateDescriptorPool(ci, &m_pool);
		if (Failed(ppxres)) return ppxres;
	}

	// Setup Vulkan binding
	{
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = m_render.GetVkInstance();
		init_info.PhysicalDevice = m_render.GetVkPhysicalDevice();
		init_info.Device = m_render.GetVkDevice();
		init_info.QueueFamily = m_render.GetVkGraphicsQueue()->QueueFamily;
		init_info.Queue = m_render.GetVkGraphicsQueue()->Queue;
		init_info.PipelineCache = VK_NULL_HANDLE;
		init_info.DescriptorPool = m_pool->GetVkDescriptorPool();
		init_info.MinImageCount = m_render.GetSwapChain().GetImageCount();
		init_info.ImageCount = m_render.GetSwapChain().GetImageCount();
		init_info.Allocator = VK_NULL_HANDLE;
		init_info.CheckVkResultFn = nullptr;
#if defined(IMGUI_IMPL_VULKAN_HAS_DYNAMIC_RENDERING)
		init_info.UseDynamicRendering = m_enableImGuiDynamicRendering;
		init_info.PipelineRenderingCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
		VkFormat colorFormat = ToVkEnum(m_render.GetSwapChain().GetColorFormat());
		init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
		init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
#else
		ASSERT_MSG(!pApp->GetSettings()->grfx.enableImGuiDynamicRendering, "This version of ImGui does not have dynamic rendering support");
#endif

		RenderPassPtr renderPass = m_render.GetSwapChain().GetRenderPass(0, AttachmentLoadOp::Load);
		ASSERT_MSG(!renderPass.IsNull(), "[imgui:vk] failed to get swapchain renderpass");

		init_info.RenderPass = renderPass->GetVkRenderPass();
		bool result = ImGui_ImplVulkan_Init(&init_info);
		if (!result) return ERROR_IMGUI_INITIALIZATION_FAILED;
	}

	// Upload Fonts
	{
		// Create command buffer
		CommandBufferPtr commandBuffer;
		Result ppxres = m_render.GetGraphicsQueue()->CreateCommandBuffer(&commandBuffer, 0, 0);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "[imgui:vk] command buffer create failed");
			return ppxres;
		}

		// Begin command buffer
		ppxres = commandBuffer->Begin();
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "[imgui:vk] command buffer begin failed");
			return ppxres;
		}

		// End command buffer
		ppxres = commandBuffer->End();
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "[imgui:vk] command buffer end failed");
			return ppxres;
		}

		// Submit
		SubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.ppCommandBuffers = &commandBuffer;

		ppxres = m_render.GetGraphicsQueue()->Submit(&submitInfo);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "[imgui:vk] command buffer submit failed");
			return ppxres;
		}

		// Wait for idle
		ppxres = m_render.GetGraphicsQueue()->WaitIdle();
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "[imgui:vk] queue wait idle failed");
			return ppxres;
		}

		// Destroy command buffer
		m_render.GetGraphicsQueue()->DestroyCommandBuffer(commandBuffer);
	}

	return SUCCESS;
}

void ImGuiImpl::setColorStyle()
{
	// ImGui::StyleColorsClassic();
	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();
}

#pragma endregion

#pragma region RenderSystem

RenderSystem::RenderSystem(EngineApplication& engine)
	: m_engine(engine)
{
}

bool RenderSystem::Setup(const RenderCreateInfo& createInfo)
{
	m_showImgui = createInfo.showImgui;

	if (!m_instance.Setup(createInfo.instance, m_engine.GetWindow().GetWindow()))
		return false;
	if (!m_device.Setup(createInfo.instance.supportShadingRateMode))
		return false;
	if (!m_surface.Setup()) return false;
	if (!createSwapChains(createInfo.swapChain)) return false;
	if (!m_imgui.Setup(createInfo.enableImGuiDynamicRendering))
		return false;
		
	return true;
}

void RenderSystem::Shutdown()
{
	WaitIdle();

	m_imgui.Shutdown();
	m_swapChain.Shutdown2();
	m_device.Shutdown();
	m_instance.Shutdown();
}

void RenderSystem::Update()
{
	// Update imgui
	if (m_showImgui) m_imgui.ProcessEvents();
}

void RenderSystem::TestDraw()
{
	// Vulkan will return an error if either dimension is 1
	if ((m_engine.GetWindowWidth() <= 1) || (m_engine.GetWindowHeight() <= 1))
		return;

	if (!m_engine.IsWindowIconified() && m_showImgui)
		m_imgui.NewFrame();
}

void RenderSystem::DrawDebugInfo()
{
	if (!m_showImgui) return;

	const uint32_t kImGuiMinWidth = 400;
	const uint32_t kImGuiMinHeight = 300;

	uint32_t minWidth = std::min(kImGuiMinWidth, GetUIWidth() / 2);
	uint32_t minHeight = std::min(kImGuiMinHeight, GetUIHeight() / 2);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { static_cast<float>(minWidth), static_cast<float>(minHeight) });
	if (ImGui::Begin("Debug Info")) {
		ImGui::Columns(2);
		
		// Application PID
		{
			ImGui::Text("Application PID");
			ImGui::NextColumn();
			ImGui::Text("%d", GetCurrentProcessId());
			ImGui::NextColumn();
		}

		ImGui::Separator();

		// GPU
		{
			ImGui::Text("GPU");
			ImGui::NextColumn();
			ImGui::Text("%s", m_instance.GetDeviceName());
			ImGui::NextColumn();
		}

		ImGui::Separator();

		// Swapchain
		{
			ImGui::Text("Swapchain Resolution");
			ImGui::NextColumn();
			ImGui::Text("%dx%d", m_swapChain.GetWidth(), m_swapChain.GetHeight());
			ImGui::NextColumn();

			ImGui::Text("Swapchain Image Count");
			ImGui::NextColumn();
			ImGui::Text("%d", m_swapChain.GetImageCount());
			ImGui::NextColumn();
		}

		ImGui::Columns(1);

		// Draw additional elements
		// TODO: user draw ui
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

void RenderSystem::DrawImGui(CommandBuffer* pCommandBuffer)
{
	if (m_showImgui) m_imgui.Render(pCommandBuffer);
}

Rect RenderSystem::GetScissor() const
{
	Rect rect = {};
	rect.x = 0;
	rect.y = 0;
	rect.width = m_engine.GetWindowWidth();
	rect.height = m_engine.GetWindowHeight();
	return rect;
}

Viewport RenderSystem::GetViewport(float minDepth, float maxDepth) const
{
	Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_engine.GetWindowWidth());
	viewport.height = static_cast<float>(m_engine.GetWindowHeight());
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

uint32_t RenderSystem::GetUIWidth() const
{
	return m_engine.GetWindowWidth();
}

uint32_t RenderSystem::GetUIHeight() const
{
	return m_engine.GetWindowHeight();
}

float2 RenderSystem::GetNormalizedDeviceCoordinates(int32_t x, int32_t y) const
{
	float  fx = x / static_cast<float>(m_engine.GetWindowWidth());
	float  fy = y / static_cast<float>(m_engine.GetWindowHeight());
	float2 ndc = float2(2.0f, -2.0f) * (float2(fx, fy) - float2(0.5f));
	return ndc;
}

Result RenderSystem::WaitIdle()
{
	VkResult vkres = vkDeviceWaitIdle(m_instance.device);
	if (vkres != VK_SUCCESS) return ERROR_API_FAILURE;
	return SUCCESS;
}

bool RenderSystem::createSwapChains(const SwapChainCreateInfo& swapChain)
{
	m_swapChainInfo = swapChain;

	VulkanSwapChainCreateInfo ci = {};
	ci.queue = m_device.GetGraphicsQueue();
	ci.surface = &m_surface;
	ci.width = m_engine.GetWindowWidth();
	ci.height = m_engine.GetWindowHeight();
	ci.colorFormat = swapChain.colorFormat;
	ci.depthFormat = swapChain.depthFormat;
	ci.imageCount = swapChain.imageCount;
	ci.presentMode = PRESENT_MODE_IMMEDIATE;

	// m_surface
	{
		const uint32_t surfaceMinImageCount = m_surface.GetMinImageCount();
		if (swapChain.imageCount < surfaceMinImageCount)
		{
			Warning("readjusting swapchain's image count from " + std::to_string(swapChain.imageCount) + " to " + std::to_string(surfaceMinImageCount) + " to match surface requirements");
			ci.imageCount = surfaceMinImageCount;
		}

		// Cap the image width/height to what the surface caps are.
		// The reason for this is that on Windows the TaskBar affect the maximum size of the window if it has borders.
		// For example an application can request a 1920x1080 window but because of the task bar, the window may get created at 1920x1061. This limits the swapchain's max image extents to 1920x1061.
		const uint32_t surfaceMaxImageWidth = m_surface.GetMaxImageWidth();
		const uint32_t surfaceMaxImageHeight = m_surface.GetMaxImageHeight();
		if ((ci.width > surfaceMaxImageWidth) || (ci.height > surfaceMaxImageHeight))
		{
			Warning("readjusting swapchain/window size from " + std::to_string(ci.width) + "x" + std::to_string(ci.height) + " to " + std::to_string(surfaceMaxImageWidth) + "x" + std::to_string(surfaceMaxImageHeight) + " to match surface requirements");
			//m_engine.GetWindowWidth() = std::min(m_engine.GetWindowWidth(), surfaceMaxImageWidth); // TODO:
			//m_engine.GetWindowHeight() = std::min(m_engine.GetWindowHeight(), surfaceMaxImageHeight);
		}

		const uint32_t surfaceCurrentImageWidth = m_surface.GetCurrentImageWidth();
		const uint32_t surfaceCurrentImageHeight = m_surface.GetCurrentImageHeight();
		if ((surfaceCurrentImageWidth != VulkanSurface::kInvalidExtent) && (surfaceCurrentImageHeight != VulkanSurface::kInvalidExtent))
		{
			if ((ci.width != surfaceCurrentImageWidth) || (ci.height != surfaceCurrentImageHeight))
			{
				Warning("window size " + std::to_string(ci.width) + "x" + std::to_string(ci.height) + " does not match current surface extent " + std::to_string(surfaceCurrentImageWidth) + "x" + std::to_string(surfaceCurrentImageHeight));
			}
			Warning("surface current extent " + std::to_string(surfaceCurrentImageWidth) + "x" + std::to_string(surfaceCurrentImageHeight));
		}
	}

	Print("Creating application swapchain");
	Print("   resolution  : " + std::to_string(ci.width) + "x" + std::to_string(ci.height));
	Print("   image count : " + std::to_string(swapChain.imageCount));

	if (!m_swapChain.Setup(ci))
	{
		ASSERT_MSG(false, "RenderSystem::createSwapChains failed");
		return false;
	}


	return true;
}

void RenderSystem::resize()
{
	// Vulkan will return an error if either dimension is 1
	if ((m_engine.GetWindowWidth() <= 1) || (m_engine.GetWindowHeight() <= 1))
		return;

	// TODO: пересоздавать свапчаин нужно по окончании ресайза, а не каждый его пиксель.  и тогда тут надо ловить этот момент и выбрасывать из функции если ресайз еще в процессе

	vkDeviceWaitIdle(m_instance.device);
	m_swapChain.Shutdown2();
	if (!createSwapChains(m_swapChainInfo)) Fatal("Vulkan swapchain recreate failed");
}

#pragma endregion

} // namespace vkr