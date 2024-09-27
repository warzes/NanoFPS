#include "stdafx.h"
#include "PerFrame.h"

bool VulkanPerFrameData::Setup(vkr::RenderDevice& device)
{
	CHECKED_CALL_AND_RETURN_FALSE(device.GetGraphicsQueue()->CreateCommandBuffer(&cmd));

	vkr::SemaphoreCreateInfo semaCreateInfo = {};
	CHECKED_CALL_AND_RETURN_FALSE(device.CreateSemaphore(semaCreateInfo, &imageAcquiredSemaphore));

	vkr::FenceCreateInfo fenceCreateInfo = {};
	CHECKED_CALL_AND_RETURN_FALSE(device.CreateFence(fenceCreateInfo, &imageAcquiredFence));

	CHECKED_CALL_AND_RETURN_FALSE(device.CreateSemaphore(semaCreateInfo, &renderCompleteSemaphore));

	fenceCreateInfo = { true }; // Create signaled
	CHECKED_CALL_AND_RETURN_FALSE(device.CreateFence(fenceCreateInfo, &renderCompleteFence));

	return true;
}

void VulkanPerFrameData::Shutdown()
{
	// TODO: очистка ресурсов объекта?
}

uint32_t VulkanPerFrameData::Frame(vkr::VulkanSwapChain& swapChain)
{
	// Wait for and reset render complete fence
	CHECKED_CALL(renderCompleteFence->WaitAndReset());

	uint32_t imageIndex = UINT32_MAX;
	CHECKED_CALL(swapChain.AcquireNextImage(UINT64_MAX, imageAcquiredSemaphore, imageAcquiredFence, &imageIndex));

	// Wait for and reset image acquired fence
	CHECKED_CALL(imageAcquiredFence->WaitAndReset());

	return imageIndex;
}

vkr::SubmitInfo VulkanPerFrameData::SetupSubmitInfo()
{
	vkr::SubmitInfo submitInfo = {};
	submitInfo.commandBufferCount = 1;
	submitInfo.ppCommandBuffers = &cmd;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.ppWaitSemaphores = &imageAcquiredSemaphore;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.ppSignalSemaphores = &renderCompleteSemaphore;
	submitInfo.pFence = renderCompleteFence;
	return submitInfo;
}
