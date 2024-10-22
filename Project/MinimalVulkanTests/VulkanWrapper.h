#pragma once

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
class Surface;
class PhysicalDevice;

struct InstanceCreateInfo;
struct SurfaceCreateInfo;
struct PhysicalDeviceSelector;

using InstancePtr = std::shared_ptr<Instance>;
using SurfacePtr = std::shared_ptr<Surface>;
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

enum class DeviceSelectionMode : uint8_t
{
	// return all suitable and partially suitable devices
	partiallyAndFullySuitable,
	// return only physical devices which are fully suitable
	onlyFullySuitable
};

enum class Suitable : uint8_t
{ 
	yes, 
	partial, 
	no 
};

#pragma endregion

//=============================================================================
#pragma region [ Core Struct ]

// Sentinel value, used in implementation only
const uint32_t QUEUE_INDEX_MAX_VALUE = 65536;

struct GenericFeaturesPNextNode
{
	static const uint32_t fieldCapacity = 256;

	GenericFeaturesPNextNode();
	template <typename T> 
	GenericFeaturesPNextNode(const T& features) noexcept
	{
		memset(fields, UINT8_MAX, sizeof(VkBool32) * fieldCapacity);
		memcpy(this, &features, sizeof(T));
	}

	static bool Match(const GenericFeaturesPNextNode& requested, const GenericFeaturesPNextNode& supported) noexcept;

	void Combine(const GenericFeaturesPNextNode& right) noexcept;

	VkStructureType sType = static_cast<VkStructureType>(0);
	void*           pNext = nullptr;
	VkBool32        fields[fieldCapacity];
};

struct GenericFeatureChain
{
	std::vector<GenericFeaturesPNextNode> nodes;

	template <typename T> 
	void Add(const T& features) noexcept
	{
		// If this struct is already in the list, combine it
		for (auto& node : nodes)
		{
			if (static_cast<VkStructureType>(features.sType) == node.sType)
			{
				node.Combine(features);
				return;
			}
		}
		// Otherwise append to the end
		nodes.push_back(features);
	}

	bool MatchAll(const GenericFeatureChain& extensionRequested) const noexcept;
	bool FindAndMatch(const GenericFeatureChain& extensionRequested) const noexcept;
	void ChainUp(VkPhysicalDeviceFeatures2& feats2) noexcept;
	void Combine(const GenericFeatureChain& right) noexcept;
};

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

	operator VkInstance() const { return m_instance; }
	operator VkAllocationCallbacks*() const { return m_allocator; }

	[[nodiscard]] SurfacePtr CreateSurface(const SurfaceCreateInfo& createInfo);


	[[nodiscard]] std::vector<PhysicalDevicePtr> GetPhysicalDevices();
	[[deprecated]] PhysicalDevicePtr GetDeviceSuitable(const PhysicalDeviceSelector& criteria);

private:
	bool checkValid(const InstanceCreateInfo& createInfo);

	// TODO: в селектор
	PhysicalDevicePtr populateDeviceDetails(PhysicalDevicePtr physDevice, const PhysicalDeviceSelector& criteria, const GenericFeatureChain& srcExtendedFeaturesChain) const;
	// TODO: в селектор
	Suitable isDeviceSuitable(PhysicalDevicePtr physDevice, const PhysicalDeviceSelector& criteria) const;

	VkInstance               m_instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	VkAllocationCallbacks*   m_allocator{ VK_NULL_HANDLE };
	Context                  m_context;
	bool                     m_isValid{ false };
};

#pragma endregion

//=============================================================================
#pragma region [ Surface ]

struct SurfaceCreateInfo final
{
#if defined(_WIN32)
	HINSTANCE hinstance{ nullptr };
	HWND      hwnd{ nullptr };
#endif
};

class Surface final
{
	friend class Instance;
public:
	Surface() = delete;
	Surface(InstancePtr instance, VkSurfaceKHR surface);
	~Surface();

	operator VkSurfaceKHR() const { return m_surface; }

	[[nodiscard]] bool IsValid() const { return m_surface != nullptr; }

private:
	InstancePtr  m_instance{ nullptr };
	VkSurfaceKHR m_surface{ nullptr };
};

#pragma endregion

//=============================================================================
#pragma region [ PhysicalDevice ]

// TODO: удалить функции
// TODO: вообще удалить
struct PhysicalDeviceSelector final
{
	DeviceSelectionMode selection = DeviceSelectionMode::onlyFullySuitable;

	// Set the surface in which the physical device should render to.
	// Be sure to set it if swapchain functionality is to be used.
	PhysicalDeviceSelector& SetSurface(SurfacePtr surface);
	
	// Set the name of the device to select.
	PhysicalDeviceSelector& SetName(const std::string& name);
	// Set the desired physical device type to select. Defaults to PreferredDeviceType::discrete.
	PhysicalDeviceSelector& PreferGPUDeviceType(PhysicalDeviceType type = PhysicalDeviceType::DiscreteGPU);
	// Allow selection of a gpu device type that isn't the preferred physical device type. Defaults to true.
	PhysicalDeviceSelector& AllowAnyGPUDeviceType(bool allowAnyType = true);

	// Require that a physical device supports presentation. Defaults to true.
	PhysicalDeviceSelector& RequirePresent(bool require = true);

	// Require a queue family that supports transfer operations but not graphics nor compute.
	PhysicalDeviceSelector& RequireDedicatedTransferQueue();
	// Require a queue family that supports compute operations but not graphics nor transfer.
	PhysicalDeviceSelector& RequireDedicatedComputeQueue();

	// Require a queue family that supports transfer operations but not graphics.
	PhysicalDeviceSelector& RequireSeparateTransferQueue();
	// Require a queue family that supports compute operations but not graphics.
	PhysicalDeviceSelector& RequireSeparateComputeQueue();

	// Require a memory heap from VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with `size` memory available.
	PhysicalDeviceSelector& RequiredDeviceMemorySize(VkDeviceSize size);

	// Require a physical device which supports a specific extension.
	PhysicalDeviceSelector& AddRequiredExtension(const char* extension);
	// Require a physical device which supports a set of extensions.
	PhysicalDeviceSelector& AddRequiredExtensions(const std::vector<const char*>& extensions);

	// Require a physical device that supports a (major, minor) version of vulkan.
	PhysicalDeviceSelector& SetMinimumVersion(uint32_t major, uint32_t minor);

	// Require a physical device which supports a specific set of general/extension features.
	// If this function is used, the user should not put their own VkPhysicalDeviceFeatures2 in
	// the pNext chain of VkDeviceCreateInfo.
	template <typename T> 
	PhysicalDeviceSelector& AddRequiredExtensionFeatures(const T& features)
	{
		extendedFeaturesChain.Add(features);
		return *this;
	}

	// TODO: возможно удалить эти методы и использовать AddRequiredExtensionFeatures
	// Require a physical device which supports the features in VkPhysicalDeviceFeatures.
	PhysicalDeviceSelector& SetRequiredFeatures(const VkPhysicalDeviceFeatures& features);
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan11Features.
	PhysicalDeviceSelector& SetRequiredFeatures11(const VkPhysicalDeviceVulkan11Features& features11);
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan12Features.
	PhysicalDeviceSelector& SetRequiredFeatures12(const VkPhysicalDeviceVulkan12Features& features12);
	// Require a physical device which supports the features in VkPhysicalDeviceVulkan13Features.
	PhysicalDeviceSelector& SetRequiredFeatures13(const VkPhysicalDeviceVulkan13Features& features13);

	// Used when surface creation happens after physical device selection.
	// Warning: This disables checking if the physical device supports a given surface.
	PhysicalDeviceSelector& DeferSurfaceInitialization(); // TODO: удалить?

private:

	friend class Instance;
	friend class PhysicalDevice;

	SurfacePtr surface{ nullptr };

	// Set the name of the device to select.
	std::string deviceName;
	PhysicalDeviceType preferredGPUDeviceType = PhysicalDeviceType::DiscreteGPU;
	bool allowAnyType = false;
	bool requirePresent = true;
	bool requireDedicatedTransferQueue = false;
	bool requireDedicatedComputeQueue = false;
	bool requireSeparateTransferQueue = false;
	bool requireSeparateComputeQueue = false;
	VkDeviceSize requiredMemSize = 0;

	std::vector<std::string> requiredExtensions;

	uint32_t requiredVersion = VK_API_VERSION_1_3;

	VkPhysicalDeviceFeatures requiredFeatures{};
	VkPhysicalDeviceFeatures2 requiredFeatures2{};

	GenericFeatureChain extendedFeaturesChain;
	bool deferSurfaceInitialization = false;
	bool enablePortabilitySubset = true; // TODO: разобраться с этим
};

class PhysicalDevice final
{
	friend class Instance;
public:
	PhysicalDevice() = delete;
	PhysicalDevice(InstancePtr instance, VkPhysicalDevice vkPhysicalDevice);
	~PhysicalDevice() = default;

	operator VkPhysicalDevice() const { return m_device; }

	[[nodiscard]] const VkPhysicalDeviceFeatures& GetFeatures() const { return m_features; }
	[[nodiscard]] const VkPhysicalDeviceProperties& GetProperties() const { m_properties; }
	[[nodiscard]] const VkPhysicalDeviceLimits& GetLimits() const { m_properties.limits; }
	[[nodiscard]] const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const { return m_memoryProperties; }

	[[nodiscard]] std::string GetDeviceName() const { return m_properties.deviceName; }
	[[nodiscard]] PhysicalDeviceType GetDeviceType() const;

	[[nodiscard]] const std::vector<std::string>& GetDeviceExtensions() const;
	[[nodiscard]] const std::vector<VkQueueFamilyProperties>& GetQueueFamilyProperties() const;

	[[nodiscard]] bool IsPresentSupported(SurfacePtr surface, uint32_t queueFamilyIndex);

	[[nodiscard]] bool CheckExtensionSupported(const std::string& requestedExtension);
	[[nodiscard]] bool CheckExtensionSupported(const std::vector<std::string>& requestedExtensions);

	[[nodiscard]] const VkFormatProperties GetFormatProperties(VkFormat format) const;

	[[nodiscatd]] uint32_t GetQueueFamilyPerformanceQueryPasses(const VkQueryPoolPerformanceCreateInfoKHR* perfQueryCreateInfo) const;
	void EnumerateQueueFamilyPerformanceQueryCounters(uint32_t queueFamilyIndex, uint32_t* count, VkPerformanceCounterKHR* counters, VkPerformanceCounterDescriptionKHR* descriptions) const;

private:
	InstancePtr                          m_instance{ nullptr };
	VkPhysicalDevice                     m_device{ VK_NULL_HANDLE };
	VkPhysicalDeviceFeatures             m_features{};
	VkPhysicalDeviceProperties           m_properties{};
	VkPhysicalDeviceMemoryProperties     m_memoryProperties{};
	std::vector<VkQueueFamilyProperties> m_queueFamilies;
	std::vector<std::string>             m_availableExtensions;

	// OLD
	Suitable                             m_suitable = Suitable::yes;
	std::vector<std::string>             m_extensionsToEnable;
	GenericFeatureChain                  m_extendedFeaturesChain;
	bool                                 m_deferSurfaceInitialization = false;

	std::string                          m_name;
	bool                                 m_properties2ExtEnabled = false;
};

#pragma endregion

} // namespace vkw