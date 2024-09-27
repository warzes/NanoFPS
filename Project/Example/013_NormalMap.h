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
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
	};

	struct Entity
	{
		float3           translate = float3(0, 0, 0);
		float3           rotate = float3(0, 0, 0);
		float3           scale = float3(1, 1, 1);
		vkr::MeshPtr          mesh;
		vkr::DescriptorSetPtr drawDescriptorSet;
		vkr::BufferPtr        drawUniformBuffer;
	};

	std::vector<PerFrame>  mPerFrame;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	vkr::DescriptorSetLayoutPtr mDrawObjectSetLayout;
	vkr::PipelineInterfacePtr   mDrawObjectPipelineInterface;
	vkr::GraphicsPipelinePtr    mDrawObjectPipeline;
	vkr::ImagePtr               mAlbedoTexture;
	vkr::ImagePtr               mNormalMap;
	vkr::SampledImageViewPtr    mAlbedoTextureView;
	vkr::SampledImageViewPtr    mNormalMapView;
	vkr::SamplerPtr             mSampler;
	Entity                 mCube;
	Entity                 mSphere;
	std::vector<Entity*>   mEntities;
	uint32_t               mEntityIndex = 0;
	PerspCamera            mCamera;

	std::vector<const char*> mEntityNames = {
		"Cube",
		"Sphere",
	};

	vkr::DescriptorSetLayoutPtr mLightSetLayout;
	vkr::PipelineInterfacePtr   mLightPipelineInterface;
	vkr::GraphicsPipelinePtr    mLightPipeline;
	Entity                 mLight;
	float3                 mLightPosition = float3(0, 5, 5);

	void setupEntity(
		const vkr::TriMesh& mesh,
		vkr::DescriptorPool* pDescriptorPool,
		const vkr::DescriptorSetLayout* pDrawSetLayout,
		const vkr::DescriptorSetLayout* pShadowSetLayout,
		Entity* pEntity);
};