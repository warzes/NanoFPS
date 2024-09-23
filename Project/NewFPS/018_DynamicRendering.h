#pragma once

// VK_KHR_dynamic_rendering
// зачем это расширение - https://docs.vulkan.org/features/latest/features/proposals/VK_KHR_dynamic_rendering.html
// https://lesleylai.info/en/vk-khr-dynamic-rendering/

class Example_018 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame>         mPerFrame;
	std::vector<vkr::CommandBufferPtr> mPreRecordedCmds;
	vkr::PipelineInterfacePtr          mPipelineInterface;
	vkr::GraphicsPipelinePtr           mPipeline;
	vkr::MeshPtr                       mSphereMesh;
};