#pragma once

class Example_029 final : public EngineApplication
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
		vkr::QueryPtr         timestampQuery;
	};

	std::vector<PerFrame> mPerFrame;
	vkr::ShaderModulePtr       mVS;
	vkr::ShaderModulePtr       mPS;
	vkr::PipelineInterfacePtr  mPipelineInterface;
	vkr::GraphicsPipelinePtr   mPipeline;
	vkr::BufferPtr             mVertexBuffer;
	vkr::Viewport                  mViewport;
	vkr::Rect                      mScissorRect;
	vkr::VertexBinding             mVertexBinding;
	uint2                           mRenderTargetSize;

	// Options
	uint32_t mNumTriangles = 1000;
	bool     mUseInstancedDraw = true;

	// Stats
	uint64_t                 mGpuWorkDuration = 0;
	vkr::PipelineStatistics mPipelineStatistics = {};
	struct PerFrameRegister
	{
		uint64_t frameNumber;
		float    gpuWorkDuration;
		float    cpuFrameTime;
	};
	std::deque<PerFrameRegister> mFrameRegisters;
};