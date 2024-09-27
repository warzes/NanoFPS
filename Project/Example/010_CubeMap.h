#pragma once

class Example_010 final : public EngineApplication
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

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;

private:
	void setupEntity(const vkr::TriMesh& mesh, const vkr::GeometryCreateInfo& createInfo, Entity* pEntity);

	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
		vkr::QueryPtr         timestampQuery;
	};

	std::vector<PerFrame>  mPerFrame;
	vkr::ShaderModulePtr        mVS;
	vkr::ShaderModulePtr        mPS;
	vkr::PipelineInterfacePtr   mPipelineInterface;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDescriptorSetLayout;
	vkr::GraphicsPipelinePtr    mSkyBoxPipeline;
	Entity                 mSkyBox;
	vkr::GraphicsPipelinePtr    mReflectorPipeline;
	Entity                 mReflector;
	vkr::ImagePtr               mCubeMapImage;
	vkr::SampledImageViewPtr    mCubeMapImageView;
	vkr::SamplerPtr             mCubeMapSampler;
	int32_t                mPrevX = 0;
	int32_t                mPrevY = 0;
	float                  mRotY = 0;
	float                  mTargetRotY = 0;
	uint64_t               mGpuWorkDuration = 0;
};