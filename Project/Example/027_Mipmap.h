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
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame>             mPerFrame;
	vkr::ShaderModulePtr        mVS;
	vkr::ShaderModulePtr        mPS;
	vkr::PipelineInterfacePtr   mPipelineInterface;
	vkr::GraphicsPipelinePtr    mPipeline;
	vkr::BufferPtr              mVertexBuffer;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
	vkr::DescriptorSetPtr       mDescriptorSet[2];
	vkr::BufferPtr              mUniformBuffer[2];
	vkr::ImagePtr               mImage[2];
	vkr::SamplerPtr             mSampler;
	vkr::SampledImageViewPtr    mSampledImageView[2];
	vkr::VertexBinding               mVertexBinding;
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