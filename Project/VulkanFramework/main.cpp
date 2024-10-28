#include "VulkanCore.h"
#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "WindowContext.h"

#include <array>

//https://github.com/elecro/vkdemos
//https://jhenriques.net/development.html
//https://www.youtube.com/watch?v=0-DTfARZ5ow&list=PLFAIgTeqcARkeHm-RimFyKET6IZPxlBSt&index=3
//https://vkguide.dev/docs/new_chapter_1/vulkan_init_code/

namespace
{
	WindowContext window;

	bool Setup()
	{
#ifdef NDEBUG
		const bool enableValidationLayers = false;
#else
		const bool enableValidationLayers = true;
#endif

		vkf::InstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.enableValidationLayers = enableValidationLayers;
		instanceCreateInfo.useDebugMessenger = enableValidationLayers;

		if (enableValidationLayers)
		{
			//instanceCreateInfo.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT); // проверка шейдеров. но это очень медленно
			instanceCreateInfo.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT); // лучшие практики использования вулкан
			instanceCreateInfo.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT);   // printf в шейдере?
			instanceCreateInfo.enabledValidationFeatures.push_back(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT); // проверка проблем синхронизации
		}

		vkf::Instance instance;
		if (!instance.Setup(instanceCreateInfo)) return 0;

		auto gpuList = instance.GetPhysicalDevices();
		if (gpuList.empty()) return 0;

		const std::vector<const char*> requestDeviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		std::vector<vkf::PhysicalDevice*> suitableGPU; // список подходящих устройств соблюдающих все обязательные требования

		vkf::PhysicalDevice* selectDevice{ nullptr };

		// подбираем GPU сначала по обязательным параметрам
		for (auto& gpu : gpuList)
		{
			auto availableExtensions = gpu.GetDeviceExtensions();
			if (!vkf::CheckDeviceExtensionSupported(requestDeviceExtensions, availableExtensions)) continue;
			//if (!gpu.IsSupportSwapChain(surface)) continue;

			auto deviceFeatures = gpu.GetFeatures();
			if (!deviceFeatures.geometryShader) continue; // TODO: сделать функцию для проверки по списку, а не каждое по отдельности

			//auto retQueue = gpu.FindGraphicsAndPresentQueueFamilyIndex(surface);
			//if (!retQueue) continue;

			suitableGPU.push_back(&gpu);
		}
		if (suitableGPU.empty())
		{
			vkf::Fatal("Support Vulkan GPU not find");
			return 0;
		}

		// теперь берем оптимальное
		for (auto gpu : suitableGPU)
		{
			if (gpu->GetDeviceType() != vkf::PhysicalDeviceType::DiscreteGPU) continue;

			selectDevice = gpu;
			break;
		}

		if (!selectDevice) selectDevice = suitableGPU[0]; // нет дискретной видеокарты



		if (!selectDevice) return false;

		puts((selectDevice->GetDeviceName()).c_str());


		return true;
	}

	void Shutdown()
	{

	}

	void FixedUpdate(float deltaTime)
	{

	}

	void Update(float deltaTime)
	{

	}
}

int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	constexpr const double microsecondsToSeconds = 1.0 / 1000000.0;
	constexpr const double fixedFramerate = 60.0;
	constexpr const double fixedDeltaTimeMicroseconds = (1.0 / fixedFramerate) / microsecondsToSeconds;
	constexpr const double fixedDeltaTimeSeconds = fixedDeltaTimeMicroseconds * microsecondsToSeconds;

	Timer timer{};
	double accumulator{};
	uint64_t frameTimeCurrent = timer.Microseconds();
	uint64_t frameTimeLast = frameTimeCurrent;
	std::array<double, 64> framerateArray = { {0.0} };
	uint64_t framerateTick{ 0 };

	if (window.Setup({}))
	{
		if (Setup())
		{
			while (!window.IsRequestedExit())
			{
				frameTimeCurrent = timer.Microseconds();
				const uint64_t frameTimeDelta = frameTimeCurrent - frameTimeLast;
				const double variableDeltaTimeSeconds = static_cast<double>(frameTimeDelta) * microsecondsToSeconds;
				frameTimeLast = frameTimeCurrent;

				if (frameTimeDelta)
					framerateArray[framerateTick] = 1000000.0 / static_cast<double>(frameTimeDelta);
				framerateTick = (framerateTick + 1) % framerateArray.size();

				accumulator += static_cast<double>(frameTimeDelta);
				while (accumulator >= fixedDeltaTimeMicroseconds)
				{
					FixedUpdate((float)fixedDeltaTimeSeconds);
					accumulator -= fixedDeltaTimeMicroseconds;
				}
				Update((float)variableDeltaTimeSeconds);

				window.PollEvent();
			}
		}
		Shutdown();
	}
	window.Shutdown();
}