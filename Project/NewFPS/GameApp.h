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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame> mPerFrame;
	ShaderModulePtr       mVS;
	ShaderModulePtr       mPS;
	PipelineInterfacePtr  mPipelineInterface;
	GraphicsPipelinePtr   mPipeline;
	BufferPtr             mVertexBuffer;
	VertexBinding         mVertexBinding;
};