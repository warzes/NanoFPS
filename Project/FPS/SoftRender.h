#pragma once

struct Buffer 
{
	vk::Buffer buffer;
	vk::DeviceMemory bufferMemory;
};

struct BufferInputChunk
{
	size_t size;
	vk::BufferUsageFlags usage;
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;
	vk::MemoryPropertyFlags memoryProperties;
};

uint32_t findMemoryTypeIndex(vk::PhysicalDevice physicalDevice, uint32_t supportedMemoryIndices, vk::MemoryPropertyFlags requestedProperties)
{
	vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		//bit i of supportedMemoryIndices is set if that memory type is supported by the device
		bool supported{ static_cast<bool>(supportedMemoryIndices & (1 << i)) };

		//propertyFlags holds all the memory properties supported by this memory type
		bool sufficient{ (memoryProperties.memoryTypes[i].propertyFlags & requestedProperties) == requestedProperties };

		if (supported && sufficient)
		{
			return i;
		}
	}

	return 0;
}

void allocateBufferMemory(Buffer& buffer, const BufferInputChunk& input) 
{
	vk::MemoryRequirements memoryRequirements = input.logicalDevice.getBufferMemoryRequirements(buffer.buffer);

	vk::MemoryAllocateInfo allocInfo;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryTypeIndex(
		input.physicalDevice, memoryRequirements.memoryTypeBits,
		input.memoryProperties
	);

	buffer.bufferMemory = input.logicalDevice.allocateMemory(allocInfo);
	input.logicalDevice.bindBufferMemory(buffer.buffer, buffer.bufferMemory, 0);
}

Buffer createBuffer(BufferInputChunk input) 
{
	vk::BufferCreateInfo bufferInfo;
	bufferInfo.flags = vk::BufferCreateFlags();
	bufferInfo.size = input.size;
	bufferInfo.usage = input.usage;
	bufferInfo.sharingMode = vk::SharingMode::eExclusive;

	Buffer buffer;
	buffer.buffer = input.logicalDevice.createBuffer(bufferInfo);

	allocateBufferMemory(buffer, input);
	return buffer;
}

class SwapChainFrame 
{
public:
	void Setup();
	void Flush();
	void Destroy();

	//For doing work
	vk::Device logicalDevice;
	vk::PhysicalDevice physicalDevice;

	//Swapchain-type stuff
	vk::Image image;
	vk::ImageView imageView;
	int width, height;

	vk::CommandBuffer commandBuffer;

	//Sync objects
	vk::Semaphore imageAvailable, renderFinished;
	vk::Fence inFlight;

	//Resources
	std::vector<unsigned char> colorBufferData;

	//Staging Buffer
	Buffer stagingBuffer;
	void* writeLocation;

	//Transition Jobs
	vk::ImageSubresourceRange colorBufferAccess;
	vk::ImageMemoryBarrier barrier, barrier2;

	//Copy Job
	vk::BufferImageCopy copy;
	vk::ImageSubresourceLayers copyAccess;
};

void SwapChainFrame::Setup() 
{
	colorBufferData.reserve(4 * width * height);

	for (int i = 0; i < width * height; ++i) {
		colorBufferData.push_back(0xff);
		colorBufferData.push_back(0x00);
		colorBufferData.push_back(0x00);
		colorBufferData.push_back(0x00);
	}

	BufferInputChunk input;
	input.logicalDevice = logicalDevice;
	input.physicalDevice = physicalDevice;
	input.memoryProperties = vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible;
	input.usage = vk::BufferUsageFlagBits::eTransferSrc;
	input.size = width * height * 4;

	stagingBuffer = createBuffer(input);

	writeLocation = logicalDevice.mapMemory(stagingBuffer.bufferMemory, 0, input.size);

	colorBufferAccess.aspectMask = vk::ImageAspectFlagBits::eColor;
	colorBufferAccess.baseMipLevel = 0;
	colorBufferAccess.levelCount = 1;
	colorBufferAccess.baseArrayLayer = 0;
	colorBufferAccess.layerCount = 1;

	barrier.oldLayout = vk::ImageLayout::eUndefined;
	barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange = colorBufferAccess;
	barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
	barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

	copy.bufferOffset = 0;
	copy.bufferRowLength = 0;
	copy.bufferImageHeight = 0;

	copyAccess.aspectMask = vk::ImageAspectFlagBits::eColor;
	copyAccess.mipLevel = 0;
	copyAccess.baseArrayLayer = 0;
	copyAccess.layerCount = 1;
	copy.imageSubresource = copyAccess;

	copy.imageOffset = vk::Offset3D(0, 0, 0);
	copy.imageExtent = vk::Extent3D(width, height, 1);

	barrier2.oldLayout = vk::ImageLayout::eTransferDstOptimal;
	barrier2.newLayout = vk::ImageLayout::ePresentSrcKHR;
	barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier2.image = image;
	barrier2.subresourceRange = colorBufferAccess;
	barrier2.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
	barrier2.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
}

void SwapChainFrame::Flush()
{
	memcpy(writeLocation, colorBufferData.data(), width * height * 4);

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		vk::DependencyFlags(), nullptr, nullptr, barrier);

	commandBuffer.copyBufferToImage(
		stagingBuffer.buffer, image, vk::ImageLayout::eTransferDstOptimal, copy
	);

	commandBuffer.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eBottomOfPipe,
		vk::DependencyFlags(), nullptr, nullptr, barrier2);
}

void SwapChainFrame::Destroy()
{
	logicalDevice.unmapMemory(stagingBuffer.bufferMemory);
	logicalDevice.freeMemory(stagingBuffer.bufferMemory);
	logicalDevice.destroyBuffer(stagingBuffer.buffer);

	logicalDevice.destroyImageView(imageView);
	logicalDevice.destroyFence(inFlight);
	logicalDevice.destroySemaphore(imageAvailable);
	logicalDevice.destroySemaphore(renderFinished);
}

struct SwapChainBundle
{
	vk::SwapchainKHR swapchain;
	std::vector<SwapChainFrame> frames;
	vk::Format format;
	vk::Extent2D extent;
};

struct SwapChainSupportDetails 
{
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

SwapChainSupportDetails query_swapchain_support(vk::PhysicalDevice device, vk::SurfaceKHR surface) 
{
	SwapChainSupportDetails support;
	support.capabilities = device.getSurfaceCapabilitiesKHR(surface);	
	support.formats = device.getSurfaceFormatsKHR(surface);
	support.presentModes = device.getSurfacePresentModesKHR(surface);
	return support;
}

vk::SurfaceFormatKHR choose_swapchain_surface_format(std::vector<vk::SurfaceFormatKHR> formats) 
{
	for (vk::SurfaceFormatKHR format : formats) 
	{
		if (format.format == vk::Format::eB8G8R8A8Unorm && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) 
		{
			return format;
		}
	}

	return formats[0];
}

vk::PresentModeKHR choose_swapchain_present_mode(std::vector<vk::PresentModeKHR> presentModes) 
{
	for (vk::PresentModeKHR presentMode : presentModes) 
	{
		if (presentMode == vk::PresentModeKHR::eMailbox)
		{
			return presentMode;
		}
	}

	return vk::PresentModeKHR::eFifo;
}

vk::Extent2D choose_swapchain_extent(uint32_t width, uint32_t height, vk::SurfaceCapabilitiesKHR capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX)
	{
		return capabilities.currentExtent;
	}
	else
	{
		vk::Extent2D extent = { width, height };

		extent.width = std::min(
			capabilities.maxImageExtent.width,
			std::max(capabilities.minImageExtent.width, extent.width)
		);

		extent.height = std::min(
			capabilities.maxImageExtent.height,
			std::max(capabilities.minImageExtent.height, extent.height)
		);

		return extent;
	}
}

struct QueueFamilyIndices 
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() 
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) 
{
	QueueFamilyIndices indices;

	std::vector<vk::QueueFamilyProperties> queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (vk::QueueFamilyProperties queueFamily : queueFamilies) 
	{
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
		{
			indices.graphicsFamily = i;
		}

		if (device.getSurfaceSupportKHR(i, surface))
		{
			indices.presentFamily = i;
		}

		if (indices.isComplete())
		{
			break;
		}

		i++;
	}

	return indices;
}

namespace vkImage {

	/**
		Create a view of a vulkan image.
	*/
	vk::ImageView make_image_view(
		vk::Device logicalDevice, vk::Image image, vk::Format format,
		vk::ImageAspectFlags aspect);
}

vk::ImageView vkImage::make_image_view(
	vk::Device logicalDevice, vk::Image image, vk::Format format,
	vk::ImageAspectFlags aspect)
{
	vk::ImageViewCreateInfo createInfo = {};
	createInfo.image = image;
	createInfo.viewType = vk::ImageViewType::e2D;
	createInfo.format = format;
	createInfo.components.r = vk::ComponentSwizzle::eIdentity;
	createInfo.components.g = vk::ComponentSwizzle::eIdentity;
	createInfo.components.b = vk::ComponentSwizzle::eIdentity;
	createInfo.components.a = vk::ComponentSwizzle::eIdentity;
	createInfo.subresourceRange.aspectMask = aspect;
	createInfo.subresourceRange.baseMipLevel = 0;
	createInfo.subresourceRange.levelCount = 1;
	createInfo.subresourceRange.baseArrayLayer = 0;
	createInfo.subresourceRange.layerCount = 1;

	return logicalDevice.createImageView(createInfo);
}

SwapChainBundle create_swapchain(vk::Device logicalDevice, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface, int width, int height) 
{
	SwapChainSupportDetails support = query_swapchain_support(physicalDevice, surface);

	vk::SurfaceFormatKHR format = choose_swapchain_surface_format(support.formats);

	vk::PresentModeKHR presentMode = choose_swapchain_present_mode(support.presentModes);

	vk::Extent2D extent = choose_swapchain_extent(width, height, support.capabilities);

	uint32_t imageCount = std::min(
		support.capabilities.maxImageCount,
		support.capabilities.minImageCount + 1
	);

	vk::SwapchainCreateInfoKHR createInfo = vk::SwapchainCreateInfoKHR(
		vk::SwapchainCreateFlagsKHR(), surface, imageCount, format.format, format.colorSpace,
		extent, 1, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst
	);

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
	}

	createInfo.preTransform = support.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = vk::SwapchainKHR(nullptr);

	SwapChainBundle bundle{};
	try {
		bundle.swapchain = logicalDevice.createSwapchainKHR(createInfo);
	}
	catch (vk::SystemError err) {
		throw std::runtime_error("failed to create swap chain!");
	}

	std::vector<vk::Image> images = logicalDevice.getSwapchainImagesKHR(bundle.swapchain);
	bundle.frames.resize(images.size());

	for (size_t i = 0; i < images.size(); ++i) {

		bundle.frames[i].image = images[i];
		bundle.frames[i].imageView = vkImage::make_image_view(
			logicalDevice, images[i], format.format, vk::ImageAspectFlagBits::eColor
		);
	}

	bundle.format = format.format;
	bundle.extent = extent;

	return bundle;
}

vk::CommandPool make_command_pool(vk::Device device, vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface) 
{
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

	vk::CommandPoolCreateInfo poolInfo;
	poolInfo.flags = vk::CommandPoolCreateFlags() | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	try 
	{
		return device.createCommandPool(poolInfo);
	}
	catch (vk::SystemError err)
	{
		Fatal("Failed to create Command Pool");
		return nullptr;
	}
}

struct commandBufferInputChunk 
{
	vk::Device device;
	vk::CommandPool commandPool;
	std::vector<SwapChainFrame>& frames;
};

vk::CommandBuffer make_command_buffer(commandBufferInputChunk inputChunk)
{
	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.commandPool = inputChunk.commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;


	//Make a "main" command buffer for the engine
	try
	{
		vk::CommandBuffer commandBuffer = inputChunk.device.allocateCommandBuffers(allocInfo)[0];
		return commandBuffer;
	}
	catch (vk::SystemError err)
	{

		Fatal("Failed to allocate main command buffer ");

		return nullptr;
	}
}

void make_frame_command_buffers(commandBufferInputChunk inputChunk) 
{
	std::stringstream message;

	vk::CommandBufferAllocateInfo allocInfo = {};
	allocInfo.commandPool = inputChunk.commandPool;
	allocInfo.level = vk::CommandBufferLevel::ePrimary;
	allocInfo.commandBufferCount = 1;

	//Make a command buffer for each frame
	for (int i = 0; i < inputChunk.frames.size(); ++i) {
		try {
			inputChunk.frames[i].commandBuffer = inputChunk.device.allocateCommandBuffers(allocInfo)[0];

			message << "Allocated command buffer for frame " << i;
			Print(message.str());
			message.str("");
		}
		catch (vk::SystemError err) {

			message << "Failed to allocate command buffer for frame " << i;
			Print(message.str());
			message.str("");
		}
	}
}

vk::Semaphore make_semaphore(vk::Device device)
{
	vk::SemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.flags = vk::SemaphoreCreateFlags();

	try {
		return device.createSemaphore(semaphoreInfo);
	}
	catch (vk::SystemError err) {
		Fatal("Failed to create semaphore ");
		return nullptr;
	}
}

vk::Fence make_fence(vk::Device device)
{
	vk::FenceCreateInfo fenceInfo = {};
	fenceInfo.flags = vk::FenceCreateFlags() | vk::FenceCreateFlagBits::eSignaled;

	try {
		return device.createFence(fenceInfo);
	}
	catch (vk::SystemError err) {
		Fatal("Failed to create fence ");
		return nullptr;
	}
}