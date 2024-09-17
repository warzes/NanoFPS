#pragma once

#include "NanoEngine.h"

class Example_009 final : public EngineApplication
{
	struct Entity
	{
		MeshPtr          mesh;
		DescriptorSetPtr descriptorSet;
		BufferPtr        uniformBuffer;
	};
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	void setupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);

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
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	GraphicsPipelinePtr    mInterleavedPipeline;
	Entity                 mInterleavedU32;
	Entity                 mInterleaved;
	GraphicsPipelinePtr    mPlanarPipeline;
	Entity                 mPlanarU32;
	Entity                 mPlanar;
};