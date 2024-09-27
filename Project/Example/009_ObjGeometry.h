#pragma once

class Example_009 final : public EngineApplication
{
	struct Entity
	{
		vkr::MeshPtr          mesh;
		vkr::DescriptorSetPtr descriptorSet;
		vkr::BufferPtr        uniformBuffer;
	};
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	void setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity);

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
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
	vkr::GraphicsPipelinePtr    mInterleavedPipeline;
	Entity                 mInterleavedU32;
	Entity                 mInterleaved;
	vkr::GraphicsPipelinePtr    mPlanarPipeline;
	Entity                 mPlanarU32;
	Entity                 mPlanar;
};