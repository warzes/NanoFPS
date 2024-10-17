#pragma once

class Example_028 final : public EngineApplication
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
	void setupEntity(const vkr::WireMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity);

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
	vkr::GraphicsPipelinePtr    mTrianglePipeline;
	Entity                       mCube;
	Entity                       mSphere;
	Entity                       mPlane;
	vkr::GraphicsPipelinePtr    mWirePipeline;
	Entity                       mWireCube;
	Entity                       mWireSphere;
	Entity                       mWirePlane;
};