﻿#pragma once

//=============================================================================
#pragma region [ Macros ]

#if !defined( VULKAN_WRAPPER_ASSERT )
#  define VULKAN_WRAPPER_ASSERT assert
#endif

#pragma endregion

//=============================================================================
#pragma region [ Header ]

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#	pragma warning(disable : 4668)
#	pragma warning(disable : 5039)
#endif

#if ( VULKAN_WRAPPER_ASSERT == assert )
#	include <cassert>
#endif

#include <cstdint>

#include <memory>
#include <string>
#include <vector>

#if defined(_WIN32)
#	define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#include <volk/volk.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion

namespace vkw {

//=============================================================================
#pragma region [ Forward Declarations ]

class Context;
class Instance;
class PhysicalDevice;

struct InstanceCreateInfo;
struct PhysicalDeviceSelectCriteria;

using InstancePtr = std::shared_ptr<Instance>;
using PhysicalDevicePtr = std::shared_ptr<PhysicalDevice>;

#pragma endregion

//=============================================================================
#pragma region Core Func

template <typename T>
inline std::vector<uint8_t> ToBytes(const T& value)
{
	return std::vector<uint8_t>{reinterpret_cast<const uint8_t*>(&value), reinterpret_cast<const uint8_t*>(&value) + sizeof(T)};
}

#pragma endregion

//=============================================================================
#pragma region [ Core Enum ]

enum class VulkanAPIVersion : uint8_t
{
	Version10 = 1,
	Version11 = 2,
	Version12 = 3,
	Version13 = 4,
	Unknown = UINT8_MAX
};

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
#pragma region [ Core Struct ]

#pragma endregion

//=============================================================================
#pragma region [ Vulkan Core Func ]

[[nodiscard]] VulkanAPIVersion ConvertVersion(uint32_t vkVersion);
[[nodiscard]] uint32_t         ConvertVersion(VulkanAPIVersion version);

template <typename T>
void SetupPNextChain(T& structure, const std::vector<VkBaseOutStructure*>& structs)
{
	structure.pNext = nullptr;
	if (structs.empty()) return;
	for (size_t i = 0; i < structs.size() - 1; i++)
		structs.at(i)->pNext = structs.at(i + 1);
	structure.pNext = structs.at(0);
}

template <typename T, typename F, typename... Ts> 
VkResult GetVector(std::vector<T>& out, F&& f, Ts&&... ts)
{
	uint32_t count = 0;
	VkResult err;
	do {
		err = f(ts..., &count, nullptr);
		if (err != VK_SUCCESS) return err;
		out.resize(count);
		err = f(ts..., &count, out.data());
		out.resize(count);
	} while (err == VK_INCOMPLETE);
	return err;
}

template <typename T, typename F, typename... Ts>
std::vector<T> GetVectorNoError(F&& f, Ts&&... ts)
{
	uint32_t count = 0;
	std::vector<T> results;
	f(ts..., &count, nullptr);
	results.resize(count);
	f(ts..., &count, results.data());
	return results;
}

VKAPI_ATTR VkBool32 VKAPI_CALL DefaultDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

#pragma endregion

//=============================================================================
#pragma region [ Error ]

[[nodiscard]] bool        IsValidState();
[[nodiscard]] const char* GetLastErrorText();

#pragma endregion

//=============================================================================
#pragma region [ Context ]

class Context final
{
public:
	Context();
	Context(const Context&) = delete;
	Context(Context&&) = default;
	~Context();

	Context& operator=(const Context&) = delete;
	Context& operator=(Context&&) = default;

	[[nodiscard]] VulkanAPIVersion                   EnumerateInstanceVersion() const;
	[[nodiscard]] std::vector<VkLayerProperties>     EnumerateInstanceLayerProperties() const;
	[[nodiscard]] std::vector<VkExtensionProperties> EnumerateInstanceExtensionProperties(const char* layerName = nullptr) const;

	[[nodiscard]] bool                               CheckAPIVersionSupported(VulkanAPIVersion version);

	[[nodiscard]] bool                               CheckLayerSupported(const char* layerName);
	[[nodiscard]] bool                               CheckLayerSupported(const std::vector<VkLayerProperties>& availableLayers, const char* layerName);
	[[nodiscard]] bool                               CheckLayerSupported(const std::vector<const char*>& layerNames);

	[[nodiscard]] bool                               CheckExtensionSupported(const char* extensionName);
	[[nodiscard]] bool                               CheckExtensionSupported(const std::vector<VkExtensionProperties>& availableExtensions, const char* extensionName);
	[[nodiscard]] bool                               CheckExtensionSupported(const std::vector<const char*>& extensionNames);

	[[nodiscard]] InstancePtr                        CreateInstance(const InstanceCreateInfo& createInfo);

private:
	static uint64_t s_refCount;
};

#pragma endregion

//=============================================================================
#pragma region [ Instance ]

struct InstanceCreateInfo final
{
	InstanceCreateInfo() = default;
	InstanceCreateInfo(const InstanceCreateInfo&) = default;

	std::string                                applicationName;
	std::string                                engineName;
	uint32_t                                   applicationVersion{ 0 };
	uint32_t                                   engineVersion{ 0 };
	VulkanAPIVersion                           apiVersion{ VulkanAPIVersion::Version13 };
	std::vector<const char*>                   layers{};
	std::vector<const char*>                   extensions{};
	VkInstanceCreateFlags                      flags = static_cast<VkInstanceCreateFlags>(0);
	std::vector<VkBaseOutStructure*>           nextElements;

	bool                                       enableValidationLayers{ false };

	bool                                       useDebugMessenger{ false };
	PFN_vkDebugUtilsMessengerCallbackEXT       debugCallback{ DefaultDebugCallback };
	VkDebugUtilsMessageSeverityFlagsEXT        debugMessageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	VkDebugUtilsMessageTypeFlagsEXT            debugMessageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
																VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
																VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	void*                                      debugUserDataPointer = nullptr;

	std::vector<VkBaseOutStructure*>           nextChain;

	// validation features
	std::vector<VkValidationCheckEXT>          disabledValidationChecks;
	std::vector<VkValidationFeatureEnableEXT>  enabledValidationFeatures;
	std::vector<VkValidationFeatureDisableEXT> disabledValidationFeatures;

	std::vector<VkLayerSettingEXT>             requiredLayerSettings;

	// Custom allocator
	VkAllocationCallbacks*                     allocator = nullptr;
};

class Instance final : public std::enable_shared_from_this<Instance>
{
	friend Context;
public:
	Instance() = delete;
	Instance(const Instance&) = default;
	Instance(const InstanceCreateInfo& createInfo);
	~Instance();

	Instance& operator=(const Instance&) = default;

	[[nodiscard]] bool IsValid() const;

	[[nodiscard]] std::vector<PhysicalDevicePtr> GetPhysicalDevices();
	[[nodiscard]] PhysicalDevicePtr GetDeviceSuitable(const PhysicalDeviceSelectCriteria& criteria);

private:
	bool checkValid(const InstanceCreateInfo& createInfo);
	VkInstance               m_instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	VkAllocationCallbacks*   m_allocator{ VK_NULL_HANDLE };
	Context                  m_context;
	bool                     m_isValid{ false };
};

#pragma endregion

//=============================================================================
#pragma region [ PhysicalDevice ]

struct PhysicalDeviceSelectCriteria final
{

};

class PhysicalDevice final
{
public:
	PhysicalDevice() = delete;
	PhysicalDevice(InstancePtr instance, VkPhysicalDevice vkPhysicalDevice);
	~PhysicalDevice() = default;

	[[nodiscard]] const VkPhysicalDeviceFeatures& GetFeatures() const { return m_features; }
	[[nodiscard]] const VkPhysicalDeviceProperties& GetProperties() const { m_properties; }
	[[nodiscard]] const VkPhysicalDeviceLimits& GetLimits() const { m_properties.limits; }
	[[nodiscard]] const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_memoryProperties; }

	[[nodiscard]] std::string GetDeviceName() const { return m_properties.deviceName; }
	[[nodiscard]] PhysicalDeviceType GetDeviceType() const;

	[[nodiscard]] std::vector<VkExtensionProperties> GetDeviceExtensions() const;
	[[nodiscard]] std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties() const;

private:
	InstancePtr                      m_instance{ nullptr };
	VkPhysicalDevice                 m_device{ VK_NULL_HANDLE };
	VkPhysicalDeviceFeatures         m_features{};
	VkPhysicalDeviceProperties       m_properties{};
	VkPhysicalDeviceMemoryProperties m_memoryProperties{};
};

#pragma endregion

} // namespace vkw