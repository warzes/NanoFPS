#include "VulkanWrapper.h"

#if defined(_MSC_VER)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#endif

#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

namespace vkw {

//=============================================================================
#pragma region [ Error ]

namespace
{
	bool IsValid = true;
	std::string ErrorText = "";
}


bool IsValidState()
{
	return IsValid;
}

const char* GetLastErrorText()
{
	return ErrorText.c_str();
}

inline void error(const std::string& msg)
{
	IsValid = false;
	ErrorText = msg;
#if defined(_DEBUG)
	puts(ErrorText.c_str());
#endif
}

inline bool resultCheck(VkResult result, const std::string& message)
{
	if (result != VK_SUCCESS)
	{
		error(message + " - " + string_VkResult(result));
		return false;
	}
	return true;
}


#pragma endregion

//=============================================================================
#pragma region [ Core Struct ]

#pragma endregion

//=============================================================================
#pragma region [ Core Func ]

VulkanAPIVersion Version(uint32_t vkVersion)
{
	if (VK_API_VERSION_MAJOR(vkVersion) == 1)
	{
		if (VK_API_VERSION_MINOR(vkVersion) == 1) return VulkanAPIVersion::Version11;
		else if (VK_API_VERSION_MINOR(vkVersion) == 2) return VulkanAPIVersion::Version12;
		else if (VK_API_VERSION_MINOR(vkVersion) == 3) return VulkanAPIVersion::Version13;
	}
	error("Unknown Instance version");
	return VulkanAPIVersion::Unknown;
}

uint32_t Version(VulkanAPIVersion version)
{
	if (version == VulkanAPIVersion::Version10) return VK_MAKE_VERSION(1, 0, 0);
	if (version == VulkanAPIVersion::Version11) return VK_MAKE_VERSION(1, 1, 0);
	if (version == VulkanAPIVersion::Version12) return VK_MAKE_VERSION(1, 2, 0);
	if (version == VulkanAPIVersion::Version13) return VK_MAKE_VERSION(1, 3, 0);
	error("Unknown Instance version");
	return 0;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkw::DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	auto ms = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
	auto mt = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
	printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE; // Applications must return false here
}

#pragma endregion


//=============================================================================
#pragma region [ Context ]

uint64_t Context::s_refCount = 0;

Context::Context()
{
	if (s_refCount == 0)
	{
		if (volkInitialize() != VK_SUCCESS)
		{
			error("Failed to initialize volk.");
			return;
		}
	}
	s_refCount++;
}

Context::~Context()
{
	s_refCount--;
	if (s_refCount == 0)
	{
		volkFinalize();
	}
}

VulkanAPIVersion Context::EnumerateInstanceVersion() const
{
	uint32_t apiVersion;
	VkResult result = vkEnumerateInstanceVersion(&apiVersion);
	resultCheck(result, "Context::EnumerateInstanceVersion()");
	return Version(apiVersion);
}

std::vector<VkLayerProperties> Context::EnumerateInstanceLayerProperties() const
{
	std::vector<VkLayerProperties> properties;
	uint32_t                       propertyCount;
	VkResult                       result;

	do
	{
		result = vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);
		if ((result == VK_SUCCESS) && propertyCount)
		{
			properties.resize(propertyCount);
			result = vkEnumerateInstanceLayerProperties(&propertyCount, properties.data());
		}
	} while (result == VK_INCOMPLETE);

	resultCheck(result, "Context::EnumerateInstanceLayerProperties()");
	VULKAN_WRAPPER_ASSERT(propertyCount <= properties.size());
	if (propertyCount < properties.size())
		properties.resize(propertyCount);

	return properties;
}

std::vector<VkExtensionProperties> Context::EnumerateInstanceExtensionProperties(const char* layerName) const
{
	std::vector<VkExtensionProperties> properties;
	uint32_t                           propertyCount;
	VkResult                           result;

	do
	{
		result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, nullptr);
		if ((result == VK_SUCCESS) && propertyCount)
		{
			properties.resize(propertyCount);
			result = vkEnumerateInstanceExtensionProperties(layerName, &propertyCount, properties.data());
		}
	} while (result == VK_INCOMPLETE);
	resultCheck(result, "Context::EnumerateInstanceExtensionProperties()");
	VULKAN_WRAPPER_ASSERT(propertyCount <= properties.size());
	if (propertyCount < properties.size())
		properties.resize(propertyCount);

	return properties;
}

bool Context::CheckAPIVersionSupported(VulkanAPIVersion version)
{
	const VulkanAPIVersion supportVersion = EnumerateInstanceVersion();
	if (static_cast<uint8_t>(supportVersion) >= static_cast<uint8_t>(version)) return true;
	
	error("API Version not support!");
	return false;
}

bool Context::CheckLayerSupported(const char* layerName)
{
	return CheckLayerSupported(EnumerateInstanceLayerProperties(), layerName);
}

bool Context::CheckLayerSupported(const std::vector<VkLayerProperties>& availableLayers, const char* layerName)
{
	if (!layerName) return false;
	for (const auto& layerProperties : availableLayers)
		if (strcmp(layerName, layerProperties.layerName) == 0) return true;
	error("Layer: " + std::string(layerName) + " not support");
	return false;
}

bool Context::CheckLayerSupported(const std::vector<const char*>& layerNames)
{
	const auto availableLayers = EnumerateInstanceLayerProperties();

	bool allFound = true;
	for (const auto& layerName : layerNames)
	{
		bool found = CheckLayerSupported(availableLayers, layerName);
		if (!found) allFound = false;
	}
	return allFound;
}

bool Context::CheckExtensionSupported(const char* extensionName)
{
	return CheckExtensionSupported(EnumerateInstanceExtensionProperties(), extensionName);
}

bool Context::CheckExtensionSupported(const std::vector<VkExtensionProperties>& availableExtensions, const char* extensionName)
{
	if (!extensionName) return false;
	for (const auto& extensionProperties : availableExtensions)
		if (strcmp(extensionName, extensionProperties.extensionName)) return true;
	error("Extension: " + std::string(extensionName) + " not support");
	return false;
}

bool Context::CheckExtensionSupported(const std::vector<const char*>& extensionNames)
{
	const auto availableExtensions = EnumerateInstanceExtensionProperties();

	bool allFound = true;
	for (const auto& extensionName : extensionNames)
	{
		bool found = CheckExtensionSupported(availableExtensions, extensionName);
		if (!found) allFound = false;
	}
	return allFound;
}

Instance Context::CreateInstance(const InstanceCreateInfo& createInfo)
{
	return Instance(createInfo);
}

#pragma endregion

//=============================================================================
#pragma region [ Instance ]

InstanceCreateInfo::InstanceCreateInfo(const VkInstanceCreateInfo& createInfo)
{
	applicationName    = createInfo.pApplicationInfo->pApplicationName;
	applicationVersion = createInfo.pApplicationInfo->applicationVersion;
	engineName         = createInfo.pApplicationInfo->pEngineName;
	engineVersion      = createInfo.pApplicationInfo->engineVersion;
	apiVersion         = Version(createInfo.pApplicationInfo->apiVersion);

	layers.resize(createInfo.enabledLayerCount);
	for (uint32_t i = 0; i < createInfo.enabledLayerCount; i++)
		layers[i] = createInfo.ppEnabledLayerNames[i];

	extensions.resize(createInfo.enabledExtensionCount);
	for (uint32_t i = 0; i < createInfo.enabledExtensionCount; i++)
		extensions[i] = createInfo.ppEnabledExtensionNames[i];

	flags = createInfo.flags;
}

Instance::Instance(const InstanceCreateInfo& createInfo)
{
	VULKAN_WRAPPER_ASSERT(!m_instance);

	if (!IsValidState()) return;
	if (createInfo.apiVersion == VulkanAPIVersion::Version10)
	{
		error("Vulkan 1.0 not support!");
		return;
	}

	if (!m_context.CheckAPIVersionSupported(createInfo.apiVersion)) return;	
	if (!m_context.CheckLayerSupported(createInfo.layers)) return;
	if (!m_context.CheckExtensionSupported(createInfo.extensions)) return;

	m_allocator = createInfo.allocator;

	const auto availableLayers     = m_context.EnumerateInstanceLayerProperties();
	const auto availableExtensions = m_context.EnumerateInstanceExtensionProperties();

	std::vector<const char*> layers = createInfo.layers;
	std::vector<const char*> extensions = createInfo.extensions;
	bool portabilityEnumerationSupport = false;

	bool useDebugMessenger = createInfo.debugCallback != nullptr;

	// вставка дополнительных расширений если они нужны, но не были заданы
	{
		// TODO: сделать проверку на уникальность при вставке?
		auto checkAndInsertExtension = [&](const char* name) -> bool {
			if (!m_context.CheckExtensionSupported(availableExtensions, name)) return false;
			extensions.push_back(name);
			return true;
			};

		if (createInfo.enableValidationLayers)
		{
			if (m_context.CheckLayerSupported(availableLayers, "VK_LAYER_KHRONOS_validation"))
			{
				if (std::none_of(layers.begin(), layers.end(), [](const std::string& layer) { return layer == "VK_LAYER_KHRONOS_validation"; }))
					layers.push_back("VK_LAYER_KHRONOS_validation");
			}
		}

		if (useDebugMessenger)
			useDebugMessenger = checkAndInsertExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#if defined(VK_KHR_portability_enumeration)
		portabilityEnumerationSupport = checkAndInsertExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

		bool khrSurfaceAdded = checkAndInsertExtension("VK_KHR_surface");
#if defined(_WIN32)
		bool addedWindowExts = checkAndInsertExtension("VK_KHR_win32_surface");
#elif defined(__ANDROID__)
		bool addedWindowExts = checkAndInsertExtension("VK_KHR_android_surface");
#elif defined(_DIRECT2DISPLAY)
		bool addedWindowExts = checkAndInsertExtension("VK_KHR_display");
#elif defined(__linux__)
		// make sure all three calls to check_add_window_ext, don't allow short circuiting
		bool addedWindowExts = checkAndInsertExtension("VK_KHR_xcb_surface");
		addedWindowExts = checkAndInsertExtension("VK_KHR_xlib_surface") || addedWindowExts;
		addedWindowExts = checkAndInsertExtension("VK_KHR_wayland_surface") || addedWindowExts;
#elif defined(__APPLE__)
		bool addedWindowExts = checkAndInsertExtension("VK_EXT_metal_surface");
#endif
		if (!khrSurfaceAdded || !addedWindowExts)
		{
			error("windowing extensions not present");
			return;
		}
	}

	VkApplicationInfo appInfo  = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = createInfo.applicationName.c_str();
	appInfo.applicationVersion = createInfo.applicationVersion;
	appInfo.pEngineName        = createInfo.engineName.c_str();
	appInfo.engineVersion      = createInfo.engineVersion;
	appInfo.apiVersion         = Version(createInfo.apiVersion);

	std::vector<VkBaseOutStructure*> nextChain;

	if (useDebugMessenger)
	{
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		messengerCreateInfo.messageSeverity = createInfo.debugMessageSeverity;
		messengerCreateInfo.messageType     = createInfo.debugMessageType;
		messengerCreateInfo.pfnUserCallback = createInfo.debugCallback;
		messengerCreateInfo.pUserData       = createInfo.debugUserDataPointer;
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&messengerCreateInfo));
	}

	if (createInfo.enabledValidationFeatures.size() != 0 || createInfo.disabledValidationFeatures.size())
	{
		VkValidationFeaturesEXT features{ .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
		features.enabledValidationFeatureCount  = static_cast<uint32_t>(createInfo.enabledValidationFeatures.size());
		features.pEnabledValidationFeatures     = createInfo.enabledValidationFeatures.data();
		features.disabledValidationFeatureCount = static_cast<uint32_t>(createInfo.disabledValidationFeatures.size());
		features.pDisabledValidationFeatures    = createInfo.disabledValidationFeatures.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features));
	}

	if (createInfo.disabledValidationChecks.size() != 0)
	{
		VkValidationFlagsEXT checks{ .sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT };
		checks.disabledValidationCheckCount = static_cast<uint32_t>(createInfo.disabledValidationChecks.size());
		checks.pDisabledValidationChecks = createInfo.disabledValidationChecks.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&checks));
	}

	VkInstanceCreateInfo instanceCreateInfo    = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	SetupPNextChain(instanceCreateInfo, nextChain);
	instanceCreateInfo.pApplicationInfo        = &appInfo;
	instanceCreateInfo.flags                   = createInfo.flags;
	instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(layers.size());
	instanceCreateInfo.ppEnabledLayerNames     = layers.data();
#if defined(VK_KHR_portability_enumeration)
	if (portabilityEnumerationSupport)
	{
		instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}
#endif

	VkResult result = vkCreateInstance(&instanceCreateInfo, createInfo.allocator, &m_instance);
	if (!resultCheck(result, "Instance create")) return;

	volkLoadInstance(m_instance);

	if (useDebugMessenger) 
	{
		VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		messengerCreateInfo.messageSeverity = createInfo.debugMessageSeverity;
		messengerCreateInfo.messageType     = createInfo.debugMessageType;
		messengerCreateInfo.pfnUserCallback = createInfo.debugCallback;
		messengerCreateInfo.pUserData       = createInfo.debugUserDataPointer;

		result = vkCreateDebugUtilsMessengerEXT(m_instance, &messengerCreateInfo, m_allocator, &m_debugMessenger);
		if (!resultCheck(result, "failed create debug messenger")) return;
	}

	m_isValid = true;
}

Instance::~Instance()
{
	if (m_debugMessenger) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_allocator);
	vkDestroyInstance(m_instance, nullptr);
}

bool Instance::IsValid() const
{
	return m_isValid && m_instance != nullptr;
}

std::vector<PhysicalDevice> vkw::Instance::GetPhysicalDevices()
{
	std::vector<VkPhysicalDevice> physicalDevices;

	auto physicalDevicesRet = GetVector<VkPhysicalDevice>(physicalDevices, vkEnumeratePhysicalDevices, m_instance);
	if (physicalDevicesRet != VK_SUCCESS)
	{
		error("failed enumerate physical devices");
		return {};
	}
	if (physicalDevices.size() == 0)
	{
		error("no physical devices found");
		return {};
	}



	return physicalDevices;
}

#pragma endregion

} // namespace vkw