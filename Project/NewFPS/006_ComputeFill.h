#pragma once

#include "NanoEngine.h"

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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	std::vector<PerFrame>      mPerFrame;
	ShaderModulePtr            mCS;
	ShaderModulePtr            mVS;
	ShaderModulePtr            mPS;
	PipelineInterfacePtr       mComputePipelineInterface;
	ComputePipelinePtr         mComputePipeline;
	PipelineInterfacePtr       mGraphicsPipelineInterface;
	GraphicsPipelinePtr        mGraphicsPipeline;
	BufferPtr                  mVertexBuffer;
	DescriptorPoolPtr          mDescriptorPool;
	DescriptorSetLayoutPtr     mComputeDescriptorSetLayout;
	DescriptorSetPtr           mComputeDescriptorSet;
	DescriptorSetLayoutPtr     mGraphicsDescriptorSetLayout;
	DescriptorSetPtr           mGraphicsDescriptorSet;
	BufferPtr                  mUniformBuffer;
	ImagePtr                   mImage;
	SamplerPtr                 mSampler;
	SampledImageViewPtr        mSampledImageView;
	StorageImageViewPtr        mStorageImageView;
	VertexBinding              mVertexBinding;
};