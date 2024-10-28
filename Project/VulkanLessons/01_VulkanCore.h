#pragma once

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

//============================================================================
// 0001 Lesson
//============================================================================

class PhysicalDevice;


inline VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
{
	auto ms = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
	auto mt = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
	printf("[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

	return VK_FALSE;
}

inline void Message(const std::string& msg)
{
	puts(msg.c_str());
}

inline void Fatal(const std::string& msg)
{
	throw std::exception(msg.c_str());
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

inline std::vector<VkLayerProperties> QueryInstanceLayers()
{
	auto result = QueryWithMethod<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
	CheckResult(result.first, "vkEnumerateInstanceLayerProperties");
	return result.second;
}

inline std::vector<VkExtensionProperties> QueryInstanceExtensions()
{
	auto result = QueryWithMethod<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
	CheckResult(result.first, "vkEnumerateInstanceExtensionProperties");
	return result.second;
}

inline bool CheckInstanceLayerSupported(const char* layerName, const std::vector<VkLayerProperties>& availableLayers)
{
	if (!layerName) return false;
	for (const auto& layerProperties : availableLayers)
		if (strcmp(layerName, layerProperties.layerName) == 0) return true;
	Message("Instance Layer: " + std::string(layerName) + " not support");
	return false;
}

inline bool CheckInstanceLayerSupported(const std::vector<const char*>& layerNames, const std::vector<VkLayerProperties>& availableLayers)
{
	bool allFound = true;
	for (const auto& layerName : layerNames)
	{
		bool found = CheckInstanceLayerSupported(layerName, availableLayers);
		if (!found) allFound = false;
	}
	return allFound;
}

inline bool CheckInstanceExtensionSupported(const char* extensionName, const std::vector<VkExtensionProperties>& availableExtensions)
{
	if (!extensionName) return false;
	for (const auto& extensionProperties : availableExtensions)
		if (strcmp(extensionName, extensionProperties.extensionName)) return true;
	Message("Instance Extension: " + std::string(extensionName) + " not support");
	return false;
}

inline bool CheckInstanceExtensionSupported(const std::vector<const char*>& extensionNames, const std::vector<VkExtensionProperties>& availableExtensions)
{
	bool allFound = true;
	for (const auto& extensionName : extensionNames)
	{
		bool found = CheckInstanceExtensionSupported(extensionName, availableExtensions);
		if (!found) allFound = false;
	}
	return allFound;
}

//============================================================================
// xxxx Lesson
//============================================================================