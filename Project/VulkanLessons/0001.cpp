/*
0001 
- Init Volk
- Create Vulkan Instance
- Check Available Layers and Available Instance Extensions
*/


https://github.com/elecro/vkdemos
https://jhenriques.net/development.html
https://www.youtube.com/watch?v=0-DTfARZ5ow&list=PLFAIgTeqcARkeHm-RimFyKET6IZPxlBSt&index=3
https://vkguide.dev/docs/new_chapter_1/vulkan_init_code/

#if defined(_MSC_VER)
#	pragma warning(push, 3)
#endif

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

#include <volk/volk.h>
#include <vulkan/vk_enum_string_helper.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

namespace
{
	template<typename PROPS, typename FUNC, typename... ARGS>
	std::vector<PROPS> QueryWithMethod(const FUNC& queryWith, ARGS... args)
	{
		uint32_t count = 0;
		if (VK_SUCCESS != queryWith(args..., &count, nullptr))  return {};
		std::vector<PROPS> result(count);
		if (VK_SUCCESS != queryWith(args..., &count, result.data())) return {};
		return result;
	}

	std::vector<VkLayerProperties> QueryInstanceLayers()
	{
		return QueryWithMethod<VkLayerProperties>(vkEnumerateInstanceLayerProperties);
	}

	std::vector<VkExtensionProperties> QueryInstanceExtensions()
	{
		return QueryWithMethod<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
	}

	VkInstance CreateInstance()
	{
		std::vector<const char*> layers = {};
		std::vector<const char*> extensions = {};

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

		VkInstance instance{ VK_NULL_HANDLE };
		VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
		if (VK_SUCCESS != result)
		{
			printf("Failed to create instance: %d\n", result);
		}

		volkLoadInstance(instance);
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
		std::vector<VkPhysicalDevice> devices = QueryWithMethod<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

		std::vector<PhysicalDeviceInfo> infos(devices.size());
		for (size_t idx = 0; idx < infos.size(); idx++)
		{
			const VkPhysicalDevice phyDevice = devices[idx];

			infos[idx].phyDevice = phyDevice;
			infos[idx].extensions = QueryWithMethod<VkExtensionProperties>(vkEnumerateDeviceExtensionProperties, phyDevice, nullptr);

			vkGetPhysicalDeviceProperties(phyDevice, &infos[idx].properties);
			vkGetPhysicalDeviceMemoryProperties(phyDevice, &infos[idx].memory);
		}

		return infos;
	}

	[[deprecated]] void DumpExtensions(const std::string& header, const std::vector<VkExtensionProperties>& exts)
	{
		uint32_t maxWidth = std::accumulate(exts.begin(), exts.end(), 10u,
			[](uint32_t value, const VkExtensionProperties& extA) {
				return std::max(value, (uint32_t)strnlen(extA.extensionName, 256));
			});

		printf("%s (count = %ld)\n", header.c_str(), exts.size());
		for (const VkExtensionProperties& ext : exts) {
			printf("    %*s : version %u\n", maxWidth - 2, ext.extensionName, ext.specVersion);
		}
	}

	[[deprecated]] void DumpPhysicalDeviceInfos(const std::string& header, const std::vector<PhysicalDeviceInfo>& phyDevices)
	{
		printf("%s (count = %ld)\n", header.c_str(), phyDevices.size());
		for (size_t idx = 0; idx < phyDevices.size(); idx++) {
			const PhysicalDeviceInfo& info = phyDevices[idx];
			const VkPhysicalDeviceProperties& props = info.properties;

			printf("  %ld: deviceName = %s vendorID = 0x%x deviceID = 0x%x\n",
				idx, props.deviceName, props.vendorID, props.deviceID);
			printf("     deviceType = %s\n",
				string_VkPhysicalDeviceType(props.deviceType));
			printf("\n");

			//DumpExtensions("    Device Extensions", info.extensions);
			printf("\n");

			printf("    Memory Types (count = %u)\n", info.memory.memoryTypeCount);
			for (uint32_t ndx = 0; ndx < info.memory.memoryTypeCount; ndx++)
			{
				const VkMemoryType& memType = info.memory.memoryTypes[ndx];
				printf("     %u: heapIndex = %u propertyFlags = 0x%x\n", ndx, memType.heapIndex, memType.propertyFlags);

				for (uint32_t shift = 0; shift < 32; shift++)
				{
					VkMemoryPropertyFlagBits flag =
						static_cast<VkMemoryPropertyFlagBits>(memType.propertyFlags & (1U << shift));
					if (flag)
					{
						printf("         | %s\n", string_VkMemoryPropertyFlagBits(flag));
					}
				}
			}
			printf("\n");

			printf("    Memory Heaps (count = %u)\n", info.memory.memoryHeapCount);
			for (uint32_t ndx = 0; ndx < info.memory.memoryHeapCount; ndx++)
			{
				const VkMemoryHeap& heap = info.memory.memoryHeaps[ndx];
				const float         sizeInGiB = (float)heap.size / (1 << 30);

				printf("     %u: size = %lu (%2.2f GiB) flags = 0x%x\n", ndx, heap.size, sizeInGiB, heap.flags);

				for (uint32_t shift = 0; shift < 32; shift++)
				{
					VkMemoryHeapFlagBits flag = static_cast<VkMemoryHeapFlagBits>(heap.flags & (1U << shift));
					if (flag)
					{
						printf("         | %s\n", string_VkMemoryHeapFlagBits(flag));
					}
				}
			}
		}
	}
}

void StartLesson0001()
{
	if (volkInitialize() != VK_SUCCESS)
	{
		//error("Failed to initialize volk.");
		return;
	}

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

	VkInstance instance = CreateInstance();
	std::vector<PhysicalDeviceInfo> phyDevices = QueryPhysicalDevices(instance);

	// просто вывести список видеокарт
	//DumpPhysicalDeviceInfos("Physical Devices", phyDevices);

	vkDestroyInstance(instance, nullptr);
	volkFinalize();
}