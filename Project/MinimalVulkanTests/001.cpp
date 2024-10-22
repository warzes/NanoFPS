#include "stdafx.h"
#include "001.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_raii.hpp"

ApplicationCreateInfo Test001::GetConfig()
{
	return ApplicationCreateInfo();
}

bool Test001::Start()
{
	vkw::Context context;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	vkw::InstanceCreateInfo ici{};
	ici.enableValidationLayers = enableValidationLayers;
	ici.useDebugMessenger = enableValidationLayers;

	if (enableValidationLayers)
	{
		//ici.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT); // проверка шейдеров. но это очень медленно
		ici.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT); // лучшие практики использования вулкан
		ici.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);   // printf в шейдере?
		ici.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT); // проверка проблем синхронизации
	}

	vkw::InstancePtr instance = context.CreateInstance(ici);
	if (!instance) return false;

	vkw::SurfaceCreateInfo surfaceCreateInfo;
	surfaceCreateInfo.hinstance = GetWindowInstance();
	surfaceCreateInfo.hwnd = GetWindowHWND();
	vkw::SurfacePtr surface = instance->CreateSurface(surfaceCreateInfo);
	if (!surface) return false;
	
#if 0
	vkw::PhysicalDeviceSelector deviceSelect;
	deviceSelect.SetSurface(surface);
	auto selPhDevice = instance->GetDeviceSuitable(deviceSelect);
	if (!selPhDevice) return false;
#else
	auto physicDevices = instance->GetPhysicalDevices();
	vkw::PhysicalDevicePtr selectDevice{ nullptr };
	uint32_t graphicsQueueFamily = 0;
	for (auto gpu : physicDevices)
	{
		if (selectDevice) break;

		if (gpu->GetDeviceType() == vkw::PhysicalDeviceType::DiscreteGPU)
		{
			auto deviceFeatures = gpu->GetFeatures();
			if (deviceFeatures.geometryShader)
			{
				if (gpu->IsPresentSupported(surface))
				{
					auto hq = gpu->GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT);
					if (hq.has_value())
					{
						graphicsQueueFamily = hq.value();

						selectDevice = gpu;
						break;
					}
				}
			}
		}
	}
	if (!selectDevice) return false;
#endif

	vkw::DevicePtr device = instance->CreateDevice(selectDevice);
	if (!device) return false;



	return true;
}

void Test001::Shutdown()
{

}

void Test001::FixedUpdate(float deltaTime)
{
}

void Test001::Update(float deltaTime)
{
}

void Test001::Frame(float deltaTime)
{
}
