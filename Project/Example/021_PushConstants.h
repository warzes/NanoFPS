#pragma once

class Example_021 final : public EngineApplication
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

	std::vector<PerFrame>  mPerFrame;
	vkr::ShaderModulePtr        mVS;
	vkr::ShaderModulePtr        mPS;
	vkr::PipelineInterfacePtr   mPipelineInterface;
	vkr::GraphicsPipelinePtr    mPipeline;
	vkr::BufferPtr              mVertexBuffer;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
	vkr::DescriptorSetPtr       mDescriptorSet;
	vkr::ImagePtr               mImages[3];
	vkr::SamplerPtr             mSampler;
	vkr::SampledImageViewPtr    mSampledImageViews[3];
	vkr::VertexBinding          mVertexBinding;
};