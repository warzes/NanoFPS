#pragma once

#include "NanoEngine.h"

class Example_005 final : public EngineApplication
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

	std::vector<PerFrame>      mPerFrame;
	ShaderModulePtr            mVS;
	ShaderModulePtr            mPS;
	PipelineInterfacePtr       mPipelineInterface;
	GraphicsPipelinePtr        mPipeline;
	BufferPtr                  mVertexBuffer;
	DescriptorPoolPtr          mDescriptorPool;
	DescriptorSetLayoutPtr     mDescriptorSetLayout;
	DescriptorSetPtr           mDescriptorSet[3];
	BufferPtr                  mUniformBuffer[3];
	ImagePtr                   mImage;
	SamplerPtr                 mSampler;
	SampledImageViewPtr        mSampledImageView;
	VertexBinding              mVertexBinding;
};