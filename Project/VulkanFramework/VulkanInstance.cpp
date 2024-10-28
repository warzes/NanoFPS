#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"

#ifdef _WIN32
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace vkf {

Instance::~Instance()
{
	Shutdown();
}

bool Instance::Setup(const InstanceCreateInfo& createInfo)
{
	assert(!m_initVolk);
	assert(!m_instance);
	assert(!m_debugMessenger);

	if (volkInitialize() != VK_SUCCESS)
	{
		Fatal("Failed to initialize volk.");
		return false;
	}
	m_initVolk = true;

	bool enableValidationLayers = createInfo.enableValidationLayers;
	bool hasDebugUtils = createInfo.useDebugMessenger && createInfo.debugCallback != nullptr;
	bool portabilityEnumerationSupport = false;

	std::vector<const char*> layers = createInfo.layers;
	std::vector<const char*> extensions = createInfo.extensions;

	// вставка дополнительных расширений если они нужны, но не были заданы
	{
		auto availableLayersRet = QueryInstanceLayers();
		if (!availableLayersRet) return false;
		std::vector<VkLayerProperties> availableLayers = availableLayersRet.value();
		if (!CheckInstanceLayerSupported(createInfo.layers, availableLayers)) return false;

		auto availableInstanceExtsRet = QueryInstanceExtensions();
		if (!availableInstanceExtsRet) return false;
		std::vector<VkExtensionProperties> availableInstanceExts = availableInstanceExtsRet.value();
		if (!CheckInstanceExtensionSupported(createInfo.extensions, availableInstanceExts)) return false;

		auto checkAndInsertLayer = [&](const char* name) -> bool {

			if (std::none_of(layers.begin(), layers.end(), [name](const char* extName) { return strcmp(extName, name) == 0; }))
			{
				if (!CheckInstanceLayerSupported(name, availableLayers)) return false;
				layers.push_back(name);
			}
			return true;
			};

		auto checkAndInsertExtension = [&](const char* name) -> bool {

			if (std::none_of(extensions.begin(), extensions.end(), [name](const char* extName) { return strcmp(extName, name) == 0; }))
			{
				if (!CheckInstanceExtensionSupported(name, availableInstanceExts)) return false;
				extensions.push_back(name);
			}
			return true;
			};

		if (enableValidationLayers) enableValidationLayers = checkAndInsertLayer("VK_LAYER_KHRONOS_validation");

		if (hasDebugUtils) hasDebugUtils = checkAndInsertExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

#if defined(VK_KHR_portability_enumeration)
		portabilityEnumerationSupport = checkAndInsertExtension(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif
		bool khrSurfaceAdded = checkAndInsertExtension(VK_KHR_SURFACE_EXTENSION_NAME);
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
			Fatal("windowing extensions not present");
			return false;
		}
	}

	std::vector<VkBaseOutStructure*> nextChain = createInfo.nextElements;

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugUtilsMessengerCI.messageSeverity = createInfo.debugMessageSeverity;
	debugUtilsMessengerCI.messageType     = createInfo.debugMessageType;
	debugUtilsMessengerCI.pfnUserCallback = createInfo.debugCallback;
	debugUtilsMessengerCI.pUserData       = createInfo.debugUserDataPointer;

	if (hasDebugUtils) nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&debugUtilsMessengerCI));

	if (createInfo.enabledValidationFeatures.size() != 0 || createInfo.disabledValidationFeatures.size())
	{
		VkValidationFeaturesEXT features = { .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
		features.enabledValidationFeatureCount = static_cast<uint32_t>(createInfo.enabledValidationFeatures.size());
		features.pEnabledValidationFeatures = createInfo.enabledValidationFeatures.data();
		features.disabledValidationFeatureCount = static_cast<uint32_t>(createInfo.disabledValidationFeatures.size());
		features.pDisabledValidationFeatures = createInfo.disabledValidationFeatures.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features));
	}

	if (createInfo.disabledValidationChecks.size() != 0)
	{
		VkValidationFlagsEXT checks = { .sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT };
		checks.disabledValidationCheckCount = static_cast<uint32_t>(createInfo.disabledValidationChecks.size());
		checks.pDisabledValidationChecks = createInfo.disabledValidationChecks.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&checks));
	}

	// If layer settings extension enabled by sample, then activate layer settings during instance creation
	if (std::find(extensions.begin(), extensions.end(), VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) != extensions.end())
	{
		VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = { .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT };
		layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(createInfo.requiredLayerSettings.size());
		layerSettingsCreateInfo.pSettings = createInfo.requiredLayerSettings.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&layerSettingsCreateInfo));
	}

	VkApplicationInfo appInfo  = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName   = createInfo.applicationName.data();
	appInfo.applicationVersion = createInfo.applicationVersion;
	appInfo.pEngineName        = createInfo.engineName.data();
	appInfo.engineVersion      = createInfo.engineVersion;
	appInfo.apiVersion         = VK_MAKE_VERSION(1, 3, 0);

	VkInstanceCreateInfo instanceCreateInfo    = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	SetupPNextChain(instanceCreateInfo, nextChain);
	instanceCreateInfo.pApplicationInfo        = &appInfo;
	instanceCreateInfo.flags                   = createInfo.flags;
	instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(layers.size());
	instanceCreateInfo.ppEnabledLayerNames     = layers.data();
#if defined(VK_KHR_portability_enumeration)
	if (portabilityEnumerationSupport) instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
	if (result != VK_SUCCESS)
	{
		Fatal("Could not create Vulkan instance - " + std::string(string_VkResult(result)));
		return false;
	}

	volkLoadInstance(m_instance);

	if (hasDebugUtils)
	{
		debugUtilsMessengerCI.pNext = nullptr;
		result = vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCI, nullptr, &m_debugMessenger);
		if (result != VK_SUCCESS)
		{
			Fatal("Could not create debug utils messenger - " + std::string(string_VkResult(result)));
			return false;
		}
	}

	return true;
}

void Instance::Shutdown()
{
	if (m_debugMessenger) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	if (m_instance)       vkDestroyInstance(m_instance, nullptr);
	if (m_initVolk)       volkFinalize();

	m_debugMessenger = nullptr;
	m_instance = nullptr;
	m_initVolk = false;
}

std::vector<PhysicalDevice> Instance::GetPhysicalDevices()
{
	auto retDevices = QueryWithMethod<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_instance);
	if (retDevices.first != VK_SUCCESS)
	{
		Fatal("vkEnumeratePhysicalDevices - " + std::string(string_VkResult(retDevices.first)));
		return {};
	}
	std::vector<PhysicalDevice> devices;
	devices.reserve(retDevices.second.size());
	for (size_t idx = 0; idx < devices.size(); idx++)
	{
		const VkPhysicalDevice phyDevice = retDevices.second[idx];
		devices.push_back({ phyDevice });
	}
	return devices;
}

} // namespace vkf