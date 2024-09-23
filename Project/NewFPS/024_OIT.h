#pragma once

#define DEPTH_PEELING_LAYERS_COUNT              8
#define DEPTH_PEELING_DEPTH_TEXTURES_COUNT      2

class Example_024 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

private:
	enum Algorithm : int32_t
	{
		ALGORITHM_UNSORTED_OVER,
		ALGORITHM_WEIGHTED_SUM,
		ALGORITHM_WEIGHTED_AVERAGE,
		ALGORITHM_DEPTH_PEELING,
		ALGORITHM_BUFFER,
		ALGORITHMS_COUNT,
	};

	enum MeshType : int32_t
	{
		MESH_TYPE_MONKEY,
		MESH_TYPE_HORSE,
		MESH_TYPE_MEGAPHONE,
		MESH_TYPE_CANNON,
		MESH_TYPES_COUNT,
	};

	enum FaceMode : int32_t
	{
		FACE_MODE_ALL,
		FACE_MODE_ALL_BACK_THEN_FRONT,
		FACE_MODE_BACK_ONLY,
		FACE_MODE_FRONT_ONLY,
		FACE_MODES_COUNT,
	};

	enum WeightAverageType : int32_t
	{
		WEIGHTED_AVERAGE_TYPE_FRAGMENT_COUNT,
		WEIGHTED_AVERAGE_TYPE_EXACT_COVERAGE,
		WEIGHTED_AVERAGE_TYPES_COUNT,
	};

	enum BufferAlgorithmType : int32_t
	{
		BUFFER_ALGORITHM_BUCKETS,
		BUFFER_ALGORITHM_LINKED_LISTS,
		BUFFER_ALGORITHMS_COUNT,
	};

	struct GuiParameters
	{
		int32_t algorithmDataIndex;

		struct
		{
			float color[3];
			bool  display;
		} background;

		struct
		{
			MeshType type;
			float    opacity;
			float    scale;
			bool     auto_rotate;
		} mesh;

		struct
		{
			FaceMode faceMode;
		} unsortedOver;

		struct
		{
			WeightAverageType type;
		} weightedAverage;

		struct
		{
			int32_t startLayer;
			int32_t layersCount;
		} depthPeeling;

		struct
		{
			BufferAlgorithmType type;
			int32_t             bucketsFragmentsMaxCount;
			int32_t             listsFragmentBufferScale;
			int32_t             listsSortedFragmentMaxCount;
		} buffer;
	};

	std::vector<const char*> mSupportedAlgorithmNames;
	std::vector<Algorithm>   mSupportedAlgorithmIds;

private:
	void SetupCommon();
	void SetupUnsortedOver();
	void SetupWeightedSum();
	void SetupWeightedAverage();
	void SetupDepthPeeling();
	void SetupBuffer();
	void SetupBufferBuckets();
	void SetupBufferLinkedLists();

	void FillSupportedAlgorithmData();
	void ParseCommandLineOptions();

	Algorithm     GetSelectedAlgorithm() const;
	vkr::MeshPtr GetTransparentMesh() const;

	void UpdateGUI();

	void RecordOpaque();
	void RecordTransparency();
	void RecordComposite(vkr::RenderPassPtr renderPass);

	void RecordUnsortedOver();
	void RecordWeightedSum();
	void RecordWeightedAverage();
	void RecordDepthPeeling();
	void RecordBuffer();
	void RecordBufferBuckets();
	void RecordBufferLinkedLists();

private:
	GuiParameters mGuiParameters = {};

	float mPreviousElapsedSeconds;
	float mMeshAnimationSeconds;

	vkr::SemaphorePtr mImageAcquiredSemaphore;
	vkr::FencePtr     mImageAcquiredFence;
	vkr::SemaphorePtr mRenderCompleteSemaphore;
	vkr::FencePtr     mRenderCompleteFence;

	vkr::CommandBufferPtr  mCommandBuffer;
	vkr::DescriptorPoolPtr mDescriptorPool;

	vkr::SamplerPtr mNearestSampler;

	vkr::MeshPtr mBackgroundMesh;
	vkr::MeshPtr mTransparentMeshes[MESH_TYPES_COUNT];

	vkr::BufferPtr mShaderGlobalsBuffer;

	vkr::DrawPassPtr            mOpaquePass;
	vkr::DescriptorSetLayoutPtr mOpaqueDescriptorSetLayout;
	vkr::DescriptorSetPtr       mOpaqueDescriptorSet;
	vkr::PipelineInterfacePtr   mOpaquePipelineInterface;
	vkr::GraphicsPipelinePtr    mOpaquePipeline;

	vkr::TexturePtr  mTransparencyTexture;
	vkr::DrawPassPtr mTransparencyPass;

	vkr::DescriptorSetLayoutPtr mCompositeDescriptorSetLayout;
	vkr::DescriptorSetPtr       mCompositeDescriptorSet;
	vkr::PipelineInterfacePtr   mCompositePipelineInterface;
	vkr::GraphicsPipelinePtr    mCompositePipeline;

	struct
	{
		vkr::DescriptorSetLayoutPtr descriptorSetLayout;
		vkr::DescriptorSetPtr       descriptorSet;

		vkr::PipelineInterfacePtr pipelineInterface;
		vkr::GraphicsPipelinePtr  meshAllFacesPipeline;
		vkr::GraphicsPipelinePtr  meshBackFacesPipeline;
		vkr::GraphicsPipelinePtr  meshFrontFacesPipeline;
	} mUnsortedOver;

	struct
	{
		vkr::DescriptorSetLayoutPtr descriptorSetLayout;
		vkr::DescriptorSetPtr       descriptorSet;

		vkr::PipelineInterfacePtr pipelineInterface;
		vkr::GraphicsPipelinePtr  pipeline;
	} mWeightedSum;

	struct
	{
		vkr::TexturePtr colorTexture;
		vkr::TexturePtr extraTexture;

		vkr::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
		vkr::DescriptorSetPtr       gatherDescriptorSet;
		vkr::PipelineInterfacePtr   gatherPipelineInterface;

		vkr::DescriptorSetLayoutPtr combineDescriptorSetLayout;
		vkr::DescriptorSetPtr       combineDescriptorSet;
		vkr::PipelineInterfacePtr   combinePipelineInterface;

		struct
		{
			vkr::DrawPassPtr         gatherPass;
			vkr::GraphicsPipelinePtr gatherPipeline;

			vkr::GraphicsPipelinePtr combinePipeline;
		} count;

		struct
		{
			vkr::DrawPassPtr         gatherPass;
			vkr::GraphicsPipelinePtr gatherPipeline;

			vkr::GraphicsPipelinePtr combinePipeline;
		} coverage;
	} mWeightedAverage;

	struct
	{
		vkr::TexturePtr  layerTextures[DEPTH_PEELING_LAYERS_COUNT];
		vkr::TexturePtr  depthTextures[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
		vkr::DrawPassPtr layerPasses[DEPTH_PEELING_LAYERS_COUNT];

		vkr::DescriptorSetLayoutPtr layerDescriptorSetLayout;
		vkr::DescriptorSetPtr       layerDescriptorSets[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
		vkr::PipelineInterfacePtr   layerPipelineInterface;
		vkr::GraphicsPipelinePtr    layerPipeline_OtherLayers;
		vkr::GraphicsPipelinePtr    layerPipeline_FirstLayer;

		vkr::DescriptorSetLayoutPtr combineDescriptorSetLayout;
		vkr::DescriptorSetPtr       combineDescriptorSet;
		vkr::PipelineInterfacePtr   combinePipelineInterface;
		vkr::GraphicsPipelinePtr    combinePipeline;
	} mDepthPeeling;

	struct
	{
		struct
		{
			vkr::TexturePtr  countTexture;
			vkr::TexturePtr  fragmentTexture;
			vkr::DrawPassPtr clearPass;
			vkr::DrawPassPtr gatherPass;

			vkr::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
			vkr::DescriptorSetPtr       gatherDescriptorSet;
			vkr::PipelineInterfacePtr   gatherPipelineInterface;
			vkr::GraphicsPipelinePtr    gatherPipeline;

			vkr::DescriptorSetLayoutPtr combineDescriptorSetLayout;
			vkr::DescriptorSetPtr       combineDescriptorSet;
			vkr::PipelineInterfacePtr   combinePipelineInterface;
			vkr::GraphicsPipelinePtr    combinePipeline;

			bool countTextureNeedClear;
		} buckets;

		struct
		{
			vkr::TexturePtr  linkedListHeadTexture;
			vkr::BufferPtr   fragmentBuffer;
			vkr::BufferPtr   atomicCounter;
			vkr::DrawPassPtr clearPass;
			vkr::DrawPassPtr gatherPass;

			vkr::DescriptorSetLayoutPtr gatherDescriptorSetLayout;
			vkr::DescriptorSetPtr       gatherDescriptorSet;
			vkr::PipelineInterfacePtr   gatherPipelineInterface;
			vkr::GraphicsPipelinePtr    gatherPipeline;

			vkr::DescriptorSetLayoutPtr combineDescriptorSetLayout;
			vkr::DescriptorSetPtr       combineDescriptorSet;
			vkr::PipelineInterfacePtr   combinePipelineInterface;
			vkr::GraphicsPipelinePtr    combinePipeline;

			bool linkedListHeadTextureNeedClear;
		} lists;
	} mBuffer;
};