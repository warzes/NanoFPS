#pragma once

class GameApplication final : public EngineApplication
{
public:
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

	std::vector<PerFrame> mPerFrame;
	vkr::ShaderModulePtr       mVS;
	vkr::ShaderModulePtr       mPS;
	vkr::PipelineInterfacePtr  mPipelineInterface;
	vkr::GraphicsPipelinePtr   mPipeline;
	vkr::BufferPtr             mVertexBuffer;
	vkr::VertexBinding         mVertexBinding;
};