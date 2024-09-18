#pragma once

class Example_015 final : public EngineApplication
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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
	};

	struct Entity
	{
		MeshPtr          mesh;
		DescriptorSetPtr descriptorSet;
		BufferPtr        uniformBuffer;
	};

	std::vector<PerFrame>  mPerFrame;
	ShaderModulePtr        mVS;
	ShaderModulePtr        mPS;
	PipelineInterfacePtr   mPipelineInterface;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	GraphicsPipelinePtr    mTrianglePipeline;
	Entity                 mCube;
	GraphicsPipelinePtr    mWirePipeline;
	Entity                 mWirePlane;

	ArcballCamera          mArcballCamera;

	void setupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
	void setupEntity(const WireMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);
};