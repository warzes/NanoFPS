#pragma once

#include "Application.h"
#include <VulkanWrapper.h>

class Test001 final : public Application
{
public:
	ApplicationCreateInfo GetConfig() final;
	bool Start() final;
	void Shutdown() final;
	void FixedUpdate(float deltaTime) final;
	void Update(float deltaTime) final;
	void Frame(float deltaTime) final;

private:
	vkw::Context context;
	vkw::InstancePtr instance;
	vkw::SurfacePtr surface;
	vkw::PhysicalDevicePtr selectDevice{ nullptr };
	uint32_t graphicsQueueFamily = 0;
	uint32_t presentQueueFamily = 0;
	vkw::DevicePtr device;
	vkw::SwapChainPtr swapChain;

	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
};