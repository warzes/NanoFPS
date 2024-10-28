#pragma once

#include "VulkanCore.h"

namespace vkf {

struct InstanceCreateInfo final
{
	std::string_view                           applicationName{ "Game" };
	uint32_t                                   applicationVersion{ VK_MAKE_VERSION(0, 0, 1) };
	std::string_view                           engineName{ "Nano VK Engine" };
	uint32_t                                   engineVersion{ VK_MAKE_VERSION(0, 0, 1) };

	std::vector<const char*>                   layers{};
	std::vector<const char*>                   extensions{};
	VkInstanceCreateFlags                      flags = static_cast<VkInstanceCreateFlags>(0);
	std::vector<VkBaseOutStructure*>           nextElements;

	// validation features
	std::vector<VkValidationCheckEXT>          disabledValidationChecks;
	std::vector<VkValidationFeatureEnableEXT>  enabledValidationFeatures;
	std::vector<VkValidationFeatureDisableEXT> disabledValidationFeatures;

	std::vector<VkLayerSettingEXT>             requiredLayerSettings;

	bool                                       enableValidationLayers{ false };

	bool                                       useDebugMessenger{ false };
	PFN_vkDebugUtilsMessengerCallbackEXT       debugCallback{ DefaultDebugCallback };
	VkDebugUtilsMessageSeverityFlagsEXT        debugMessageSeverity{ DefaultDebugMessageSeverity };
	VkDebugUtilsMessageTypeFlagsEXT            debugMessageType{ DefaultDebugMessageType };
	void*                                      debugUserDataPointer{ nullptr };
};

class Instance final
{
public:
	~Instance();

	bool Setup(const InstanceCreateInfo& createInfo);
	void Shutdown();

	operator VkInstance() const { return m_instance; }

	std::vector<PhysicalDevice> GetPhysicalDevices();

private:
	VkInstance               m_instance{ VK_NULL_HANDLE };
	VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
	bool                     m_initVolk{ false };
};

} // namespace vkf