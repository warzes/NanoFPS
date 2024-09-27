#pragma once

class Example_014 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;
	void Scroll(float dx, float dy) final;
	void KeyDown(KeyCode key) final;

private:
	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	struct Entity
	{
		vkr::MeshPtr          mesh;
		vkr::DescriptorSetPtr descriptorSet;
		vkr::BufferPtr        uniformBuffer;
	};

	std::vector<PerFrame>  mPerFrame;
	vkr::ShaderModulePtr        mVS;
	vkr::ShaderModulePtr        mPS;
	vkr::PipelineInterfacePtr   mPipelineInterface;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
	vkr::GraphicsPipelinePtr    mTrianglePipeline;
	Entity                 mCube;
	vkr::GraphicsPipelinePtr    mWirePipeline;
	Entity                 mWirePlane;

	ArcballCamera          mArcballCamera;

	void setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity);
	void setupEntity(const vkr::WireMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity);
};