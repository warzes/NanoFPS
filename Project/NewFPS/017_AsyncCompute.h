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
		vkr::SemaphorePtr imageAcquiredSemaphore;
		vkr::FencePtr     imageAcquiredFence;
		vkr::SemaphorePtr renderCompleteSemaphore;
		vkr::FencePtr     renderCompleteFence;

		// Graphics pipeline objects.
		struct RenderData
		{
			vkr::CommandBufferPtr cmd;
			vkr::DescriptorSetPtr descriptorSet;
			vkr::BufferPtr        constants;
			vkr::DrawPassPtr      drawPass;
			vkr::SemaphorePtr     completeSemaphore;
		};

		std::array<RenderData, 4> renderData;

		// Compute pipeline objects.
		struct ComputeData
		{
			vkr::CommandBufferPtr    cmd;
			vkr::DescriptorSetPtr    descriptorSet;
			vkr::BufferPtr           constants;
			vkr::ImagePtr            outputImage;
			vkr::SampledImageViewPtr outputImageSampledView;
			vkr::StorageImageViewPtr outputImageStorageView;
			vkr::SemaphorePtr        completeSemaphore;
		};
		std::array<ComputeData, 4> computeData;

		// Final image composition objects.
		struct ComposeData
		{
			vkr::CommandBufferPtr cmd;
			vkr::DescriptorSetPtr descriptorSet;
			vkr::BufferPtr        quadVertexBuffer;
			vkr::SemaphorePtr     completeSemaphore;
		};
		std::array<ComposeData, 4> composeData;
		vkr::DrawPassPtr          composeDrawPass;

		// Draw to swapchain objects.
		struct DrawToSwapchainData
		{
			vkr::CommandBufferPtr cmd;
			vkr::DescriptorSetPtr descriptorSet;
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

	vkr::MeshPtr    mModelMesh;
	vkr::TexturePtr mModelTexture;
	float            mModelRotation = 45.0f;
	float            mModelTargetRotation = 45.0f;

	int mGraphicsLoad = 150;
	int mComputeLoad = 5;

	vkr::SamplerPtr mLinearSampler;
	vkr::SamplerPtr mNearestSampler;

	// This will be a compute queue if async compute is enabled or a graphics queue otherwise.
	vkr::QueuePtr mComputeQueue;
	vkr::QueuePtr mGraphicsQueue;

	vkr::DescriptorPoolPtr mDescriptorPool;

	vkr::DescriptorSetLayoutPtr mRenderLayout;
	vkr::GraphicsPipelinePtr    mRenderPipeline;
	vkr::PipelineInterfacePtr   mRenderPipelineInterface;

	vkr::DescriptorSetLayoutPtr mComputeLayout;
	vkr::ComputePipelinePtr     mComputePipeline;
	vkr::PipelineInterfacePtr   mComputePipelineInterface;

	vkr::DescriptorSetLayoutPtr mComposeLayout;
	vkr::GraphicsPipelinePtr    mComposePipeline;
	vkr::PipelineInterfacePtr   mComposePipelineInterface;
	vkr::VertexBinding          mComposeVertexBinding;

	vkr::DescriptorSetLayoutPtr mDrawToSwapchainLayout;
	vkr::FullscreenQuadPtr      mDrawToSwapchainPipeline;

	bool mAsyncComputeEnabled = true;
	bool mUseQueueFamilyTransfers = true;
};