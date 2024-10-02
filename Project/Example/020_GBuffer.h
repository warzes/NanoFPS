#pragma once

namespace e020
{
	extern const float3 F0_MetalTitanium;
	extern const float3 F0_MetalChromium;
	extern const float3 F0_MetalIron;
	extern const float3 F0_MetalNickel;
	extern const float3 F0_MetalPlatinum;
	extern const float3 F0_MetalCopper;
	extern const float3 F0_MetalPalladium;
	extern const float3 F0_MetalZinc;
	extern const float3 F0_MetalGold;
	extern const float3 F0_MetalAluminum;
	extern const float3 F0_MetalSilver;
	extern const float3 F0_DiletricWater;
	extern const float3 F0_DiletricPlastic;
	extern const float3 F0_DiletricGlass;
	extern const float3 F0_DiletricCrystal;
	extern const float3 F0_DiletricGem;
	extern const float3 F0_DiletricDiamond;

	HLSL_PACK_BEGIN();
	struct MaterialConstants
	{
		hlsl_float<4>   F0;
		hlsl_float3<12> albedo;
		hlsl_float<4>   roughness;
		hlsl_float<4>   metalness;
		hlsl_float<4>   iblStrength;
		hlsl_float<4>   envStrength;
		hlsl_uint<4>    albedoSelect;
		hlsl_uint<4>    roughnessSelect;
		hlsl_uint<4>    metalnessSelect;
		hlsl_uint<4>    normalSelect;
	};
	HLSL_PACK_END();

	struct MaterialCreateInfo
	{
		float                 F0;
		float3                albedo;
		float                 roughness;
		float                 metalness;
		float                 iblStrength;
		float                 envStrength;
		std::filesystem::path albedoTexturePath;
		std::filesystem::path roughnessTexturePath;
		std::filesystem::path metalnessTexturePath;
		std::filesystem::path normalTexturePath;
	};

	class Material
	{
	public:
		Material() {}
		virtual ~Material() {}

		Result Create(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool, const MaterialCreateInfo* pCreateInfo);
		void        Destroy();

		static Result CreateMaterials(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool);
		static void        DestroyMaterials();

		static Material* GetMaterialWood() { return &sWood; }
		static Material* GetMaterialTiles() { return &sTiles; }

		static vkr::DescriptorSetLayout* GetMaterialResourcesLayout() { return sMaterialResourcesLayout.Get(); }
		static vkr::DescriptorSetLayout* GetMaterialDataLayout() { return sMaterialDataLayout.Get(); }

		vkr::DescriptorSet* GetMaterialResourceSet() const { return mMaterialResourcesSet.Get(); }
		vkr::DescriptorSet* GetMaterialDataSet() const { return mMaterialDataSet.Get(); }

	private:
		vkr::BufferPtr        mMaterialConstants;
		vkr::TexturePtr       mAlbedoTexture;
		vkr::TexturePtr       mRoughnessTexture;
		vkr::TexturePtr       mMetalnessTexture;
		vkr::TexturePtr       mNormalMapTexture;
		vkr::DescriptorSetPtr mMaterialResourcesSet;
		vkr::DescriptorSetPtr mMaterialDataSet;

		static vkr::TexturePtr             s1x1BlackTexture;
		static vkr::TexturePtr             s1x1WhiteTexture;
		static vkr::SamplerPtr             sClampedSampler;
		static vkr::DescriptorSetLayoutPtr sMaterialResourcesLayout;
		static vkr::DescriptorSetLayoutPtr sMaterialDataLayout;

		static Material sWood;
		static Material sTiles;
	};

	struct EntityCreateInfo
	{
		vkr::Mesh* pMesh = nullptr;
		const Material* pMaterial = nullptr;
	};

	class Entity
	{
	public:
		Entity() {}
		~Entity() {}

		Result Create(vkr::RenderDevice& device, vkr::Queue* pQueue, vkr::DescriptorPool* pPool, const EntityCreateInfo* pCreateInfo);
		void        Destroy();

		static Result CreatePipelines(vkr::RenderDevice& device, vkr::DescriptorSetLayout* pSceneDataLayout, vkr::DrawPass* pDrawPass);
		static void        DestroyPipelines();

		Transform& GetTransform() { return mTransform; }
		const Transform& GetTransform() const { return mTransform; }

		void UpdateConstants(vkr::Queue* pQueue);
		void Draw(vkr::DescriptorSet* pSceneDataSet, vkr::CommandBuffer* pCmd);

	private:
		Transform              mTransform;
		vkr::MeshPtr          mMesh;
		const Material* mMaterial = nullptr;
		vkr::BufferPtr        mCpuModelConstants;
		vkr::BufferPtr        mGpuModelConstants;
		vkr::DescriptorSetPtr mModelDataSet;

		static vkr::DescriptorSetLayoutPtr sModelDataLayout;
		static vkr::VertexDescription      sVertexDescription;
		static vkr::PipelineInterfacePtr   sPipelineInterface;
		static vkr::GraphicsPipelinePtr    sPipeline;
	};

	// b#
#define SCENE_CONSTANTS_REGISTER    0
#define MATERIAL_CONSTANTS_REGISTER 1
#define MODEL_CONSTANTS_REGISTER    2
#define GBUFFER_CONSTANTS_REGISTER  3

// s#
#define CLAMPED_SAMPLER_REGISTER 4

// t#
#define LIGHT_DATA_REGISTER                  5
#define MATERIAL_ALBEDO_TEXTURE_REGISTER     6  // DeferredRender only
#define MATERIAL_ROUGHNESS_TEXTURE_REGISTER  7  // DeferredRender only
#define MATERIAL_METALNESS_TEXTURE_REGISTER  8  // DeferredRender only
#define MATERIAL_NORMAL_MAP_TEXTURE_REGISTER 9  // DeferredRender only
#define MATERIAL_AMB_OCC_TEXTURE_REGISTER    10 // DeferredRender only
#define MATERIAL_HEIGHT_MAP_TEXTURE_REGISTER 11 // DeferredRender only
#define MATERIAL_IBL_MAP_TEXTURE_REGISTER    12
#define MATERIAL_ENV_MAP_TEXTURE_REGISTER    13

// t#
#define GBUFFER_RT0_REGISTER 16 // DeferredLight only
#define GBUFFER_RT1_REGISTER 17 // DeferredLight only
#define GBUFFER_RT2_REGISTER 18 // DeferredLight only
#define GBUFFER_RT3_REGISTER 19 // DeferredLight only
#define GBUFFER_ENV_REGISTER 20 // DeferredLight only
#define GBUFFER_IBL_REGISTER 21 // DeferredLight only

// s#
#define GBUFFER_SAMPLER_REGISTER 6 // DeferredLight only

// GBuffer Attributes
#define GBUFFER_POSITION     0
#define GBUFFER_NORMAL       1
#define GBUFFER_ALBEDO       2
#define GBUFFER_F0           3
#define GBUFFER_ROUGHNESS    4
#define GBUFFER_METALNESS    5
#define GBUFFER_AMB_OCC      6
#define GBUFFER_IBL_STRENGTH 7
#define GBUFFER_ENV_STRENGTH 8
}


#define ENABLE_GPU_QUERIES

class Example_020 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;

private:
	struct PerFrame
	{
		vkr::CommandBufferPtr cmd;
		vkr::SemaphorePtr     imageAcquiredSemaphore;
		vkr::FencePtr         imageAcquiredFence;
		vkr::SemaphorePtr     renderCompleteSemaphore;
		vkr::FencePtr         renderCompleteFence;
#ifdef ENABLE_GPU_QUERIES
		vkr::QueryPtr timestampQuery;
		vkr::QueryPtr pipelineStatsQuery;
#endif
	};

	vkr::PipelineStatistics mPipelineStatistics = {};
	uint64_t                 mTotalGpuFrameTime = 0;

	std::vector<PerFrame>        mPerFrame;
	vkr::DescriptorPoolPtr      mDescriptorPool;
	PerspectiveCamera                  mCamera;
	vkr::DescriptorSetLayoutPtr mSceneDataLayout;
	vkr::DescriptorSetPtr       mSceneDataSet;
	vkr::BufferPtr              mCpuSceneConstants;
	vkr::BufferPtr              mGpuSceneConstants;
	vkr::BufferPtr              mCpuLightConstants;
	vkr::BufferPtr              mGpuLightConstants;

	vkr::SamplerPtr mSampler;

	vkr::DrawPassPtr            mGBufferRenderPass;
	vkr::TexturePtr             mGBufferLightRenderTarget;
	vkr::DrawPassPtr            mGBufferLightPass;
	vkr::DescriptorSetLayoutPtr mGBufferReadLayout;
	vkr::DescriptorSetPtr       mGBufferReadSet;
	vkr::BufferPtr              mGBufferDrawAttrConstants;
	bool                         mEnableIBL = false;
	bool                         mEnableEnv = false;
	vkr::FullscreenQuadPtr      mGBufferLightQuad;
	vkr::FullscreenQuadPtr      mDebugDrawQuad;

	vkr::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
	vkr::DescriptorSetPtr       mDrawToSwapchainSet;
	vkr::FullscreenQuadPtr      mDrawToSwapchain;

	vkr::MeshPtr       mSphere;
	vkr::MeshPtr       mBox;
	std::vector<e020::Entity> mEntities;

	vkr::TexturePtr m1x1WhiteTexture;

	float mCamSwing = 0;
	float mTargetCamSwing = 0;

	bool                     mDrawGBufferAttr = false;
	uint32_t                 mGBufferAttrIndex = 0;
	std::vector<const char*> mGBufferAttrNames = {
		"POSITION",
		"NORMAL",
		"ALBEDO",
		"F0",
		"ROUGHNESS",
		"METALNESS",
		"AMB_OCC",
		"IBL_STRENGTH",
		"ENV_STRENGTH",
	};

	void setupPerFrame();
	void setupEntities();
	void setupGBufferPasses();
	void setupGBufferLightQuad();
	void setupDebugDraw();
	void setupDrawToSwapchain();
	void updateConstants();
};