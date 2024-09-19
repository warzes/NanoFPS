#pragma once

class Example_023 final : public EngineApplication
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

	std::vector<PerFrame>             mPerFrame;
	ShaderModulePtr        mVS;
	ShaderModulePtr        mPS;
	PipelineInterfacePtr   mPipelineInterface;
	GraphicsPipelinePtr    mPipeline;
	BufferPtr              mVertexBuffer;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	DescriptorSetLayoutPtr mDescriptorSetLayoutBuffers;
	DescriptorSetPtr       mDescriptorSet;
	BufferPtr              mUniformBuffers[3];
	ImagePtr               mImages[3];
	SamplerPtr             mSampler;
	SampledImageViewPtr    mSampledImageViews[3];
	VertexBinding               mVertexBinding;
};