#pragma once

class Example_013 final : public EngineApplication
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
	};

	std::vector<PerFrame>  mPerFrame;
	DescriptorPoolPtr      mDescriptorPool;
	DescriptorSetLayoutPtr mDrawObjectSetLayout;
	PipelineInterfacePtr   mDrawObjectPipelineInterface;
	GraphicsPipelinePtr    mDrawObjectPipeline;
	ImagePtr               mAlbedoTexture;
	ImagePtr               mNormalMap;
	SampledImageViewPtr    mAlbedoTextureView;
	SampledImageViewPtr    mNormalMapView;
	SamplerPtr             mSampler;
	Entity                 mCube;
	Entity                 mSphere;
	std::vector<Entity*>   mEntities;
	uint32_t               mEntityIndex = 0;
	PerspCamera            mCamera;

	std::vector<const char*> mEntityNames = {
		"Cube",
		"Sphere",
	};

	DescriptorSetLayoutPtr mLightSetLayout;
	PipelineInterfacePtr   mLightPipelineInterface;
	GraphicsPipelinePtr    mLightPipeline;
	Entity                 mLight;
	float3                 mLightPosition = float3(0, 5, 5);

	void setupEntity(
		const TriMesh& mesh,
		DescriptorPool* pDescriptorPool,
		const DescriptorSetLayout* pDrawSetLayout,
		const DescriptorSetLayout* pShadowSetLayout,
		Entity* pEntity);
};