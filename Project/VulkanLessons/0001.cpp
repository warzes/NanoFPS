/*
0001 
- Init Volk
- Create Vulkan Instance
- Check Available Layers and Available Instance Extensions
- List Physical Devices
*/

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#endif

#include <string>
#include <vector>

#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#ifdef _WIN32
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
	{
		auto ms = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
		auto mt = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
		printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

		return VK_FALSE;
	}

	inline void CheckResult(VkResult result, const std::string& message)
	{
		if (result != VK_SUCCESS)
			throw std::exception((message + " - " + string_VkResult(result)).c_str());
	}

	template<typename PROPS, typename FUNC, typename... ARGS>
	std::pair<VkResult, std::vector<PROPS>> QueryWithMethod(FUNC&& queryWith, ARGS&&... args)
	{
		uint32_t count{ 0 };
		VkResult result{ VK_SUCCESS };
		if (VK_SUCCESS != (result = queryWith(args..., &count, nullptr))) return { result, {} };
		std::vector<PROPS> resultVec(count);
		if (VK_SUCCESS != (result = queryWith(args..., &count, resultVec.data()))) return { result, {} };
		return { VK_SUCCESS, resultVec };
	}

	template<typename PROPS, typename FUNC, typename... ARGS>
	std::vector<PROPS> QueryWithMethodNoError(FUNC&& queryWith, ARGS&&... args)
	{
		uint32_t count{ 0 };
		std::vector<PROPS> resultVec;
		queryWith(args..., &count, nullptr);
		resultVec.resize(count);
		queryWith(args..., &count, resultVec.data());
		return resultVec;
	}

	template <typename T>
	void SetupPNextChain(T& structure, const std::vector<VkBaseOutStructure*>& structs)
	{
		structure.pNext = nullptr;
		if (structs.empty()) return;
		for (size_t i = 0; i < structs.size() - 1; i++)
			structs.at(i)->pNext = structs.at(i + 1);
		structure.pNext = structs.at(0);
	}

	std::vector<VkLayerProperties> QueryInstanceLayers()
	{
		auto result = QueryWithMethod<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
		CheckResult(result.first, "vkEnumerateInstanceLayerProperties");
		return result.second;
	}

	std::vector<VkExtensionProperties> QueryInstanceExtensions()
	{
		auto result = QueryWithMethod<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
		CheckResult(result.first, "vkEnumerateInstanceExtensionProperties");
		return result.second;
	}

	struct Instance
	{
		VkInstance instance{ VK_NULL_HANDLE };
		VkDebugUtilsMessengerEXT debugMessenger{ VK_NULL_HANDLE };
	};

	Instance CreateInstance()
	{
		Instance instance;

		std::vector<const char*> layers = 
		{
#if defined(_DEBUG)
			"VK_LAYER_KHRONOS_validation"
#endif		
		};
		std::vector<const char*> extensions = 
		{
#if defined(_DEBUG)
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
			VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(_WIN32)
			"VK_KHR_win32_surface",
#endif
		};

#if defined(_DEBUG)
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugUtilsMessengerCI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
												VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugUtilsMessengerCI.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
												VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
												VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugUtilsMessengerCI.pfnUserCallback = DefaultDebugCallback;
#endif

		VkApplicationInfo appInfo  = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO };
		appInfo.pApplicationName   = "Game";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName        = "Engine";
		appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion         = VK_MAKE_VERSION(1, 3, 0);

		VkInstanceCreateInfo instanceCreateInfo    = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
		instanceCreateInfo.pApplicationInfo        = &appInfo;
		instanceCreateInfo.enabledLayerCount       = static_cast<uint32_t>(layers.size());
		instanceCreateInfo.ppEnabledLayerNames     = layers.data();
		instanceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
#if defined(_DEBUG)
		instanceCreateInfo.pNext = reinterpret_cast<VkBaseOutStructure*>(&debugUtilsMessengerCI);
#endif

		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance.instance);
		CheckResult(result, "Failed to create instance:");

		volkLoadInstance(instance.instance);

#if defined(_DEBUG)
			result = vkCreateDebugUtilsMessengerEXT(instance.instance, &debugUtilsMessengerCI, nullptr, &instance.debugMessenger);
			CheckResult(result, "Could not create debug utils messenger: ");
#endif
		return instance;
	}

	struct PhysicalDeviceInfo
	{
		VkPhysicalDevice                   phyDevice;
		VkPhysicalDeviceProperties         properties;
		std::vector<VkExtensionProperties> extensions;
		VkPhysicalDeviceMemoryProperties   memory;
	};

	std::vector<PhysicalDeviceInfo> QueryPhysicalDevices(VkInstance instance)
	{
		std::vector<VkPhysicalDevice> devices = QueryWithMethod<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance).second;

		std::vector<PhysicalDeviceInfo> infos(devices.size());
		for (size_t idx = 0; idx < infos.size(); idx++)
		{
			const VkPhysicalDevice phyDevice = devices[idx];

			infos[idx].phyDevice = phyDevice;
			infos[idx].extensions = QueryWithMethod<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, phyDevice, nullptr).second;

			vkGetPhysicalDeviceProperties(phyDevice, &infos[idx].properties);
			vkGetPhysicalDeviceMemoryProperties(phyDevice, &infos[idx].memory);
		}

		return infos;
	}
}

void StartLesson0001()
{
	try
	{
		VkResult result = volkInitialize();
		CheckResult(result, "Failed to initialize volk.");

		std::vector<VkLayerProperties> availableLayers = QueryInstanceLayers();
		puts("Instance Layers:");
		for (size_t i = 0; i < availableLayers.size(); i++)
		{
			puts(("\t" + std::string(availableLayers[i].layerName)).c_str());
			puts(("\t\t" + std::string(availableLayers[i].description)).c_str());
		}

		std::vector<VkExtensionProperties> availableInstanceExts = QueryInstanceExtensions();
		puts("Instance Extensions:");
		for (size_t i = 0; i < availableInstanceExts.size(); i++)
			puts(("\t" + std::string(availableInstanceExts[i].extensionName)).c_str());

		Instance instance = CreateInstance();
		std::vector<PhysicalDeviceInfo> phyDevices = QueryPhysicalDevices(instance.instance);

		printf("Physical Devices (count = %ld)\n", phyDevices.size());
		for (size_t idx = 0; idx < phyDevices.size(); idx++)
		{
			const PhysicalDeviceInfo& info = phyDevices[idx];
			const VkPhysicalDeviceProperties& props = info.properties;

			printf("  %ld: deviceName = %s vendorID = 0x%x deviceID = 0x%x\n", idx, props.deviceName, props.vendorID, props.deviceID);
			printf("     deviceType = %s\n", string_VkPhysicalDeviceType(props.deviceType));
		}

		vkDestroyDebugUtilsMessengerEXT(instance.instance, instance.debugMessenger, nullptr);
		vkDestroyInstance(instance.instance, nullptr);
		volkFinalize();
	}
	catch (const std::exception& msg)
	{
		puts(msg.what());
	}
	catch (...)
	{
		puts("Unknown error");
	}
}