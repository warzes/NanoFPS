#pragma once

class Example_010 final : public EngineApplication
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

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;

private:
	void setupEntity(const TriMesh& mesh, const GeometryCreateInfo& createInfo, Entity* pEntity);

	struct PerFrame
	{
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
		QueryPtr         timestampQuery;
	};

	std::vector<PerFrame>  mPerFrame;
	ShaderModulePtr        mVS;
	ShaderModulePtr        mPS;
	PipelineInterfacePtr   mPipelineInterface;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDescriptorSetLayout;
	GraphicsPipelinePtr    mSkyBoxPipeline;
	Entity                 mSkyBox;
	GraphicsPipelinePtr    mReflectorPipeline;
	Entity                 mReflector;
	ImagePtr               mCubeMapImage;
	SampledImageViewPtr    mCubeMapImageView;
	SamplerPtr             mCubeMapSampler;
	int32_t                mPrevX = 0;
	int32_t                mPrevY = 0;
	float                  mRotY = 0;
	float                  mTargetRotY = 0;
	uint64_t               mGpuWorkDuration = 0;
};