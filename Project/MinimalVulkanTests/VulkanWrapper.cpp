#include "VulkanWrapper.h"

#if defined(_MSC_VER)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 5039)
#endif

#include <algorithm>
#include <vulkan/vk_enum_string_helper.h>

#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

//#ifdef _WIN32
//extern "C"
//{
//	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
//	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}
//#endif

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

GenericFeaturesPNextNode::GenericFeaturesPNextNode() { memset(fields, UINT8_MAX, sizeof(VkBool32) * fieldCapacity); }

bool GenericFeaturesPNextNode::Match(const GenericFeaturesPNextNode& requested, const GenericFeaturesPNextNode& supported) noexcept
{
	assert(requested.sType == supported.sType && "Non-matching sTypes in features nodes!");
	for (uint32_t i = 0; i < fieldCapacity; i++)
	{
		if (requested.fields[i] && !supported.fields[i]) return false;
	}
	return true;
}

void GenericFeaturesPNextNode::Combine(const GenericFeaturesPNextNode& right) noexcept
{
	assert(sType == right.sType && "Non-matching sTypes in features nodes!");
	for (uint32_t i = 0; i < GenericFeaturesPNextNode::fieldCapacity; i++)
	{
		fields[i] = fields[i] || right.fields[i];
	}
}

bool GenericFeatureChain::MatchAll(const GenericFeatureChain& extensionRequested) const noexcept
{
	// Should only be false if extension_supported was unable to be filled out, due to the physical device not supporting vkGetPhysicalDeviceFeatures2 in any capacity.
	if (extensionRequested.nodes.size() != nodes.size()) return false;

	for (size_t i = 0; i < nodes.size() && i < nodes.size(); ++i)
	{
		if (!GenericFeaturesPNextNode::Match(extensionRequested.nodes[i], nodes[i])) return false;
	}
	return true;
}

bool GenericFeatureChain::FindAndMatch(const GenericFeatureChain& extensionsRequested) const noexcept
{
	for (const auto& requested_extension_node : extensionsRequested.nodes)
	{
		bool found = false;
		for (const auto& supported_node : nodes)
		{
			if (supported_node.sType == requested_extension_node.sType)
			{
				found = true;
				if (!GenericFeaturesPNextNode::Match(requested_extension_node, supported_node)) return false;
				break;
			}
		}
		if (!found) return false;
	}
	return true;
}

void GenericFeatureChain::ChainUp(VkPhysicalDeviceFeatures2& feats2) noexcept
{
	GenericFeaturesPNextNode* prev = nullptr;
	for (auto& extension : nodes)
	{
		if (prev != nullptr)
		{
			prev->pNext = &extension;
		}
		prev = &extension;
	}
	feats2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	feats2.pNext = !nodes.empty() ? &nodes.at(0) : nullptr;
}

void GenericFeatureChain::Combine(const GenericFeatureChain& right) noexcept
{
	for (const auto& right_node : right.nodes)
	{
		bool already_contained = false;
		for (auto& left_node : nodes)
		{
			if (left_node.sType == right_node.sType)
			{
				left_node.Combine(right_node);
				already_contained = true;
			}
		}
		if (!already_contained)
		{
			nodes.push_back(right_node);
		}
	}
}

#pragma endregion

//=============================================================================
#pragma region [ Vulkan Core Func ]

VulkanAPIVersion ConvertVersion(uint32_t vkVersion)
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

uint32_t ConvertVersion(VulkanAPIVersion version)
{
	if (version == VulkanAPIVersion::Version10) return VK_MAKE_VERSION(1, 0, 0);
	if (version == VulkanAPIVersion::Version11) return VK_MAKE_VERSION(1, 1, 0);
	if (version == VulkanAPIVersion::Version12) return VK_MAKE_VERSION(1, 2, 0);
	if (version == VulkanAPIVersion::Version13) return VK_MAKE_VERSION(1, 3, 0);
	error("Unknown Instance version");
	return 0;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	auto ms = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
	auto mt = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
	printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE; // Applications must return false here
}

void combineFeatures(VkPhysicalDeviceFeatures& dest, VkPhysicalDeviceFeatures src)
{
	dest.robustBufferAccess = dest.robustBufferAccess || src.robustBufferAccess;
	dest.fullDrawIndexUint32 = dest.fullDrawIndexUint32 || src.fullDrawIndexUint32;
	dest.imageCubeArray = dest.imageCubeArray || src.imageCubeArray;
	dest.independentBlend = dest.independentBlend || src.independentBlend;
	dest.geometryShader = dest.geometryShader || src.geometryShader;
	dest.tessellationShader = dest.tessellationShader || src.tessellationShader;
	dest.sampleRateShading = dest.sampleRateShading || src.sampleRateShading;
	dest.dualSrcBlend = dest.dualSrcBlend || src.dualSrcBlend;
	dest.logicOp = dest.logicOp || src.logicOp;
	dest.multiDrawIndirect = dest.multiDrawIndirect || src.multiDrawIndirect;
	dest.drawIndirectFirstInstance = dest.drawIndirectFirstInstance || src.drawIndirectFirstInstance;
	dest.depthClamp = dest.depthClamp || src.depthClamp;
	dest.depthBiasClamp = dest.depthBiasClamp || src.depthBiasClamp;
	dest.fillModeNonSolid = dest.fillModeNonSolid || src.fillModeNonSolid;
	dest.depthBounds = dest.depthBounds || src.depthBounds;
	dest.wideLines = dest.wideLines || src.wideLines;
	dest.largePoints = dest.largePoints || src.largePoints;
	dest.alphaToOne = dest.alphaToOne || src.alphaToOne;
	dest.multiViewport = dest.multiViewport || src.multiViewport;
	dest.samplerAnisotropy = dest.samplerAnisotropy || src.samplerAnisotropy;
	dest.textureCompressionETC2 = dest.textureCompressionETC2 || src.textureCompressionETC2;
	dest.textureCompressionASTC_LDR = dest.textureCompressionASTC_LDR || src.textureCompressionASTC_LDR;
	dest.textureCompressionBC = dest.textureCompressionBC || src.textureCompressionBC;
	dest.occlusionQueryPrecise = dest.occlusionQueryPrecise || src.occlusionQueryPrecise;
	dest.pipelineStatisticsQuery = dest.pipelineStatisticsQuery || src.pipelineStatisticsQuery;
	dest.vertexPipelineStoresAndAtomics = dest.vertexPipelineStoresAndAtomics || src.vertexPipelineStoresAndAtomics;
	dest.fragmentStoresAndAtomics = dest.fragmentStoresAndAtomics || src.fragmentStoresAndAtomics;
	dest.shaderTessellationAndGeometryPointSize = dest.shaderTessellationAndGeometryPointSize || src.shaderTessellationAndGeometryPointSize;
	dest.shaderImageGatherExtended = dest.shaderImageGatherExtended || src.shaderImageGatherExtended;
	dest.shaderStorageImageExtendedFormats = dest.shaderStorageImageExtendedFormats || src.shaderStorageImageExtendedFormats;
	dest.shaderStorageImageMultisample = dest.shaderStorageImageMultisample || src.shaderStorageImageMultisample;
	dest.shaderStorageImageReadWithoutFormat = dest.shaderStorageImageReadWithoutFormat || src.shaderStorageImageReadWithoutFormat;
	dest.shaderStorageImageWriteWithoutFormat = dest.shaderStorageImageWriteWithoutFormat || src.shaderStorageImageWriteWithoutFormat;
	dest.shaderUniformBufferArrayDynamicIndexing = dest.shaderUniformBufferArrayDynamicIndexing || src.shaderUniformBufferArrayDynamicIndexing;
	dest.shaderSampledImageArrayDynamicIndexing = dest.shaderSampledImageArrayDynamicIndexing || src.shaderSampledImageArrayDynamicIndexing;
	dest.shaderStorageBufferArrayDynamicIndexing = dest.shaderStorageBufferArrayDynamicIndexing || src.shaderStorageBufferArrayDynamicIndexing;
	dest.shaderStorageImageArrayDynamicIndexing = dest.shaderStorageImageArrayDynamicIndexing || src.shaderStorageImageArrayDynamicIndexing;
	dest.shaderClipDistance = dest.shaderClipDistance || src.shaderClipDistance;
	dest.shaderCullDistance = dest.shaderCullDistance || src.shaderCullDistance;
	dest.shaderFloat64 = dest.shaderFloat64 || src.shaderFloat64;
	dest.shaderInt64 = dest.shaderInt64 || src.shaderInt64;
	dest.shaderInt16 = dest.shaderInt16 || src.shaderInt16;
	dest.shaderResourceResidency = dest.shaderResourceResidency || src.shaderResourceResidency;
	dest.shaderResourceMinLod = dest.shaderResourceMinLod || src.shaderResourceMinLod;
	dest.sparseBinding = dest.sparseBinding || src.sparseBinding;
	dest.sparseResidencyBuffer = dest.sparseResidencyBuffer || src.sparseResidencyBuffer;
	dest.sparseResidencyImage2D = dest.sparseResidencyImage2D || src.sparseResidencyImage2D;
	dest.sparseResidencyImage3D = dest.sparseResidencyImage3D || src.sparseResidencyImage3D;
	dest.sparseResidency2Samples = dest.sparseResidency2Samples || src.sparseResidency2Samples;
	dest.sparseResidency4Samples = dest.sparseResidency4Samples || src.sparseResidency4Samples;
	dest.sparseResidency8Samples = dest.sparseResidency8Samples || src.sparseResidency8Samples;
	dest.sparseResidency16Samples = dest.sparseResidency16Samples || src.sparseResidency16Samples;
	dest.sparseResidencyAliased = dest.sparseResidencyAliased || src.sparseResidencyAliased;
	dest.variableMultisampleRate = dest.variableMultisampleRate || src.variableMultisampleRate;
	dest.inheritedQueries = dest.inheritedQueries || src.inheritedQueries;
}

#pragma endregion

//=============================================================================
#pragma region [ Context ]

uint64_t Context::s_refCount = 0;

Context::Context()
{
	if (s_refCount == 0)
	{
		// TODO: вообще можно сделать версию без volk (через дефайны) передав эту задачу пользователю и предоставив ему обертку над vkGetInstanceProcAddr
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
	return ConvertVersion(apiVersion);
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
	error("Instance Extension: " + std::string(extensionName) + " not support");
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

InstancePtr Context::CreateInstance(const InstanceCreateInfo& createInfo)
{
	InstancePtr ptr = std::make_shared<Instance>(createInfo);
	if (ptr && ptr->IsValid()) return ptr;

	return nullptr;
}

#pragma endregion

//=============================================================================
#pragma region [ Instance ]

Instance::Instance(const InstanceCreateInfo& createInfo)
{
	if (!checkValid(createInfo)) return;

	m_allocator = createInfo.allocator;

	bool enableValidationLayers = createInfo.enableValidationLayers;
	bool hasDebugUtils = createInfo.useDebugMessenger && createInfo.debugCallback != nullptr;
	bool portabilityEnumerationSupport = false;

	std::vector<const char*> layers = createInfo.layers;
	std::vector<const char*> extensions = createInfo.extensions;
	// вставка дополнительных расширений если они нужны, но не были заданы
	{
		const auto availableLayers = m_context.EnumerateInstanceLayerProperties();
		const auto availableExtensions = m_context.EnumerateInstanceExtensionProperties();

		auto checkAndInsertLayer = [&](const char* name) -> bool {

			if (std::none_of(layers.begin(), layers.end(), [name](const char* extName) { return strcmp(extName, name) == 0; }))
			{
				if (!m_context.CheckLayerSupported(availableLayers, name)) return false;
				layers.push_back(name);
			}
			return true;
			};

		auto checkAndInsertExtension = [&](const char* name) -> bool {

			if (std::none_of(extensions.begin(), extensions.end(), [name](const char* extName) { return strcmp(extName, name) == 0; }))
			{
				if (!m_context.CheckExtensionSupported(availableExtensions, name)) return false;
				extensions.push_back(name);
			}
			return true;
			};

		if (enableValidationLayers) enableValidationLayers = checkAndInsertLayer("VK_LAYER_KHRONOS_validation");

		if (hasDebugUtils) hasDebugUtils = checkAndInsertExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

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
	appInfo.apiVersion         = ConvertVersion(createInfo.apiVersion);

	std::vector<VkBaseOutStructure*> nextChain = createInfo.nextChain;

	if (hasDebugUtils)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugUtilsMessengerCI.messageSeverity                    = createInfo.debugMessageSeverity;
		debugUtilsMessengerCI.messageType                        = createInfo.debugMessageType;
		debugUtilsMessengerCI.pfnUserCallback                    = createInfo.debugCallback;
		debugUtilsMessengerCI.pUserData                          = createInfo.debugUserDataPointer;
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&debugUtilsMessengerCI));
	}

	if (createInfo.enabledValidationFeatures.size() != 0 || createInfo.disabledValidationFeatures.size())
	{
		VkValidationFeaturesEXT features        = { .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
		features.enabledValidationFeatureCount  = static_cast<uint32_t>(createInfo.enabledValidationFeatures.size());
		features.pEnabledValidationFeatures     = createInfo.enabledValidationFeatures.data();
		features.disabledValidationFeatureCount = static_cast<uint32_t>(createInfo.disabledValidationFeatures.size());
		features.pDisabledValidationFeatures    = createInfo.disabledValidationFeatures.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&features));
	}

	if (createInfo.disabledValidationChecks.size() != 0)
	{
		VkValidationFlagsEXT checks         = { .sType = VK_STRUCTURE_TYPE_VALIDATION_FLAGS_EXT };
		checks.disabledValidationCheckCount = static_cast<uint32_t>(createInfo.disabledValidationChecks.size());
		checks.pDisabledValidationChecks    = createInfo.disabledValidationChecks.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&checks));
	}

	// If layer settings extension enabled by sample, then activate layer settings during instance creation
	if (std::find(extensions.begin(), extensions.end(), VK_EXT_LAYER_SETTINGS_EXTENSION_NAME) != extensions.end())
	{
		VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = { .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT };
		layerSettingsCreateInfo.settingCount                 = static_cast<uint32_t>(createInfo.requiredLayerSettings.size());
		layerSettingsCreateInfo.pSettings                    = createInfo.requiredLayerSettings.data();
		nextChain.push_back(reinterpret_cast<VkBaseOutStructure*>(&layerSettingsCreateInfo));
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
	if (portabilityEnumerationSupport) instanceCreateInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkResult result = vkCreateInstance(&instanceCreateInfo, createInfo.allocator, &m_instance);
	if (!resultCheck(result, "Could not create Vulkan instance")) return;

	volkLoadInstance(m_instance);

	if (hasDebugUtils)
	{
		VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debugUtilsMessengerCI.messageSeverity                    = createInfo.debugMessageSeverity;
		debugUtilsMessengerCI.messageType                        = createInfo.debugMessageType;
		debugUtilsMessengerCI.pfnUserCallback                    = createInfo.debugCallback;
		debugUtilsMessengerCI.pUserData                          = createInfo.debugUserDataPointer;
		result = vkCreateDebugUtilsMessengerEXT(m_instance, &debugUtilsMessengerCI, m_allocator, &m_debugMessenger);
		if (!resultCheck(result, "Could not create debug utils messenger")) return;
	}

	m_isValid = true;
}

Instance::~Instance()
{
	if (m_debugMessenger) vkDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_allocator);
	if (m_instance)       vkDestroyInstance(m_instance, nullptr);
}

bool Instance::IsValid() const
{
	return m_isValid && m_instance != nullptr;
}

SurfacePtr Instance::CreateSurface(const SurfaceCreateInfo& createInfo)
{
	VkSurfaceKHR vkSurface{ nullptr };
#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{ .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hinstance = createInfo.hinstance;
	surfaceCreateInfo.hwnd = createInfo.hwnd;
	VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, m_allocator, &vkSurface);
#endif
	if (!resultCheck(result, "Create Surface")) return nullptr;

	SurfacePtr surface = std::make_shared<Surface>(shared_from_this(), vkSurface);
	if (!surface || !surface->IsValid()) return nullptr;

	return surface;
}

std::vector<PhysicalDevicePtr> Instance::GetPhysicalDevices()
{
	std::vector<VkPhysicalDevice> vkPhysicalDevices;
	auto vkresult = GetVector<VkPhysicalDevice>(vkPhysicalDevices, vkEnumeratePhysicalDevices, m_instance);
	if (vkresult != VK_SUCCESS || vkPhysicalDevices.size() == 0)
	{
		error("Couldn't find a physical device that supports Vulkan.");
		return {};
	}

	std::vector<PhysicalDevicePtr> physicalDevices(vkPhysicalDevices.size());
	for (size_t i = 0; i < vkPhysicalDevices.size(); i++)
		physicalDevices[i] = std::make_shared<PhysicalDevice>(shared_from_this(), vkPhysicalDevices[i]);

	return physicalDevices;
}

DevicePtr Instance::CreateDevice(PhysicalDevicePtr physicalDevice)
{
	auto resource = std::make_shared<Device>(shared_from_this(), physicalDevice);
	if (!resource || !resource->IsValid()) return nullptr;
	return resource;
}

bool Instance::checkValid(const InstanceCreateInfo& createInfo)
{
	if (!IsValidState()) return false;
	if (createInfo.apiVersion == VulkanAPIVersion::Version10)
	{
		error("Vulkan 1.0 not support!");
		return false;
	}

	if (!m_context.CheckAPIVersionSupported(createInfo.apiVersion)) return false;
	if (!m_context.CheckLayerSupported(createInfo.layers)) return false;
	if (!m_context.CheckExtensionSupported(createInfo.extensions)) return false;

	return true;
}

#pragma endregion

//=============================================================================
#pragma region [ Surface ]

Surface::Surface(InstancePtr instance, VkSurfaceKHR surface)
	: m_instance(instance)
	, m_surface(surface)
{
}

Surface::~Surface()
{
	vkDestroySurfaceKHR(*m_instance, m_surface, *m_instance);
}

#pragma endregion

//=============================================================================
#pragma region [ PhysicalDevice ]

PhysicalDevice::PhysicalDevice(InstancePtr instance, VkPhysicalDevice vkPhysicalDevice)
	: m_instance(instance)
	, m_device(vkPhysicalDevice)
{
	vkGetPhysicalDeviceFeatures(m_device, &m_features);
	vkGetPhysicalDeviceProperties(m_device, &m_properties);
	vkGetPhysicalDeviceMemoryProperties(m_device, &m_memoryProperties);

	m_queueFamilies = GetVectorNoError<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, m_device);

	// TODO: а может не нужно их хранить?
	std::vector<VkExtensionProperties> deviceExtensions;
	auto result = GetVector<VkExtensionProperties>(deviceExtensions, vkEnumerateDeviceExtensionProperties, m_device, nullptr);
	if (result != VK_SUCCESS)
	{
		error("vkEnumerateDeviceExtensionProperties");
		return;
	}
	m_availableExtensions.resize(deviceExtensions.size());
	for (size_t i = 0; i < deviceExtensions.size(); i++)
	{
		m_availableExtensions[i] = deviceExtensions[i].extensionName;
	}
}

PhysicalDeviceType PhysicalDevice::GetDeviceType() const
{
	switch (m_properties.deviceType)
	{
	case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return PhysicalDeviceType::Other;
	case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return PhysicalDeviceType::IntegratedGPU;
	case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return PhysicalDeviceType::DiscreteGPU;
	case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return PhysicalDeviceType::VirtualGPU;
	case VK_PHYSICAL_DEVICE_TYPE_CPU:            return PhysicalDeviceType::CPU;
	default: break;
	}
	return PhysicalDeviceType::Other;
}

std::vector<VkSurfaceFormatKHR> PhysicalDevice::GetSupportSurfaceFormat(SurfacePtr surface) const
{
	std::vector<VkSurfaceFormatKHR> formats;
	VkResult result = GetVector<VkSurfaceFormatKHR>(formats, vkGetPhysicalDeviceSurfaceFormatsKHR, m_device, *surface);
	resultCheck(result, "PhysicalDevice::GetSupportSurfaceFormat");
	return formats;
}

std::vector<VkPresentModeKHR> PhysicalDevice::GetSupportPresentMode(SurfacePtr surface) const
{
	std::vector<VkPresentModeKHR> presentModes;
	VkResult result = GetVector<VkPresentModeKHR>(presentModes, vkGetPhysicalDeviceSurfacePresentModesKHR, m_device, *surface);
	resultCheck(result, "PhysicalDevice::GetSupportPresentMode");
	return presentModes;
}

const std::vector<std::string>& PhysicalDevice::GetDeviceExtensions() const
{
	return m_availableExtensions;
}

const std::vector<VkQueueFamilyProperties>& PhysicalDevice::GetQueueFamilyProperties() const
{
	return m_queueFamilies;
}

//bool PhysicalDevice::CheckExtensionSupported(const std::string& requestedExtension) const
//{
//	return std::find_if(m_availableExtensions.begin(), m_availableExtensions.end(), [requestedExtension](auto& device_extension) {
//		return std::strcmp(device_extension.c_str(), requestedExtension.c_str()) == 0;
//		}) != m_availableExtensions.end();
//}
//
//bool PhysicalDevice::CheckExtensionSupported(const std::vector<std::string>& requestedExtensions) const
//{
//	bool allFound = true;
//	for (const auto& extensionName : requestedExtensions)
//	{
//		bool found = CheckExtensionSupported(extensionName);
//		if (!found) allFound = false;
//	}
//	return allFound;
//}

std::vector<std::string> vkw::PhysicalDevice::CheckDeviceExtensionSupport(const std::vector<std::string>& availableExtensions, const std::vector<std::string>& desiredExtensions)
{
	std::vector<std::string> extensions_to_enable;
	for (const auto& avail_ext : availableExtensions)
	{
		for (auto& req_ext : desiredExtensions)
		{
			if (avail_ext == req_ext)
			{
				extensions_to_enable.push_back(req_ext);
				break;
			}
		}
	}
	return extensions_to_enable;
}

const VkFormatProperties PhysicalDevice::GetFormatProperties(VkFormat format) const
{
	VkFormatProperties format_properties;
	vkGetPhysicalDeviceFormatProperties(m_device, format, &format_properties);
	return format_properties;
}

std::optional<uint32_t> PhysicalDevice::GetQueueFamilyIndex(VkQueueFlagBits queueFlag) const
{
	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if (queueFlag & VK_QUEUE_COMPUTE_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
		{
			if ((m_queueFamilies[i].queueFlags & queueFlag) && !(m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			{
				return i;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if (queueFlag & VK_QUEUE_TRANSFER_BIT)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
		{
			if ((m_queueFamilies[i].queueFlags & queueFlag) && !(m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && !(m_queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
			{
				return i;
			}
		}
	}

	for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
	{
		if (m_queueFamilies[i].queueFlags & queueFlag)
		{
			return i;
		}
	}

	return std::nullopt;
}

std::optional<uint32_t> PhysicalDevice::GetDedicatedQueueIndex(VkQueueFlags desiredFlags, VkQueueFlags undesiredFlags) const
{
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
	{
		if (   (m_queueFamilies[i].queueFlags & desiredFlags) == desiredFlags
			&& (m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0
			&& (m_queueFamilies[i].queueFlags & undesiredFlags) == 0)
			return i;
	}
	return std::nullopt;
}

std::optional<uint32_t> PhysicalDevice::GetSeparateQueueIndex(VkQueueFlags desiredFlags, VkQueueFlags undesiredFlags) const
{
	constexpr uint32_t MaxIndex = 65536;
	uint32_t index = MaxIndex;
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
	{
		if (    (m_queueFamilies[i].queueFlags & desiredFlags) == desiredFlags 
			&& ((m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
		{
			if ((m_queueFamilies[i].queueFlags & undesiredFlags) == 0) return i;
			else index = i;
		}
	}
	if (index >= MaxIndex) return std::nullopt;

	return index;
}

std::optional<uint32_t> PhysicalDevice::FindGraphicsQueueFamilyIndex() const
{
	// get the first index into queueFamiliyProperties which supports graphics
	for (uint32_t i = 0; i < static_cast<uint32_t>(m_queueFamilies.size()); i++)
	{
		if (m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			return i;
		}
	}
	error("Graphics Queue Family not find");
	return std::nullopt;
}

std::optional<std::pair<uint32_t, uint32_t>> vkw::PhysicalDevice::FindGraphicsAndPresentQueueFamilyIndex(SurfacePtr surface) const
{
	std::optional<uint32_t> graphicsQueueFamilyIndex = FindGraphicsQueueFamilyIndex();
	if (!graphicsQueueFamilyIndex.has_value()) return std::nullopt;

	if (GetSurfaceSupportKHR(graphicsQueueFamilyIndex.value(), surface))
	{
		// the first graphicsQueueFamilyIndex does also support presents
		return std::make_pair(graphicsQueueFamilyIndex.value(), graphicsQueueFamilyIndex.value());
	}

	// the graphicsQueueFamilyIndex doesn't support present -> look for an other family index that supports both graphics and present
	for (size_t i = 0; i < m_queueFamilies.size(); i++)
	{
		if ((m_queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && GetSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
		{
			return std::make_pair(static_cast<uint32_t>(i), static_cast<uint32_t>(i));
		}
	}

	// there's nothing like a single family index that supports both graphics and present -> look for an other family index that supports present
	for (size_t i = 0; i < m_queueFamilies.size(); i++)
	{
		if (GetSurfaceSupportKHR(static_cast<uint32_t>(i), surface))
		{
			return std::make_pair(graphicsQueueFamilyIndex.value(), static_cast<uint32_t>(i));
		}
	}

	error("Could not find queues for both graphics or present -> terminating");
	return std::nullopt;
}

bool vkw::PhysicalDevice::GetSurfaceSupportKHR(uint32_t queueFamilyIndex, SurfacePtr surface) const
{
	VkBool32 supported{ VK_FALSE };
	if (surface != nullptr)
	{
		VkResult result = vkGetPhysicalDeviceSurfaceSupportKHR(m_device, queueFamilyIndex, *surface, &supported);
		if (!resultCheck(result, "PhysicalDevice::GetSurfaceSupportKHR")) return false;
	}
	return supported == VK_TRUE;
}

uint32_t PhysicalDevice::GetQueueFamilyPerformanceQueryPasses(const VkQueryPoolPerformanceCreateInfoKHR* perfQueryCreateInfo) const
{
	uint32_t passes_needed;
	vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR(m_device, perfQueryCreateInfo, &passes_needed);
	return passes_needed;
}

void PhysicalDevice::EnumerateQueueFamilyPerformanceQueryCounters(uint32_t queueFamilyIndex, uint32_t* count, VkPerformanceCounterKHR* counters, VkPerformanceCounterDescriptionKHR* descriptions) const
{
	VkResult result = vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR(m_device, queueFamilyIndex, count, counters, descriptions);
	resultCheck(result, "EnumerateQueueFamilyPerformanceQueryCounters");
}

bool vkw::PhysicalDevice::SupportsFeatures(const VkPhysicalDeviceFeatures& requested, const GenericFeatureChain& extensionSupported, const GenericFeatureChain& extensionRequested)
{
	if (requested.robustBufferAccess && !m_features.robustBufferAccess) return false;
	if (requested.fullDrawIndexUint32 && !m_features.fullDrawIndexUint32) return false;
	if (requested.imageCubeArray && !m_features.imageCubeArray) return false;
	if (requested.independentBlend && !m_features.independentBlend) return false;
	if (requested.geometryShader && !m_features.geometryShader) return false;
	if (requested.tessellationShader && !m_features.tessellationShader) return false;
	if (requested.sampleRateShading && !m_features.sampleRateShading) return false;
	if (requested.dualSrcBlend && !m_features.dualSrcBlend) return false;
	if (requested.logicOp && !m_features.logicOp) return false;
	if (requested.multiDrawIndirect && !m_features.multiDrawIndirect) return false;
	if (requested.drawIndirectFirstInstance && !m_features.drawIndirectFirstInstance) return false;
	if (requested.depthClamp && !m_features.depthClamp) return false;
	if (requested.depthBiasClamp && !m_features.depthBiasClamp) return false;
	if (requested.fillModeNonSolid && !m_features.fillModeNonSolid) return false;
	if (requested.depthBounds && !m_features.depthBounds) return false;
	if (requested.wideLines && !m_features.wideLines) return false;
	if (requested.largePoints && !m_features.largePoints) return false;
	if (requested.alphaToOne && !m_features.alphaToOne) return false;
	if (requested.multiViewport && !m_features.multiViewport) return false;
	if (requested.samplerAnisotropy && !m_features.samplerAnisotropy) return false;
	if (requested.textureCompressionETC2 && !m_features.textureCompressionETC2) return false;
	if (requested.textureCompressionASTC_LDR && !m_features.textureCompressionASTC_LDR) return false;
	if (requested.textureCompressionBC && !m_features.textureCompressionBC) return false;
	if (requested.occlusionQueryPrecise && !m_features.occlusionQueryPrecise) return false;
	if (requested.pipelineStatisticsQuery && !m_features.pipelineStatisticsQuery) return false;
	if (requested.vertexPipelineStoresAndAtomics && !m_features.vertexPipelineStoresAndAtomics) return false;
	if (requested.fragmentStoresAndAtomics && !m_features.fragmentStoresAndAtomics) return false;
	if (requested.shaderTessellationAndGeometryPointSize && !m_features.shaderTessellationAndGeometryPointSize) return false;
	if (requested.shaderImageGatherExtended && !m_features.shaderImageGatherExtended) return false;
	if (requested.shaderStorageImageExtendedFormats && !m_features.shaderStorageImageExtendedFormats) return false;
	if (requested.shaderStorageImageMultisample && !m_features.shaderStorageImageMultisample) return false;
	if (requested.shaderStorageImageReadWithoutFormat && !m_features.shaderStorageImageReadWithoutFormat) return false;
	if (requested.shaderStorageImageWriteWithoutFormat && !m_features.shaderStorageImageWriteWithoutFormat) return false;
	if (requested.shaderUniformBufferArrayDynamicIndexing && !m_features.shaderUniformBufferArrayDynamicIndexing) return false;
	if (requested.shaderSampledImageArrayDynamicIndexing && !m_features.shaderSampledImageArrayDynamicIndexing) return false;
	if (requested.shaderStorageBufferArrayDynamicIndexing && !m_features.shaderStorageBufferArrayDynamicIndexing) return false;
	if (requested.shaderStorageImageArrayDynamicIndexing && !m_features.shaderStorageImageArrayDynamicIndexing) return false;
	if (requested.shaderClipDistance && !m_features.shaderClipDistance) return false;
	if (requested.shaderCullDistance && !m_features.shaderCullDistance) return false;
	if (requested.shaderFloat64 && !m_features.shaderFloat64) return false;
	if (requested.shaderInt64 && !m_features.shaderInt64) return false;
	if (requested.shaderInt16 && !m_features.shaderInt16) return false;
	if (requested.shaderResourceResidency && !m_features.shaderResourceResidency) return false;
	if (requested.shaderResourceMinLod && !m_features.shaderResourceMinLod) return false;
	if (requested.sparseBinding && !m_features.sparseBinding) return false;
	if (requested.sparseResidencyBuffer && !m_features.sparseResidencyBuffer) return false;
	if (requested.sparseResidencyImage2D && !m_features.sparseResidencyImage2D) return false;
	if (requested.sparseResidencyImage3D && !m_features.sparseResidencyImage3D) return false;
	if (requested.sparseResidency2Samples && !m_features.sparseResidency2Samples) return false;
	if (requested.sparseResidency4Samples && !m_features.sparseResidency4Samples) return false;
	if (requested.sparseResidency8Samples && !m_features.sparseResidency8Samples) return false;
	if (requested.sparseResidency16Samples && !m_features.sparseResidency16Samples) return false;
	if (requested.sparseResidencyAliased && !m_features.sparseResidencyAliased) return false;
	if (requested.variableMultisampleRate && !m_features.variableMultisampleRate) return false;
	if (requested.inheritedQueries && !m_features.inheritedQueries) return false;

	return extensionSupported.MatchAll(extensionRequested);
}

#pragma endregion

//=============================================================================
#pragma region [ Device ]

Device::Device(InstancePtr instance, PhysicalDevicePtr physicalDevice)
	: m_instance(instance)
	, m_physicalDevice(physicalDevice)
{
	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueCreateInfo = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	//queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo   = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	createInfo.pQueueCreateInfos    = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pEnabledFeatures     = &deviceFeatures;


	VkResult result = vkCreateDevice(*m_physicalDevice, &createInfo, *m_instance, &m_device);
	if (!resultCheck(result, "failed to create logical device!")) return;
}

Device::~Device()
{
	if (m_device) vkDestroyDevice(m_device, nullptr);
}

#pragma endregion

} // namespace vkw