#include "Base.h"
#include "Core.h"
#include "NewRenderer.h"
#include "EngineWindow.h"
#include "SoftRender.h"

namespace
{
	bool UseValidationLayers{ false };

	VkInstance Instance{ nullptr };
	VkDebugUtilsMessengerEXT DebugMessenger{ nullptr };
	VkPhysicalDevice ChosenGPU{ nullptr };
	VkDevice Device{ nullptr };
	VkSurfaceKHR Surface{ nullptr };

	VkQueue GraphicsQueue{ nullptr };
	uint32_t GraphicsQueueFamily{ 0 };
	VkQueue PresentQueue{ nullptr };
	uint32_t PresentQueueFamily{ 0 };

	VmaAllocator Allocator{ nullptr };

	vk::SwapchainKHR swapchain{ nullptr };
	std::vector<SwapChainFrame> swapchainFrames;
	vk::Format swapchainFormat;
	vk::Extent2D swapchainExtent;

	int maxFramesInFlight, frameNumber;

	//Color conversion function
	unsigned char* (*convert_color)(float, float, float);

	vk::CommandPool commandPool;
	vk::CommandBuffer mainCommandBuffer;
}

namespace NewRenderer
{
	bool initVulkan();
	bool initSwapChain();
	bool initCommands();

	bool createSwapChain(uint32_t width, uint32_t height);
	void destroySwapChain();
}

bool NewRenderer::initVulkan()
{
	VkResult result = volkInitialize();
	if (result != VK_SUCCESS)
	{
		Fatal("volkInitialize failed!");
		return false;
	}

	// current version vulkan
	{
		uint32_t version{ 0 };
		vkEnumerateInstanceVersion(&version);

		std::string text = "System can support vulkan Variant: ";
		text += std::to_string(VK_API_VERSION_MAJOR(version));
		text += ".";
		text += std::to_string(VK_API_VERSION_MINOR(version));
		text += ".";
		text += std::to_string(VK_API_VERSION_PATCH(version));
		Print(text);

		// TODO: делать проверку на версию
	}

	vkb::InstanceBuilder builder;

	auto inst_ret = builder
		.set_app_name("FPS Game")
		.set_engine_name("NanoEngineVK")
		//.set_app_version(1, 0, 0)
		//.set_engine_version(1, 0, 0)
		.require_api_version(1, 1, 0) // TODO: пока 1.1, позже 1.3
		.request_validation_layers(UseValidationLayers)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkbInst = inst_ret.value();

	Instance = vkbInst.instance;
	DebugMessenger = vkbInst.debug_messenger;

	volkLoadInstanceOnly(Instance);

	result = glfwCreateWindowSurface(Instance, Window::GetWindow(), nullptr, &Surface);
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to abstract glfw surface for Vulkan.");
		return false;
	}

	////vulkan 1.3 features
	//VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	//features.dynamicRendering = true;
	//features.synchronization2 = true;

	////vulkan 1.2 features
	//VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	//features12.bufferDeviceAddress = true;
	//features12.descriptorIndexing = true;

	//use vkbootstrap to select a gpu. 
	vkb::PhysicalDeviceSelector selector{ vkbInst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1) // TODO: пока 1.1, позже 1.3
		//.set_required_features_13(features)
		//.set_required_features_12(features12)
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.set_surface(Surface)
		.select()
		.value();

	//create the final vulkan device
	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	// Get the VkDevice handle used in the rest of a vulkan application
	Device = vkbDevice.device;
	ChosenGPU = physicalDevice.physical_device;

	volkLoadDevice(Device);

	GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	PresentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
	PresentQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::present).value();

	// initialize the memory allocator
	{
		VmaVulkanFunctions vmaVulkanFunc{};
		vmaVulkanFunc.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
		vmaVulkanFunc.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
		vmaVulkanFunc.vkAllocateMemory = vkAllocateMemory;
		vmaVulkanFunc.vkBindBufferMemory = vkBindBufferMemory;
		vmaVulkanFunc.vkBindImageMemory = vkBindImageMemory;
		vmaVulkanFunc.vkCreateBuffer = vkCreateBuffer;
		vmaVulkanFunc.vkCreateImage = vkCreateImage;
		vmaVulkanFunc.vkDestroyBuffer = vkDestroyBuffer;
		vmaVulkanFunc.vkDestroyImage = vkDestroyImage;
		vmaVulkanFunc.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
		vmaVulkanFunc.vkFreeMemory = vkFreeMemory;
		vmaVulkanFunc.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
		vmaVulkanFunc.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
		vmaVulkanFunc.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
		vmaVulkanFunc.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
		vmaVulkanFunc.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
		vmaVulkanFunc.vkMapMemory = vkMapMemory;
		vmaVulkanFunc.vkUnmapMemory = vkUnmapMemory;
		vmaVulkanFunc.vkCmdCopyBuffer = vkCmdCopyBuffer;

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = ChosenGPU;
		allocatorInfo.device = Device;
		allocatorInfo.instance = Instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		allocatorInfo.pVulkanFunctions = &vmaVulkanFunc;
		vmaCreateAllocator(&allocatorInfo, &Allocator);
	}	

	return true;
}

bool NewRenderer::initSwapChain()
{
	if (!createSwapChain(Window::GetWidth(), Window::GetHeight()))
		return false;

	return true;
}

bool NewRenderer::initCommands()
{
	commandPool = make_command_pool(Device, ChosenGPU, Surface);

	return true;
}

unsigned char* convert_to_r8g8b8a8_unorm(float r, float g, float b) {

	unsigned char color[4];

	r = std::max(std::min(r, 0.99f), 0.0f);
	g = std::max(std::min(g, 0.99f), 0.0f);
	b = std::max(std::min(b, 0.99f), 0.0f);

	color[0] = static_cast<unsigned char>(r * 0xFF);
	color[1] = static_cast<unsigned char>(g * 0xFF);
	color[2] = static_cast<unsigned char>(b * 0xFF);
	color[3] = static_cast<unsigned char>(0xFF);

	return color;
}

unsigned char* convert_to_b8g8r8a8_unorm(float r, float g, float b) {

	unsigned char color[4];

	r = std::max(std::min(r, 0.99f), 0.0f);
	g = std::max(std::min(g, 0.99f), 0.0f);
	b = std::max(std::min(b, 0.99f), 0.0f);

	color[0] = static_cast<unsigned char>(b * 0xFF);
	color[1] = static_cast<unsigned char>(g * 0xFF);
	color[2] = static_cast<unsigned char>(r * 0xFF);
	color[3] = static_cast<unsigned char>(0xFF);

	return color;
}

void chooseColorConversionFunction()
{
	if (swapchainFormat == vk::Format::eR8G8B8A8Unorm)
	{
		convert_color = &convert_to_r8g8b8a8_unorm;
	}

	else if (swapchainFormat == vk::Format::eB8G8R8A8Unorm)
	{
		convert_color = &convert_to_b8g8r8a8_unorm;
	}
}

bool NewRenderer::createSwapChain(uint32_t width, uint32_t height)
{
	SwapChainBundle bundle = create_swapchain(Device, ChosenGPU, Surface, width, height);

	swapchain = bundle.swapchain;
	swapchainFrames = bundle.frames;
	swapchainFormat = bundle.format;
	swapchainExtent = bundle.extent;
	maxFramesInFlight = static_cast<int>(swapchainFrames.size());
	chooseColorConversionFunction();

	for (SwapChainFrame& frame : swapchainFrames) 
	{
		frame.logicalDevice = Device;
		frame.physicalDevice = ChosenGPU;
		frame.width = swapchainExtent.width;
		frame.height = swapchainExtent.height;
	}

	return true;
}

void NewRenderer::destroySwapChain()
{
	for (SwapChainFrame& frame : swapchainFrames)
	{
		frame.Destroy();
	}
	//vkDestroySwapchainKHR(Device, swapchain);
}

void make_frame_resources()
{
	for (SwapChainFrame& frame : swapchainFrames)
	{

		frame.imageAvailable = make_semaphore(Device);
		frame.renderFinished = make_semaphore(Device);
		frame.inFlight = make_fence(Device);

		frame.Setup();
	}
}

bool NewRenderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;
	if (!initSwapChain()) return false;
	if (!initCommands()) return false;

	commandBufferInputChunk commandBufferInput = { Device, commandPool, swapchainFrames };
	mainCommandBuffer = make_command_buffer(commandBufferInput);
	make_frame_command_buffers(commandBufferInput);

	make_frame_resources();

	frameNumber = 0;

	return true;
}

void NewRenderer::Close()
{
	if (Allocator)
		vmaDestroyAllocator(Allocator);

	if (Device)
		vkDeviceWaitIdle(Device);

	destroySwapChain();

	if (Instance)
	{
		vkDestroySurfaceKHR(Instance, Surface, nullptr);
		vkDestroyDevice(Device, nullptr);

		vkb::destroy_debug_utils_messenger(Instance, DebugMessenger);
		vkDestroyInstance(Instance, nullptr);
	}

	Allocator = nullptr;
	Surface = nullptr;
	Device = nullptr;
	Instance = nullptr;
	volkFinalize();
}

void flush_frame(uint32_t imageIndex, uint32_t frameNumber)
{
	vk::CommandBuffer& commandBuffer = swapchainFrames[frameNumber].commandBuffer;

	vk::CommandBufferBeginInfo beginInfo = {};

	try {
		commandBuffer.begin(beginInfo);
	}
	catch (vk::SystemError err) {
		Fatal("Failed to begin recording command buffer!");
	}

	swapchainFrames[imageIndex].Flush();

	try 
	{
		commandBuffer.end();
	}
	catch (vk::SystemError err)
	{
		Fatal("failed to record command buffer!");
	}
}

void recreate_swapchain()
{
	width = 0;
	height = 0;
	while (width == 0 || height == 0)
	{
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	device.waitIdle();

	cleanup_swapchain();
	make_swapchain();
	make_frame_resources();
	vkInit::commandBufferInputChunk commandBufferInput = { device, commandPool, swapchainFrames };
	vkInit::make_frame_command_buffers(commandBufferInput);
}

void NewRenderer::Draw()
{
	vkWaitForFences(Device, 1, &swapchainFrames[frameNumber].inFlight, true, UINT64_MAX);
	vkResetFences(Device, 1, &swapchainFrames[frameNumber].inFlight);

	uint32_t imageIndex;
	//try {
		vk::ResultValue acquire = device.acquireNextImageKHR(
			swapchain, UINT64_MAX,
			swapchainFrames[frameNumber].imageAvailable, nullptr
		);
		imageIndex = acquire.value;
	}
	catch (vk::OutOfDateKHRError error) 
	{
		recreate_swapchain();
		return;
	}
	catch (vk::IncompatibleDisplayKHRError error)
	{
		recreate_swapchain();
		return;
	}
	catch (vk::SystemError error)
	{
		Fatal("Failed to acquire swapchain image!");
	}

	vk::CommandBuffer& commandBuffer = swapchainFrames[frameNumber].commandBuffer;

	commandBuffer.reset();

	flush_frame(imageIndex, frameNumber);

	vk::SubmitInfo submitInfo = {};

	vk::Semaphore waitSemaphores[] = { swapchainFrames[frameNumber].imageAvailable };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vk::Semaphore signalSemaphores[] = { swapchainFrames[frameNumber].renderFinished };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	try
	{
		graphicsQueue.submit(submitInfo, swapchainFrames[frameNumber].inFlight);
	}
	catch (vk::SystemError err) 
	{
		Fatal("failed to submit draw command buffer!");
	}

	vk::PresentInfoKHR presentInfo = {};
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	vk::SwapchainKHR swapChains[] = { swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	vk::Result present;

	try {
		present = presentQueue.presentKHR(presentInfo);
	}
	catch (vk::OutOfDateKHRError error) {
		present = vk::Result::eErrorOutOfDateKHR;
	}

	if (present == vk::Result::eErrorOutOfDateKHR || present == vk::Result::eSuboptimalKHR)
	{
		recreate_swapchain();
		return;
	}

	frameNumber = (frameNumber + 1) % maxFramesInFlight;
}