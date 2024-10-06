#include "stdafx.h"
#include "001.h"

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif
const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	[[maybe_unused]] VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	[[maybe_unused]] const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	[[maybe_unused]] void* pUserData) noexcept
{
	std::string text = "validation layer: " + std::string(pCallbackData->pMessage);
	Error(text);
	return VK_FALSE;
}

bool Test001::Init()
{
	if (volkInitialize() != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}

	if (!createInstance()) return false;

	volkLoadInstanceOnly(m_instance);

	if (!setupDebugMessenger()) return false;
	if (!createSurface()) return false;
	if (!pickPhysicalDevice()) return false;
	if (!createLogicalDevice()) return false;

	volkLoadDevice(m_device);

	if (!getDeviceQueue()) return false;
	if (!createCommandPool()) return false;

	return true;
}

void Test001::Close()
{
	vkDestroyCommandPool(m_device, m_commandPool, nullptr);
	m_commandPool = VK_NULL_HANDLE;

	vkDestroyDevice(m_device, nullptr);
	m_device = VK_NULL_HANDLE;

	if (enableValidationLayers)
		vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	m_debugMessenger = VK_NULL_HANDLE;

	vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
	m_surface = VK_NULL_HANDLE;

	vkDestroyInstance(m_instance, nullptr);
	m_instance = VK_NULL_HANDLE;

	volkFinalize();
}

void Test001::Update()
{
}

void Test001::Draw()
{
}

VkFormat Test001::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)  return format;
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) return format;
	}
	Fatal("failed to find supported format!");
	return {};
}

uint32_t Test001::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			return i;
	}

	Fatal("failed to find suitable memory type!");
	return 0;
}

void Test001::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}

	vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

bool Test001::checkValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (auto layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) return false;
	}

	return true;
}

std::vector<const char*> Test001::getRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

	return extensions;
}

void Test001::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
	createInfo.pUserData = nullptr; // Optional
}

bool Test001::hasGLFWRequiredInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	Print("Available extensions:");
	std::unordered_set<std::string> available;
	for (const auto& extension : extensions)
	{
		Print("\t" + std::string(extension.extensionName));
		available.insert(extension.extensionName);
	}

	Print("Required extensions:");
	auto requiredExtensions = getRequiredExtensions();
	for (const auto& required : requiredExtensions)
	{
		Print("\t" + std::string(required));
		if (available.find(required) == available.end())
		{
			Fatal("Missing required glfw extension");
			return false;
		}
	}

	return true;
}

bool Test001::isDeviceSuitable(VkPhysicalDevice device)
{
	QueueFamilyIndices indices = findQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

QueueFamilyIndices Test001::findQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indices.graphicsFamily = i;
			indices.graphicsFamilyHasValue = true;
		}
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
		if (queueFamily.queueCount > 0 && presentSupport)
		{
			indices.presentFamily = i;
			indices.presentFamilyHasValue = true;
		}
		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

bool Test001::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
		requiredExtensions.erase(extension.extensionName);

	return requiredExtensions.empty();
}

SwapChainSupportDetails Test001::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
	}
	return details;
}

bool Test001::createInstance()
{
	if (enableValidationLayers && !checkValidationLayerSupport())
	{
		Fatal("validation layers requested, but not available!");
		return false;
	}

	auto extensions = getRequiredExtensions();

	VkApplicationInfo appInfo  = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName        = "No Engine";
	appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion         = VK_API_VERSION_1_3;

	VkInstanceCreateInfo createInfo    = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo        = &appInfo;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}

	VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create instance! " + std::string(string_VkResult(result)));
		return false;
	}

	if (!hasGLFWRequiredInstanceExtensions())
		return false;

	return true;
}

bool Test001::setupDebugMessenger()
{
	if (enableValidationLayers)
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);
		VkResult result = vkCreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger);
		if (result != VK_SUCCESS)
		{
			Fatal("failed to set up debug messenger! " + std::string(string_VkResult(result)));
			return false;
		}
	}

	return true;
}

bool Test001::createSurface()
{
	VkResult result = glfwCreateWindowSurface(m_instance, GetWindow(), nullptr, &m_surface);
	if (result != VK_SUCCESS)
	{
		const char* errorMsg = nullptr;
		int ret = glfwGetError(&errorMsg);
		if (ret != 0)
		{
			std::string text = std::to_string(ret) + " ";
			if (errorMsg != nullptr) text += std::string(errorMsg);
			Fatal(text);
			return false;
		}
	}
	return true;
}

bool Test001::pickPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
	if (deviceCount == 0)
	{
		Fatal("failed to find GPUs with Vulkan support!");
		return false;
	}
	Print("Device count: " + std::to_string(deviceCount));
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (isDeviceSuitable(device))
		{
			m_physicalDevice = device;
			break;
		}
	}

	if (m_physicalDevice == VK_NULL_HANDLE)
	{
		Fatal("failed to find a suitable GPU!");
		return false;
	}

	vkGetPhysicalDeviceProperties(m_physicalDevice, &m_properties);
	Print("physical device: " + std::string(m_properties.deviceName));

	return true;
}

bool Test001::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
		queueCreateInfo.queueFamilyIndex        = queueFamily;
		queueCreateInfo.queueCount              = 1;
		queueCreateInfo.pQueuePriorities        = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo      = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos       = queueCreateInfos.data();
	createInfo.pEnabledFeatures        = &deviceFeatures;
	createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	// might not really be necessary anymore because device specific validation layers have been deprecated
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VkResult result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create logical device! " + std::string(string_VkResult(result)));
		return false;
	}

	m_graphicsQueueFamilyIndex = indices.graphicsFamily;
	m_presentQueueFamilyIndex = indices.presentFamily;

	return true;
}

bool Test001::getDeviceQueue()
{
	vkGetDeviceQueue(m_device, m_graphicsQueueFamilyIndex, 0, &m_graphicsQueue);
	vkGetDeviceQueue(m_device, m_presentQueueFamilyIndex, 0, &m_presentQueue);

	return true;
}

bool Test001::createCommandPool()
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);

	VkCommandPoolCreateInfo poolInfo = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily;
	poolInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create command pool! " + std::string(string_VkResult(result)));
		return false;
	}
	return true;
}