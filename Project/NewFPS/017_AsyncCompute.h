#pragma once

class Example_017 final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;
	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;

private:
	void setupComposition();
	void setupCompute();
	void setupDrawToSwapchain();

	struct PerFrame
	{
		SemaphorePtr imageAcquiredSemaphore;
		FencePtr     imageAcquiredFence;
		SemaphorePtr renderCompleteSemaphore;
		FencePtr     renderCompleteFence;

		// Graphics pipeline objects.
		struct RenderData
		{
			CommandBufferPtr cmd;
			DescriptorSetPtr descriptorSet;
			BufferPtr        constants;
			DrawPassPtr      drawPass;
			SemaphorePtr     completeSemaphore;
		};

		std::array<RenderData, 4> renderData;

		// Compute pipeline objects.
		struct ComputeData
		{
			CommandBufferPtr    cmd;
			DescriptorSetPtr    descriptorSet;
			BufferPtr           constants;
			ImagePtr            outputImage;
			SampledImageViewPtr outputImageSampledView;
			StorageImageViewPtr outputImageStorageView;
			SemaphorePtr        completeSemaphore;
		};
		std::array<ComputeData, 4> computeData;

		// Final image composition objects.
		struct ComposeData
		{
			CommandBufferPtr cmd;
			DescriptorSetPtr descriptorSet;
			BufferPtr        quadVertexBuffer;
			SemaphorePtr     completeSemaphore;
		};
		std::array<ComposeData, 4> composeData;
		DrawPassPtr          composeDrawPass;

		// Draw to swapchain objects.
		struct DrawToSwapchainData
		{
			CommandBufferPtr cmd;
			DescriptorSetPtr descriptorSet;
		};
		DrawToSwapchainData drawToSwapchainData;
	};
	std::vector<PerFrame> mPerFrame;
	const uint32_t        mNumFramesInFlight = 2;

	void     updateTransforms(PerFrame& frame);
	uint32_t acquireFrame(PerFrame& frame);
	void     blitAndPresent(PerFrame& frame, uint32_t swapchainImageIndex);
	void     runCompute(PerFrame& frame, size_t quadIndex);
	void     compose(PerFrame& frame, size_t quadIndex);
	void     drawScene(PerFrame& frame, size_t quadIndex);

	PerspCamera mCamera;

	MeshPtr    mModelMesh;
	TexturePtr mModelTexture;
	float            mModelRotation = 45.0f;
	float            mModelTargetRotation = 45.0f;

	int mGraphicsLoad = 150;
	int mComputeLoad = 5;

	SamplerPtr mLinearSampler;
	SamplerPtr mNearestSampler;

	// This will be a compute queue if async compute is enabled or a graphics queue otherwise.
	QueuePtr mComputeQueue;
	QueuePtr mGraphicsQueue;

	DescriptorPoolPtr mDescriptorPool;

	DescriptorSetLayoutPtr mRenderLayout;
	GraphicsPipelinePtr    mRenderPipeline;
	PipelineInterfacePtr   mRenderPipelineInterface;

	DescriptorSetLayoutPtr mComputeLayout;
	ComputePipelinePtr     mComputePipeline;
	PipelineInterfacePtr   mComputePipelineInterface;

	DescriptorSetLayoutPtr mComposeLayout;
	GraphicsPipelinePtr    mComposePipeline;
	PipelineInterfacePtr   mComposePipelineInterface;
	VertexBinding          mComposeVertexBinding;

	DescriptorSetLayoutPtr mDrawToSwapchainLayout;
	FullscreenQuadPtr      mDrawToSwapchainPipeline;

	bool mAsyncComputeEnabled = true;
	bool mUseQueueFamilyTransfers = true;
};