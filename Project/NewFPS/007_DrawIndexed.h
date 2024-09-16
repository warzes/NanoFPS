#pragma once

#include "NanoEngine.h"

class Example_007 final : public EngineApplication
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
	BufferPtr              mIndexBuffer;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	DescriptorSetPtr       mDescriptorSet;
	BufferPtr              mUniformBuffer;
	VertexBinding          mVertexBinding;
};