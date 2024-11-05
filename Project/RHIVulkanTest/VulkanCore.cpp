#include "VulkanCore.h"

namespace vkf {

//=============================================================================
#pragma region [ Debug Utils ]

void DefaultMessage(const std::string& msg)
{
	puts(msg.c_str());
}

void DefaultFatal(const std::string& msg)
{
	puts(msg.c_str());
	exit(1);
}

namespace
{
	std::function<void(const std::string&)> messageFunc = DefaultMessage;
	std::function<void(const std::string&)> fatalFunc = DefaultFatal;
}

void SetMessageFunc(std::function<void(const std::string&)> func)
{
	messageFunc = func;
}

void SetFatalMessageFunc(std::function<void(const std::string&)> func)
{
	fatalFunc = func;
}

void Message(const std::string& msg)
{
	messageFunc(msg);
}

void Fatal(const std::string& msg)
{
	fatalFunc(msg);
}

VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData)
{
	auto ms = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
	auto mt = string_VkDebugUtilsMessageTypeFlagsEXT(messageType);
	std::string message = "[" + std::string(ms) + ": " + mt + "]\n" + pCallbackData->pMessage;
	Message(message);

	return VK_FALSE;
}

#pragma endregion

//=============================================================================
#pragma region [ Get Available Instance Layer and Extension]

std::optional<std::vector<VkLayerProperties>> QueryInstanceLayers()
{
	auto result = QueryWithMethod<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
	if (result.first != VK_SUCCESS)
	{
		Fatal("vkEnumerateInstanceLayerProperties");
		return std::nullopt;
	}
	return result.second;
}

std::optional<std::vector<VkExtensionProperties>> QueryInstanceExtensions()
{
	auto result = QueryWithMethod<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
	if (result.first != VK_SUCCESS)
	{
		Fatal("vkEnumerateInstanceExtensionProperties");
		return std::nullopt;
	}
	return result.second;
}

#pragma endregion

//=============================================================================
#pragma region [ Check Instance Layer and Extension Supported]

bool CheckInstanceLayerSupported(const char* layerName, const std::vector<VkLayerProperties>& availableLayers)
{
	if (!layerName) return false;
	for (const auto& layerProperties : availableLayers)
		if (strcmp(layerName, layerProperties.layerName) == 0) return true;
	Message("Instance Layer: " + std::string(layerName) + " not support");
	return false;
}

bool CheckInstanceLayerSupported(const std::vector<const char*>& layerNames, const std::vector<VkLayerProperties>& availableLayers)
{
	bool allFound = true;
	for (const auto& layerName : layerNames)
	{
		bool found = CheckInstanceLayerSupported(layerName, availableLayers);
		if (!found) allFound = false;
	}
	return allFound;
}

bool CheckInstanceExtensionSupported(const char* extensionName, const std::vector<VkExtensionProperties>& availableExtensions)
{
	if (!extensionName) return false;
	for (const auto& extensionProperties : availableExtensions)
		if (strcmp(extensionName, extensionProperties.extensionName)) return true;
	Message("Instance Extension: " + std::string(extensionName) + " not support");
	return false;
}

bool CheckInstanceExtensionSupported(const std::vector<const char*>& extensionNames, const std::vector<VkExtensionProperties>& availableExtensions)
{
	bool allFound = true;
	for (const auto& extensionName : extensionNames)
	{
		bool found = CheckInstanceExtensionSupported(extensionName, availableExtensions);
		if (!found) allFound = false;
	}
	return allFound;
}

bool CheckDeviceExtensionSupported(const char* requestedExtensionName, const std::vector<VkExtensionProperties>& availableExtensions)
{
	if (!requestedExtensionName) return false;
	for (const auto& extensionProperties : availableExtensions)
		if (strcmp(requestedExtensionName, extensionProperties.extensionName)) return true;
	Message("Device Extension: " + std::string(requestedExtensionName) + " not support");
	return false;
}

bool CheckDeviceExtensionSupported(const std::vector<const char*>& requestedExtensions, const std::vector<VkExtensionProperties>& availableExtensions)
{
	bool allFound = true;
	for (const auto& extensionName : requestedExtensions)
	{
		bool found = CheckDeviceExtensionSupported(extensionName, availableExtensions);
		if (!found) allFound = false;
	}
	return allFound;
}

#pragma endregion

} // namespace vkf