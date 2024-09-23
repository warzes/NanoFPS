#pragma once

class Example_006 final : public EngineApplication
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

	std::vector<PerFrame>      mPerFrame;
	vkr::ShaderModulePtr            mCS;
	vkr::ShaderModulePtr            mVS;
	vkr::ShaderModulePtr            mPS;
	vkr::PipelineInterfacePtr       mComputePipelineInterface;
	vkr::ComputePipelinePtr         mComputePipeline;
	vkr::PipelineInterfacePtr       mGraphicsPipelineInterface;
	vkr::GraphicsPipelinePtr        mGraphicsPipeline;
	vkr::BufferPtr                  mVertexBuffer;
	vkr::DescriptorPoolPtr          mDescriptorPool;
	vkr::DescriptorSetLayoutPtr     mComputeDescriptorSetLayout;
	vkr::DescriptorSetPtr           mComputeDescriptorSet;
	vkr::DescriptorSetLayoutPtr     mGraphicsDescriptorSetLayout;
	vkr::DescriptorSetPtr           mGraphicsDescriptorSet;
	vkr::BufferPtr                  mUniformBuffer;
	vkr::ImagePtr                   mImage;
	vkr::SamplerPtr                 mSampler;
	vkr::SampledImageViewPtr        mSampledImageView;
	vkr::StorageImageViewPtr        mStorageImageView;
	vkr::VertexBinding              mVertexBinding;
};