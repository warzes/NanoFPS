#pragma once

class Example_022 final : public EngineApplication
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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame>  mPerFrame;
	ShaderModulePtr        mVS;
	ShaderModulePtr        mPS;
	PipelineInterfacePtr   mPipelineInterface;
	GraphicsPipelinePtr    mPipeline;
	BufferPtr              mVertexBuffer;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	BufferPtr              mUniformBuffer;
	ImagePtr               mImages[3];
	SamplerPtr             mSampler;
	SampledImageViewPtr    mSampledImageViews[3];
	VertexBinding          mVertexBinding;
};