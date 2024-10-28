#pragma once

//=============================================================================
#pragma region [ Headers ]

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#endif

#include <cassert>
#include <functional>
#include <optional>
#include <algorithm>
#include <string>
#include <vector>

#if defined(_WIN32)
#	define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion

namespace vkf {

//=============================================================================
#pragma region [ Decl Class ]

class Instance;
class PhysicalDevice;
class Surface;
class Device;

struct InstanceCreateInfo;
struct SurfaceCreateInfo;

#pragma endregion

//=============================================================================
#pragma region [ Debug Utils ]

void SetMessageFunc(std::function<void(const std::string&)> func);
void SetFatalMessageFunc(std::function<void(const std::string&)> func);
void Message(const std::string& msg);
void Fatal(const std::string& msg);

VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, [[maybe_unused]] void* pUserData);

constexpr VkDebugUtilsMessageSeverityFlagsEXT DefaultDebugMessageSeverity =
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

constexpr VkDebugUtilsMessageTypeFlagsEXT DefaultDebugMessageType =
	VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
	VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

#pragma endregion

//=============================================================================
#pragma region [ Vulkan Enum ]

enum class PhysicalDeviceType : uint8_t
{
	Other,
	IntegratedGPU,
	DiscreteGPU,
	VirtualGPU,
	CPU
};

#pragma endregion

//=============================================================================
#pragma region [ Vulkan Core Utils ]

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
void SetupPNextChain(T& baseStruct, const std::vector<VkBaseOutStructure*>& nextStructs)
{
	baseStruct.pNext = nullptr;
	if (nextStructs.empty()) return;
	for (size_t i = 0; i < nextStructs.size() - 1; i++)
		nextStructs[i]->pNext = nextStructs[i + 1];
	baseStruct.pNext = nextStructs[0];
}

template <typename TVkStruct1, typename TVkStruct2>
void InsertPNext(TVkStruct1& baseStruct, TVkStruct2& nextStruct)
{
	nextStruct.pNext = baseStruct.pNext;
	baseStruct.pNext = reinterpret_cast<VkBaseOutStructure*>(&nextStruct);
}

#pragma endregion

//=============================================================================
#pragma region [ Get Available Instance Layer and Extension]

std::optional<std::vector<VkLayerProperties>> QueryInstanceLayers();
std::optional<std::vector<VkExtensionProperties>> QueryInstanceExtensions();

#pragma endregion

//=============================================================================
#pragma region [ Check Layer and Extension Supported]

bool CheckInstanceLayerSupported(const char* layerName, const std::vector<VkLayerProperties>& availableLayers);
bool CheckInstanceLayerSupported(const std::vector<const char*>& layerNames, const std::vector<VkLayerProperties>& availableLayers);

bool CheckInstanceExtensionSupported(const char* extensionName, const std::vector<VkExtensionProperties>& availableExtensions);
bool CheckInstanceExtensionSupported(const std::vector<const char*>& extensionNames, const std::vector<VkExtensionProperties>& availableExtensions);

bool CheckDeviceExtensionSupported(const char* requestedExtensionName, const std::vector< VkExtensionProperties>& availableExtensions);
bool CheckDeviceExtensionSupported(const std::vector<const char*>& requestedExtensions, const std::vector< VkExtensionProperties>& availableExtensions);

#pragma endregion



} // namespace vkf