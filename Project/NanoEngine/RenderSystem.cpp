#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"
#include "Application.h"

#pragma region VulkanInstance

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

	Print("GPU Used: " + std::string(physicalDeviceProperties.deviceName));

	if (!getQueues(vkbDevice)) return false;
	if (!initVma()) return false;
	if (!getDeviceInfo()) return false;

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
		.require_api_version(createInfo.requireVersion)
		.enable_extensions(createInfo.instanceExtensions)
		.request_validation_layers(useValidationLayers)
		.use_default_debug_messenger()
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
	, m_surface(render)
{
}

bool VulkanSwapChain::Setup(const SwapChainCreateInfo& createInfo)
{
	m_createInfo = createInfo;

	ASSERT_NULL_ARG(createInfo.queue);
	if (IsNull(createInfo.queue)) return false;

	m_surface.Setup();

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
				const VkSurfaceCapabilitiesKHR& caps = m_surface.GetCapabilities();
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
				VkFormat format = ToVkFormat(m_createInfo.colorFormat);
				if (format == VK_FORMAT_UNDEFINED)
				{
					ASSERT_MSG(false, "Invalid swapchain format");
					return false;
				}

				const std::vector<VkSurfaceFormatKHR>& surfaceFormats = m_surface .GetSurfaceFormats();
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
			VkPresentModeKHR presentMode = ToVkPresentMode(createInfo.presentMode);
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
				imageCreateInfo.type = IMAGE_TYPE_2D;
				imageCreateInfo.width = createInfo.width;
				imageCreateInfo.height = createInfo.height;
				imageCreateInfo.depth = 1;
				imageCreateInfo.format = createInfo.colorFormat;
				imageCreateInfo.sampleCount = SAMPLE_COUNT_1;
				imageCreateInfo.mipLevelCount = 1;
				imageCreateInfo.arrayLayerCount = createInfo.arrayLayerCount;
				imageCreateInfo.usageFlags.bits.transferSrc = true;
				imageCreateInfo.usageFlags.bits.transferDst = true;
				imageCreateInfo.usageFlags.bits.sampled = true;
				imageCreateInfo.usageFlags.bits.storage = true;
				imageCreateInfo.usageFlags.bits.colorAttachment = true;
				imageCreateInfo.pApiObject = (void*)(colorImages[i]);

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
				ImageCreateInfo imageCreateInfo = ImageCreateInfo::DepthStencilTarget(createInfo.width, createInfo.height, createInfo.depthFormat, SAMPLE_COUNT_1);
				imageCreateInfo.pApiObject = (void*)(depthImages[i]);
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
			rtCreateInfo.ownership = OWNERSHIP_RESTRICTED;
			rtCreateInfo.RTVClearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
			rtCreateInfo.initialState = RESOURCE_STATE_PRESENT;
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
	//
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
	if (loadOp == ATTACHMENT_LOAD_OP_CLEAR)
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
	if (loadOp == ATTACHMENT_LOAD_OP_CLEAR)
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
	if (loadOp == ATTACHMENT_LOAD_OP_CLEAR)
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
	if ((m_createInfo.depthFormat != FORMAT_UNDEFINED) && m_depthImages.empty())
	{
		for (uint32_t i = 0; i < m_createInfo.imageCount; ++i)
		{
			ImageCreateInfo dpCreateInfo = ImageCreateInfo::DepthStencilTarget(m_createInfo.width, m_createInfo.height, m_createInfo.depthFormat);
			dpCreateInfo.ownership = OWNERSHIP_RESTRICTED;
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

	// Create render passes with ATTACHMENT_LOAD_OP_CLEAR for render target.
	for (size_t i = 0; i < imageCount; ++i)
	{
		RenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.width = m_createInfo.width;
		rpCreateInfo.height = m_createInfo.height;
		rpCreateInfo.renderTargetCount = 1;
		rpCreateInfo.pRenderTargetViews[0] = m_clearRenderTargets[i];
		rpCreateInfo.pDepthStencilView = m_depthImages.empty() ? nullptr : m_clearDepthStencilViews[i];
		rpCreateInfo.renderTargetClearValues[0] = { {0.0f, 0.0f, 0.0f, 0.0f} };
		rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
		rpCreateInfo.ownership = OWNERSHIP_RESTRICTED;
		rpCreateInfo.pShadingRatePattern = m_createInfo.shadingRatePattern;
		rpCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

		RenderPassPtr renderPass;
		auto ppxres = m_render.GetRenderDevice().CreateRenderPass(rpCreateInfo, &renderPass);
		if (Failed(ppxres)) {
			ASSERT_MSG(false, "Swapchain::CreateRenderPass(CLEAR) failed");
			return ppxres;
		}

		m_clearRenderPasses.push_back(renderPass);
	}

	// Create render passes with ATTACHMENT_LOAD_OP_LOAD for render target.
	for (size_t i = 0; i < imageCount; ++i)
	{
		RenderPassCreateInfo rpCreateInfo = {};
		rpCreateInfo.width = m_createInfo.width;
		rpCreateInfo.height = m_createInfo.height;
		rpCreateInfo.renderTargetCount = 1;
		rpCreateInfo.pRenderTargetViews[0] = m_loadRenderTargets[i];
		rpCreateInfo.pDepthStencilView = m_depthImages.empty() ? nullptr : m_loadDepthStencilViews[i];
		rpCreateInfo.renderTargetClearValues[0] = { {0.0f, 0.0f, 0.0f, 0.0f} };
		rpCreateInfo.depthStencilClearValue = { 1.0f, 0xFF };
		rpCreateInfo.ownership = OWNERSHIP_RESTRICTED;
		rpCreateInfo.pShadingRatePattern = m_createInfo.shadingRatePattern;

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
		rtvCreateInfo.loadOp = ATTACHMENT_LOAD_OP_CLEAR;
		rtvCreateInfo.ownership = OWNERSHIP_RESTRICTED;
		rtvCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

		RenderTargetViewPtr rtv;
		Result ppxres = m_render.GetRenderDevice().CreateRenderTargetView(rtvCreateInfo, &rtv);
		if (Failed(ppxres))
		{
			ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for LOAD_OP_CLEAR failed");
			return ppxres;
		}
		m_clearRenderTargets.push_back(rtv);

		rtvCreateInfo.loadOp = ATTACHMENT_LOAD_OP_LOAD;
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
			dsvCreateInfo.depthLoadOp = ATTACHMENT_LOAD_OP_CLEAR;
			dsvCreateInfo.stencilLoadOp = ATTACHMENT_LOAD_OP_CLEAR;
			dsvCreateInfo.ownership = OWNERSHIP_RESTRICTED;
			dsvCreateInfo.arrayLayerCount = m_createInfo.arrayLayerCount;

			DepthStencilViewPtr clearDsv;
			ppxres = m_render.GetRenderDevice().CreateDepthStencilView(dsvCreateInfo, &clearDsv);
			if (Failed(ppxres))
			{
				ASSERT_MSG(false, "Swapchain::CreateRenderTargets() for depth stencil view failed");
				return ppxres;
			}

			m_clearDepthStencilViews.push_back(clearDsv);

			dsvCreateInfo.depthLoadOp = ATTACHMENT_LOAD_OP_LOAD;
			dsvCreateInfo.stencilLoadOp = ATTACHMENT_LOAD_OP_LOAD;
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

#pragma region RenderSystem

VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;

std::vector<VkFramebuffer> framebuffers;

VkCommandPool command_pool;
std::vector<VkCommandBuffer> command_buffers;

std::vector<VulkanSemaphorePtr> availableSemaphores;
std::vector<VulkanSemaphorePtr> finishedSemaphore;

std::vector<VulkanFencePtr> inFlightFences;
std::vector<VulkanFencePtr> imageInFlight;
size_t current_frame = 0;

const int MAX_FRAMES_IN_FLIGHT = 2;

struct PerFrame
{
	CommandBufferPtr cmd;
	SemaphorePtr     imageAcquiredSemaphore;
	FencePtr         imageAcquiredFence;
	SemaphorePtr     renderCompleteSemaphore;
	FencePtr         renderCompleteFence;
};

std::vector<PerFrame>      mPerFrame;
ShaderModulePtr      mVS;
ShaderModulePtr      mPS;
PipelineInterfacePtr mPipelineInterface;
GraphicsPipelinePtr  mPipeline;
BufferPtr            mVertexBuffer;
VertexBinding        mVertexBinding;

std::optional<std::filesystem::path> GetShaderPathSuffix(const std::filesystem::path& baseName)
{
	return (std::filesystem::path("spv") / baseName).concat(".spv");
};

std::vector<char> LoadShader(const std::filesystem::path& baseDir, const std::filesystem::path& baseName)
{
	ASSERT_MSG(baseDir.is_relative(), "baseDir must be relative. Do not call GetAssetPath() on the directory.");
	ASSERT_MSG(baseName.is_relative(), "baseName must be relative. Do not call GetAssetPath() on the directory.");
	auto suffix = GetShaderPathSuffix(baseName);
	if (!suffix.has_value())
	{
		ASSERT_MSG(false, "unsupported API");
		return {};
	}

	const auto filePath = baseDir / suffix.value();
	auto       bytecode = LoadFile(filePath);
	if (!bytecode.has_value()) {
		ASSERT_MSG(false, "could not load file: " + filePath.string());
		return {};
	}

	Print("Loaded shader from " + filePath.string());
	return bytecode.value();
}

bool createFramebuffers(VkDevice device, VulkanSwapChain& swapchain)
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

bool createCommandPool(VkDevice device, DeviceQueuePtr graphicsQueue)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex = graphicsQueue->QueueFamily;

	if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) != VK_SUCCESS)
	{
		Fatal("failed to create command pool");
		return false; // failed to create command pool
	}
	return true;
}

bool createCommandBuffers(VkDevice device, VulkanSwapChain& swapchain)
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

int recreateSwapchain(VulkanSwapChain& swapchain, uint32_t width, uint32_t height, VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, DeviceQueuePtr graphicsQueue)
{
	vkDeviceWaitIdle(device);
	vkDestroyCommandPool(device, command_pool, nullptr);

	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	if (!swapchain.Resize(width, height)) return -1;
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
	if (!m_instance.Setup(createInfo.instance, m_engine.GetWindow().GetWindow()))
		return false;
	//if (!m_swapChain.Resize(m_engine.GetWindow().GetWidth(), m_engine.GetWindow().GetHeight()))
	//	return false;
	if (!m_device.Setup(createInfo.instance.supportShadingRateMode))
		return false;

	// NEW =>
	{
		// Pipeline
		{
			std::vector<char> bytecode = LoadShader("basic/shaders", "StaticVertexColors.vs");
			ASSERT_MSG(!bytecode.empty(), "VS shader bytecode load failed");
			ShaderModuleCreateInfo shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(GetRenderDevice().CreateShaderModule(shaderCreateInfo, &mVS));

			bytecode = LoadShader("basic/shaders", "StaticVertexColors.ps");
			ASSERT_MSG(!bytecode.empty(), "PS shader bytecode load failed");
			shaderCreateInfo = { static_cast<uint32_t>(bytecode.size()), bytecode.data() };
			CHECKED_CALL(GetRenderDevice().CreateShaderModule(shaderCreateInfo, &mPS));

			PipelineInterfaceCreateInfo piCreateInfo = {};
			CHECKED_CALL(GetRenderDevice().CreatePipelineInterface(piCreateInfo, &mPipelineInterface));

			mVertexBinding.AppendAttribute({ "POSITION", 0, FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });
			mVertexBinding.AppendAttribute({ "COLOR", 1, FORMAT_R32G32B32_FLOAT, 0, APPEND_OFFSET_ALIGNED, VERTEX_INPUT_RATE_VERTEX });

			GraphicsPipelineCreateInfo2 gpCreateInfo = {};
			gpCreateInfo.VS = { mVS.Get(), "vsmain" };
			gpCreateInfo.PS = { mPS.Get(), "psmain" };
			gpCreateInfo.vertexInputState.bindingCount = 1;
			gpCreateInfo.vertexInputState.bindings[0] = mVertexBinding;
			gpCreateInfo.topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			gpCreateInfo.polygonMode = POLYGON_MODE_FILL;
			gpCreateInfo.cullMode = CULL_MODE_NONE;
			gpCreateInfo.frontFace = FRONT_FACE_CCW;
			gpCreateInfo.depthReadEnable = false;
			gpCreateInfo.depthWriteEnable = false;
			gpCreateInfo.blendModes[0] = BLEND_MODE_NONE;
			gpCreateInfo.outputState.renderTargetCount = 1;
			gpCreateInfo.outputState.renderTargetFormats[0] = FORMAT_B8G8R8A8_UNORM;// GetSwapChain().GetFormat();
			gpCreateInfo.pPipelineInterface = mPipelineInterface;
			CHECKED_CALL(GetRenderDevice().CreateGraphicsPipeline(gpCreateInfo, &mPipeline));
		}

		// Per frame data
		{
			PerFrame frame = {};

			CHECKED_CALL(GetRenderDevice().GetGraphicsQueue()->CreateCommandBuffer(&frame.cmd));

			SemaphoreCreateInfo semaCreateInfo = {};
			CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &frame.imageAcquiredSemaphore));

			FenceCreateInfo fenceCreateInfo = {};
			CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &frame.imageAcquiredFence));

			CHECKED_CALL(GetRenderDevice().CreateSemaphore(semaCreateInfo, &frame.renderCompleteSemaphore));

			fenceCreateInfo = { true }; // Create signaled
			CHECKED_CALL(GetRenderDevice().CreateFence(fenceCreateInfo, &frame.renderCompleteFence));

			mPerFrame.push_back(frame);
		}

		// Buffer and geometry data
		{
			// clang-format off
			std::vector<float> vertexData = {
				// position           // vertex colors
				 0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
				-0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
				 0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,
			};
			// clang-format on
			uint32_t dataSize = SizeInBytesU32(vertexData);

			BufferCreateInfo bufferCreateInfo = {};
			bufferCreateInfo.size = dataSize;
			bufferCreateInfo.usageFlags.bits.vertexBuffer = true;
			bufferCreateInfo.memoryUsage = MEMORY_USAGE_CPU_TO_GPU;
			bufferCreateInfo.initialState = RESOURCE_STATE_VERTEX_BUFFER;

			CHECKED_CALL(GetRenderDevice().CreateBuffer(bufferCreateInfo, &mVertexBuffer));

			void* pAddr = nullptr;
			CHECKED_CALL(mVertexBuffer->MapMemory(0, &pAddr));
			memcpy(pAddr, vertexData.data(), dataSize);
			mVertexBuffer->UnmapMemory();
		}
	}

	return true;

	// OLD =>
	{
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
			availableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
			finishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
			inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
			imageInFlight.resize(m_swapChain.GetImageNum(), nullptr);

			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				inFlightFences[i] = m_device.CreateFence({ .signaled = true });
				availableSemaphores[i] = m_device.CreateSemaphore({});
				finishedSemaphore[i] = m_device.CreateSemaphore({});

				if (availableSemaphores[i] == nullptr || finishedSemaphore[i] == nullptr || inFlightFences[i] == nullptr)
				{
					Fatal("failed to create sync objects");
					return false; // failed to create synchronization objects for a frame
				}
			}
		}
	}

	return true;
}

void RenderSystem::Shutdown()
{
	vkDeviceWaitIdle(m_instance.device);

	m_device.Shutdown();

	availableSemaphores.clear();
	finishedSemaphore.clear();
	inFlightFences.clear();
	imageInFlight.clear();

	vkDestroyCommandPool(m_instance.device, command_pool, nullptr);

	for (auto framebuffer : framebuffers)
	{
		vkDestroyFramebuffer(m_instance.device, framebuffer, nullptr);
	}

	vkDestroyPipeline(m_instance.device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(m_instance.device, pipelineLayout, nullptr);
	vkDestroyRenderPass(m_instance.device, renderPass, nullptr);

	m_swapChain.Close();
	m_instance.Shutdown();	
}

void RenderSystem::TestDraw()
{
	// NEW =>
	{
		PerFrame& frame = mPerFrame[0];

		// Wait for and reset render complete fence
		CHECKED_CALL(frame.renderCompleteFence->WaitAndReset());

		uint32_t imageIndex = UINT32_MAX;
		CHECKED_CALL(m_swapChain.AcquireNextImage(UINT64_MAX, frame.imageAcquiredSemaphore, frame.imageAcquiredFence, &imageIndex));

		// Wait for and reset image acquired fence
		CHECKED_CALL(frame.imageAcquiredFence->WaitAndReset());

		// Build command buffer
		CHECKED_CALL(frame.cmd->Begin());
		{
			RenderPassPtr renderPass = m_swapChain.GetRenderPass(imageIndex);
			ASSERT_MSG(!renderPass.IsNull(), "render pass object is null");

			RenderPassBeginInfo beginInfo = {};
			beginInfo.pRenderPass = renderPass;
			beginInfo.renderArea = renderPass->GetRenderArea();
			beginInfo.RTVClearCount = 1;
			beginInfo.RTVClearValues[0] = { {1, 0, 0, 1} };

			frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_PRESENT, RESOURCE_STATE_RENDER_TARGET);
			frame.cmd->BeginRenderPass(&beginInfo);
			{
				frame.cmd->SetScissors(GetScissor());
				frame.cmd->SetViewports(GetViewport());
				frame.cmd->BindGraphicsDescriptorSets(mPipelineInterface, 0, nullptr);
				frame.cmd->BindGraphicsPipeline(mPipeline);
				frame.cmd->BindVertexBuffers(1, &mVertexBuffer, &mVertexBinding.GetStride());
				frame.cmd->Draw(3, 1, 0, 0);

				// Draw ImGui
				DrawDebugInfo();
				DrawImGui(frame.cmd);
			}
			frame.cmd->EndRenderPass();
			frame.cmd->TransitionImageLayout(renderPass->GetRenderTargetImage(0), ALL_SUBRESOURCES, RESOURCE_STATE_RENDER_TARGET, RESOURCE_STATE_PRESENT);
		}
		CHECKED_CALL(frame.cmd->End());

		SubmitInfo submitInfo = {};
		submitInfo.commandBufferCount = 1;
		submitInfo.ppCommandBuffers = &frame.cmd;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.ppWaitSemaphores = &frame.imageAcquiredSemaphore;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.ppSignalSemaphores = &frame.renderCompleteSemaphore;
		submitInfo.pFence = frame.renderCompleteFence;

		CHECKED_CALL(GetGraphicsQueue()->Submit(&submitInfo));

		CHECKED_CALL(m_swapChain.Present(imageIndex, 1, &frame.renderCompleteSemaphore));

		return;
	}


	// OLD =>
	{
		inFlightFences[current_frame]->Wait();

		uint32_t image_index = UINT32_MAX;
		VkResult result = vkAcquireNextImageKHR(m_instance.device, m_swapChain.GetVkSwapChain(), UINT64_MAX, availableSemaphores[current_frame]->Get(), VK_NULL_HANDLE, &image_index);
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

		VkSemaphore wait_semaphores[] = { availableSemaphores[current_frame]->Get() };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		VkSemaphore signal_semaphores[] = { finishedSemaphore[current_frame]->Get() };

		VkSubmitInfo submitInfo = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO }; // TODO: переделать
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = wait_semaphores;
		submitInfo.pWaitDstStageMask = wait_stages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &command_buffers[image_index];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signal_semaphores;

		inFlightFences[current_frame]->Reset();

		if (vkQueueSubmit(m_instance.graphicsQueue->Queue, 1, &submitInfo, inFlightFences[current_frame]->Get()) != VK_SUCCESS)
		{
			Fatal("failed to submit draw command buffer");
			return; //"failed to submit draw command buffer
		}

		VkSwapchainKHR swapChains[] = { m_swapChain.GetVkSwapChain() };

		VkPresentInfoKHR present_info = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = signal_semaphores;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swapChains;
		present_info.pImageIndices = &image_index;

		result = vkQueuePresentKHR(m_instance.presentQueue->Queue, &present_info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			if (recreateSwapchain(m_swapChain, m_engine.GetWindow().GetWidth(), m_engine.GetWindow().GetHeight(), m_instance.physicalDevice, m_instance.device, m_instance.surface, m_instance.graphicsQueue) != 0)
				Fatal("recreateSwapchain failed");
			return;
		}
		else if (result != VK_SUCCESS)
		{
			Fatal("failed to present swapchain image");
			return;
		}

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
}

Rect RenderSystem::GetScissor() const
{
	Rect rect = {};
	rect.x = 0;
	rect.y = 0;
	rect.width = m_engine.GetWindow().GetWidth();
	rect.height = m_engine.GetWindow().GetHeight();
	return rect;
}

Viewport RenderSystem::GetViewport(float minDepth, float maxDepth) const
{
	Viewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(m_engine.GetWindow().GetWidth());
	viewport.height = static_cast<float>(m_engine.GetWindow().GetHeight());
	viewport.minDepth = minDepth;
	viewport.maxDepth = maxDepth;
	return viewport;
}

#pragma endregion