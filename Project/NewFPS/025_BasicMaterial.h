#pragma once

#define ENABLE_GPU_QUERIES

// Skybox registers
#define SKYBOX_CONSTANTS_REGISTER 0
#define SKYBOX_TEXTURE_REGISTER   1
#define SKYBOX_SAMPLER_REGISTER   2

// Material registers
// b#
#define SCENE_CONSTANTS_REGISTER    0
#define MATERIAL_CONSTANTS_REGISTER 1
#define MODEL_CONSTANTS_REGISTER    2
// s#
#define CLAMPED_SAMPLER_REGISTER 3
// t#
#define LIGHT_DATA_REGISTER         4
#define ALBEDO_TEXTURE_REGISTER     5
#define ROUGHNESS_TEXTURE_REGISTER  6
#define METALNESS_TEXTURE_REGISTER  7
#define NORMAL_MAP_TEXTURE_REGISTER 8
#define AMB_OCC_TEXTURE_REGISTER    9
#define HEIGHT_MAP_TEXTURE_REGISTER 10
#define IRR_MAP_TEXTURE_REGISTER    11
#define ENV_MAP_TEXTURE_REGISTER    12
#define BRDF_LUT_TEXTURE_REGISTER   13

const float3 F0_MetalTitanium = float3(0.542f, 0.497f, 0.449f);
const float3 F0_MetalChromium = float3(0.549f, 0.556f, 0.554f);
const float3 F0_MetalIron = float3(0.562f, 0.565f, 0.578f);
const float3 F0_MetalNickel = float3(0.660f, 0.609f, 0.526f);
const float3 F0_MetalPlatinum = float3(0.673f, 0.637f, 0.585f);
const float3 F0_MetalCopper = float3(0.955f, 0.638f, 0.538f);
const float3 F0_MetalPalladium = float3(0.733f, 0.697f, 0.652f);
const float3 F0_MetalZinc = float3(0.664f, 0.824f, 0.850f);
const float3 F0_MetalGold = float3(1.022f, 0.782f, 0.344f);
const float3 F0_MetalAluminum = float3(0.913f, 0.922f, 0.924f);
const float3 F0_MetalSilver = float3(0.972f, 0.960f, 0.915f);
const float3 F0_DiletricWater = float3(0.020f);
const float3 F0_DiletricPlastic = float3(0.040f);
const float3 F0_DiletricGlass = float3(0.045f);
const float3 F0_DiletricCrystal = float3(0.050f);
const float3 F0_DiletricGem = float3(0.080f);
const float3 F0_DiletricDiamond = float3(0.150f);


class Example_025 final : public EngineApplication
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
		CommandBufferPtr cmd;
		SemaphorePtr     imageAcquiredSemaphore;
		FencePtr         imageAcquiredFence;
		SemaphorePtr     renderCompleteSemaphore;
		FencePtr         renderCompleteFence;
#ifdef ENABLE_GPU_QUERIES
		QueryPtr timestampQuery;
		QueryPtr pipelineStatsQuery;
#endif
	};

	PipelineStatistics mPipelineStatistics = {};
	uint64_t                 mTotalGpuFrameTime = 0;

	TexturePtr m1x1BlackTexture;
	TexturePtr m1x1WhiteTexture;

	std::vector<PerFrame>      mPerFrame;
	PerspCamera                mCamera;
	DescriptorPoolPtr    mDescriptorPool;
	std::vector<MeshPtr> mMeshes;
	MeshPtr              mEnvDrawMesh;

	// Descriptor Set 0 - Scene Data
	DescriptorSetLayoutPtr mSceneDataLayout;
	DescriptorSetPtr       mSceneDataSet;
	BufferPtr              mCpuSceneConstants;
	BufferPtr              mGpuSceneConstants;
	BufferPtr              mCpuLightConstants;
	BufferPtr              mGpuLightConstants;

	// Descriptor Set 1 - MaterialData Resources
	DescriptorSetLayoutPtr mMaterialResourcesLayout;
	BufferPtr              mCpuEnvDrawConstants;
	BufferPtr              mGpuEnvDrawConstants;

	struct MaterialResources
	{
		DescriptorSetPtr set;
		TexturePtr       albedoTexture;
		TexturePtr       roughnessTexture;
		TexturePtr       metalnessTexture;
		TexturePtr       normalMapTexture;
	};

	SamplerPtr                  mSampler;
	MaterialResources                 mMetalMaterial;
	MaterialResources                 mWoodMaterial;
	MaterialResources                 mTilesMaterial;
	MaterialResources                 mStoneWallMaterial;
	MaterialResources                 mMeasuringTapeMaterial;
	MaterialResources                 mKiwiMaterial;
	MaterialResources                 mHandPlaneMaterial;
	MaterialResources                 mHorseStatueMaterial;
	std::vector<DescriptorSet*> mMaterialResourcesSets;

	struct IBLResources
	{
		TexturePtr irradianceTexture;
		TexturePtr environmentTexture;
	};
	std::vector<IBLResources> mIBLResources;
	TexturePtr          mBRDFLUTTexture;

	// Descriptor Set 2 - MaterialData Data
	DescriptorSetLayoutPtr mMaterialDataLayout;
	DescriptorSetPtr       mMaterialDataSet;
	BufferPtr              mCpuMaterialConstants;
	BufferPtr              mGpuMaterialConstants;

	// Descriptor Set 3 - Model Data
	DescriptorSetLayoutPtr mModelDataLayout;
	DescriptorSetPtr       mModelDataSet;
	BufferPtr              mCpuModelConstants;
	BufferPtr              mGpuModelConstants;

	// Descriptor Set 4 - Env Draw Data
	DescriptorSetLayoutPtr mEnvDrawLayout;
	DescriptorSetPtr       mEnvDrawSet;

	PipelineInterfacePtr           mPipelineInterface;
	GraphicsPipelinePtr            mGouraudPipeline;
	GraphicsPipelinePtr            mPhongPipeline;
	GraphicsPipelinePtr            mBlinnPhongPipeline;
	GraphicsPipelinePtr            mPBRPipeline;
	std::vector<GraphicsPipeline*> mShaderPipelines;

	PipelineInterfacePtr mEnvDrawPipelineInterface;
	GraphicsPipelinePtr  mEnvDrawPipeline;

	struct MaterialData
	{
		float3 albedo = float3(0.4f, 0.4f, 0.7f);
		float  roughness = 0.5f; // 0 = smooth, 1 = rough
		float  metalness = 0.5f; // 0 = dielectric, 1 = metal
		float  iblStrength = 0.4f; // 0 = nocontrib, 10 = max
		float  envStrength = 0.3f; // 0 = nocontrib, 1 = max
		bool   albedoSelect = 1;    // 0 = value, 1 = texture
		bool   roughnessSelect = 1;    // 0 = value, 1 = texture
		bool   metalnessSelect = 1;    // 0 = value, 1 = texture
		bool   normalSelect = 1;    // 0 = attrb, 1 = texture
		bool   iblSelect = 0;    // 0 = white, 1 = texture
		bool   envSelect = 1;    // 0 = none,  1 = texture
	};

	float        mModelRotY = 0;
	float        mTargetModelRotY = 0;
	float        mCameraRotY = 0;
	float        mTargetCameraRotY = 0;
	float        mAmbient = 0;
	MaterialData mMaterialData = {};
	float3       mAlbedoColor = float3(1);
	bool         mUseBRDFLUT = true;

	std::vector<float3> mF0 = {
	F0_MetalTitanium,
	F0_MetalChromium,
	F0_MetalIron,
	F0_MetalNickel,
	F0_MetalPlatinum,
	F0_MetalCopper,
	F0_MetalPalladium,
	F0_MetalZinc,
	F0_MetalGold,
	F0_MetalAluminum,
	F0_MetalSilver,
	F0_DiletricWater,
	F0_DiletricPlastic,
	F0_DiletricGlass,
	F0_DiletricCrystal,
	F0_DiletricGem,
	F0_DiletricDiamond,
	float3(0.04f),
	};

	uint32_t                 mMeshIndex = 0;
	std::vector<const char*> mMeshNames = {
	"Knob",
	"Sphere",
	"Cube",
	"Monkey",
	"Measuring Tape",
	"Kiwi",
	"Hand Plane",
	"Horse Statue",
	};

	uint32_t                 mF0Index = 0;
	std::vector<const char*> mF0Names = {
	"MetalTitanium",
	"MetalChromium",
	"MetalIron",
	"MetalNickel",
	"MetalPlatinum",
	"MetalCopper",
	"MetalPalladium",
	"MetalZinc",
	"MetalGold",
	"MetalAluminum",
	"MetalSilver",
	"DiletricWater",
	"DiletricPlastic",
	"DiletricGlass",
	"DiletricCrystal",
	"DiletricGem",
	"DiletricDiamond",
	"Use Albedo Color",
	};

	uint32_t                 mMaterialIndex = 0;
	std::vector<const char*> mMaterialNames = {
	"Green Metal Rust",
	"Wood",
	"Tiles",
	"Stone Wall",
	"Measuring Tape",
	"Kiwi",
	"Hand Plane",
	"Horse Statue",
	};

	uint32_t                 mShaderIndex = 3;
	std::vector<const char*> mShaderNames = {
	"Gouraud",
	"Phong",
	"Blinn",
	"PBR",
	};

	uint32_t                 mIBLIndex = 0;
	uint32_t                 mCurrentIBLIndex = 0;
	std::vector<const char*> mIBLNames = {
	"Old Depot",
	"Palermo Square",
	"Venice Sunset",
	"Hilly Terrain",
	"Neon Photo Studio",
	"Sky Lit Garage",
	"Noon Grass",
	};

private:
	void SetupSamplers();
	void SetupMaterialResources(
		const std::filesystem::path& albedoPath,
		const std::filesystem::path& roughnessPath,
		const std::filesystem::path& metalnessPath,
		const std::filesystem::path& normalMapPath,
		MaterialResources& materialResources);
	void SetupMaterials();
	void SetupIBL();
};