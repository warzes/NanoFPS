#include "stdafx.h"
#include "001.h"

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
	

	vkw::PhysicalDevicePtr selectDevice{ nullptr };
	uint32_t graphicsQueueFamily = 0;
	uint32_t presentQueueFamily = 0;

	const std::vector<const char*> requestDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	auto physicDevices = instance->GetPhysicalDevices();
	for (auto gpu : physicDevices)
	{
		if (selectDevice) break;

		if (gpu->GetDeviceType() != vkw::PhysicalDeviceType::DiscreteGPU) continue;

		if (!gpu->CheckExtensionSupported(requestDeviceExtensions)) continue;

		auto deviceFeatures = gpu->GetFeatures();
		if (!deviceFeatures.geometryShader) continue;

		auto retQueue = gpu->FindGraphicsAndPresentQueueFamilyIndex(surface);
		if (!retQueue) continue;
		graphicsQueueFamily = retQueue.value().first;
		presentQueueFamily = retQueue.value().second;
		
		selectDevice = gpu;
		break;
	}
	if (!selectDevice) return false;

	vkw::DeviceCreateInfo dci{};
	dci.physicalDevice          = selectDevice;
	dci.graphicsQueueFamily     = graphicsQueueFamily;
	dci.presentQueueFamily      = presentQueueFamily;
	dci.requestDeviceExtensions = requestDeviceExtensions;
	vkw::DevicePtr device = instance->CreateDevice(dci);
	if (!device) return false;

	return true;
}

void Test001::Shutdown()
{
	//surface.reset();
	//device.reset();
	//instance.reset();
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
