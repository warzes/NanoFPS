#pragma once

struct VulkanPerFrameData
{
	bool Setup(vkr::RenderDevice& device);
	void Shutdown();
	uint32_t Frame(vkr::VulkanSwapChain& swapChain);
	vkr::SubmitInfo SetupSubmitInfo();

	vkr::CommandBufferPtr cmd;
	vkr::SemaphorePtr     imageAcquiredSemaphore;
	vkr::FencePtr         imageAcquiredFence;
	vkr::SemaphorePtr     renderCompleteSemaphore;
	vkr::FencePtr         renderCompleteFence;
};