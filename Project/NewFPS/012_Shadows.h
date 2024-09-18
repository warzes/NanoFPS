#pragma once

class Example_012 final : public EngineApplication
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

	struct Entity
	{
		float3           translate = float3(0, 0, 0);
		float3           rotate = float3(0, 0, 0);
		float3           scale = float3(1, 1, 1);
		MeshPtr          mesh;
		DescriptorSetPtr drawDescriptorSet;
		BufferPtr        drawUniformBuffer;
		DescriptorSetPtr shadowDescriptorSet;
		BufferPtr        shadowUniformBuffer;
	};

	void setupEntity(
		const TriMesh& mesh,
		DescriptorPool* pDescriptorPool,
		const DescriptorSetLayout* pDrawSetLayout,
		const DescriptorSetLayout* pShadowSetLayout,
		Entity* pEntity);

	std::vector<PerFrame>  mPerFrame;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDrawObjectSetLayout;
	PipelineInterfacePtr   mDrawObjectPipelineInterface;
	GraphicsPipelinePtr    mDrawObjectPipeline;
	Entity                 mGroundPlane;
	Entity                 mCube;
	Entity                 mKnob;
	std::vector<Entity*>   mEntities;
	PerspCamera            mCamera;

	DescriptorSetLayoutPtr mShadowSetLayout;
	PipelineInterfacePtr   mShadowPipelineInterface;
	GraphicsPipelinePtr    mShadowPipeline;
	RenderPassPtr          mShadowRenderPass;
	SampledImageViewPtr    mShadowImageView;
	SamplerPtr             mShadowSampler;

	DescriptorSetLayoutPtr mLightSetLayout;
	PipelineInterfacePtr   mLightPipelineInterface;
	GraphicsPipelinePtr    mLightPipeline;
	Entity                 mLight;
	float3                 mLightPosition = float3(0, 5, 5);
	PerspCamera            mLightCamera;
	bool                   mUsePCF = false;
};