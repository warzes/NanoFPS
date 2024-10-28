#include "02_VulkanInstance.h"
#include "03_VulkanPhysicalDevice.h"

//=============================================================================
//
//=============================================================================

VulkanInstance::VulkanInstance()
{
	VkResult result = volkInitialize();
	CheckResult(result, "Failed to initialize volk.");

	std::vector<VkLayerProperties> availableLayers = QueryInstanceLayers();
	std::vector<VkExtensionProperties> availableInstanceExts = QueryInstanceExtensions();

	const std::vector<const char*> layers =
	{
#if defined(_DEBUG)
		"VK_LAYER_KHRONOS_validation"
#endif		
	};
	const std::vector<const char*> extensions =
	{
#if defined(_DEBUG)
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
		"VK_KHR_win32_surface",
#endif
	};

	if (!CheckInstanceLayerSupported(layers, availableLayers))
		Fatal("instance layers not supports");
	if (!CheckInstanceExtensionSupported(extensions, availableInstanceExts))
		Fatal("instance extensions not supports");

	std::vector<VkBaseOutStructure*> nextChain;

#if defined(_DEBUG)
	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCI.pfnUserCallback = DefaultDebugCallback;
	nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&debugUtilsMessengerCI));
#endif

	std::vector<VkValidationFeatureEnableEXT> enabledValidationFeatures =
	{
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,               // проверка шейдеров. но это очень медленно
		VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,             // лучшие практики использования вулкан
		VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,               // printf в шейдере?
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT, // проверка проблем синхронизации
	};

	VkValidationFeaturesEXT features = { .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	features.enabledValidationFeatureCount = static_cast<uint32_t>(enabledValidationFeatures.size());
	features.pEnabledValidationFeatures = enabledValidationFeatures.data();
	nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features));

	VkApplicationInfo appInfo = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = "Game";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

	VkInstanceCreateInfo instanceCreateInfo = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	instanceCreateInfo.ppEnabledLayerNames = layers.data();
	instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
	SetupPNextChain(instanceCreateInfo, nextChain);

	result = vkCreateInstance(&instanceCreateInfo, nullptr, &m_instance);
	CheckResult(result, "Failed to create instance:");

#if defined(_DEBUG)
	result = vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCI, nullptr, &m_debugMessenger);
	CheckResult(result, "Could not create debug utils messenger: ");
#endif
}

VulkanInstance::~VulkanInstance()
{
	if (m_debugMessenger) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
	if (m_instance) vkDestroyInstance(m_instance, nullptr);
	volkFinalize();
}

std::vector<PhysicalDevice> VulkanInstance::QueryPhysicalDevices()
{
	auto retDevices = QueryWithMethod<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_instance);
	CheckResult(retDevices.first, "vkEnumeratePhysicalDevices");

	std::vector<PhysicalDevice> devices;
	devices.reserve(retDevices.second.size());
	for (size_t idx = 0; idx < devices.size(); idx++)
	{
		const VkPhysicalDevice phyDevice = retDevices.second[idx];
		devices.push_back({ phyDevice });
	}
	return devices;
}