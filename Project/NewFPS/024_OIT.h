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
	MeshPtr GetTransparentMesh() const;

	void UpdateGUI();

	void RecordOpaque();
	void RecordTransparency();
	void RecordComposite(RenderPassPtr renderPass);

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

	SemaphorePtr mImageAcquiredSemaphore;
	FencePtr     mImageAcquiredFence;
	SemaphorePtr mRenderCompleteSemaphore;
	FencePtr     mRenderCompleteFence;

	CommandBufferPtr  mCommandBuffer;
	DescriptorPoolPtr mDescriptorPool;

	SamplerPtr mNearestSampler;

	MeshPtr mBackgroundMesh;
	MeshPtr mTransparentMeshes[MESH_TYPES_COUNT];

	BufferPtr mShaderGlobalsBuffer;

	DrawPassPtr            mOpaquePass;
	DescriptorSetLayoutPtr mOpaqueDescriptorSetLayout;
	DescriptorSetPtr       mOpaqueDescriptorSet;
	PipelineInterfacePtr   mOpaquePipelineInterface;
	GraphicsPipelinePtr    mOpaquePipeline;

	TexturePtr  mTransparencyTexture;
	DrawPassPtr mTransparencyPass;

	DescriptorSetLayoutPtr mCompositeDescriptorSetLayout;
	DescriptorSetPtr       mCompositeDescriptorSet;
	PipelineInterfacePtr   mCompositePipelineInterface;
	GraphicsPipelinePtr    mCompositePipeline;

	struct
	{
		DescriptorSetLayoutPtr descriptorSetLayout;
		DescriptorSetPtr       descriptorSet;

		PipelineInterfacePtr pipelineInterface;
		GraphicsPipelinePtr  meshAllFacesPipeline;
		GraphicsPipelinePtr  meshBackFacesPipeline;
		GraphicsPipelinePtr  meshFrontFacesPipeline;
	} mUnsortedOver;

	struct
	{
		DescriptorSetLayoutPtr descriptorSetLayout;
		DescriptorSetPtr       descriptorSet;

		PipelineInterfacePtr pipelineInterface;
		GraphicsPipelinePtr  pipeline;
	} mWeightedSum;

	struct
	{
		TexturePtr colorTexture;
		TexturePtr extraTexture;

		DescriptorSetLayoutPtr gatherDescriptorSetLayout;
		DescriptorSetPtr       gatherDescriptorSet;
		PipelineInterfacePtr   gatherPipelineInterface;

		DescriptorSetLayoutPtr combineDescriptorSetLayout;
		DescriptorSetPtr       combineDescriptorSet;
		PipelineInterfacePtr   combinePipelineInterface;

		struct
		{
			DrawPassPtr         gatherPass;
			GraphicsPipelinePtr gatherPipeline;

			GraphicsPipelinePtr combinePipeline;
		} count;

		struct
		{
			DrawPassPtr         gatherPass;
			GraphicsPipelinePtr gatherPipeline;

			GraphicsPipelinePtr combinePipeline;
		} coverage;
	} mWeightedAverage;

	struct
	{
		TexturePtr  layerTextures[DEPTH_PEELING_LAYERS_COUNT];
		TexturePtr  depthTextures[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
		DrawPassPtr layerPasses[DEPTH_PEELING_LAYERS_COUNT];

		DescriptorSetLayoutPtr layerDescriptorSetLayout;
		DescriptorSetPtr       layerDescriptorSets[DEPTH_PEELING_DEPTH_TEXTURES_COUNT];
		PipelineInterfacePtr   layerPipelineInterface;
		GraphicsPipelinePtr    layerPipeline_OtherLayers;
		GraphicsPipelinePtr    layerPipeline_FirstLayer;

		DescriptorSetLayoutPtr combineDescriptorSetLayout;
		DescriptorSetPtr       combineDescriptorSet;
		PipelineInterfacePtr   combinePipelineInterface;
		GraphicsPipelinePtr    combinePipeline;
	} mDepthPeeling;

	struct
	{
		struct
		{
			TexturePtr  countTexture;
			TexturePtr  fragmentTexture;
			DrawPassPtr clearPass;
			DrawPassPtr gatherPass;

			DescriptorSetLayoutPtr gatherDescriptorSetLayout;
			DescriptorSetPtr       gatherDescriptorSet;
			PipelineInterfacePtr   gatherPipelineInterface;
			GraphicsPipelinePtr    gatherPipeline;

			DescriptorSetLayoutPtr combineDescriptorSetLayout;
			DescriptorSetPtr       combineDescriptorSet;
			PipelineInterfacePtr   combinePipelineInterface;
			GraphicsPipelinePtr    combinePipeline;

			bool countTextureNeedClear;
		} buckets;

		struct
		{
			TexturePtr  linkedListHeadTexture;
			BufferPtr   fragmentBuffer;
			BufferPtr   atomicCounter;
			DrawPassPtr clearPass;
			DrawPassPtr gatherPass;

			DescriptorSetLayoutPtr gatherDescriptorSetLayout;
			DescriptorSetPtr       gatherDescriptorSet;
			PipelineInterfacePtr   gatherPipelineInterface;
			GraphicsPipelinePtr    gatherPipeline;

			DescriptorSetLayoutPtr combineDescriptorSetLayout;
			DescriptorSetPtr       combineDescriptorSet;
			PipelineInterfacePtr   combinePipelineInterface;
			GraphicsPipelinePtr    combinePipeline;

			bool linkedListHeadTextureNeedClear;
		} lists;
	} mBuffer;
};