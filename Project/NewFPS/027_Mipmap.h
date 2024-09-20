#pragma once

class Example_027 final : public EngineApplication
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
	DescriptorSetPtr       mDescriptorSet[2];
	BufferPtr              mUniformBuffer[2];
	ImagePtr               mImage[2];
	SamplerPtr             mSampler;
	SampledImageViewPtr    mSampledImageView[2];
	VertexBinding               mVertexBinding;
	int                               mLevelRight;
	int                               mLevelLeft;
	int                               mMaxLevelRight;
	int                               mMaxLevelLeft;
	bool                              mLeftInGpu;
	bool                              mRightInGpu;
	int                               mFilterOption;
	std::vector<const char*>          mFilterNames = {
		"Bilinear",
		"Other",
	};
};