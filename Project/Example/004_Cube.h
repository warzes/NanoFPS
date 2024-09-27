#pragma once

class Example_004 final : public EngineApplication
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

	std::vector<PerFrame>      mPerFrame;
	std::vector<vkr::RenderPassPtr> mRenderPasses;
	vkr::ShaderModulePtr            mVS;
	vkr::ShaderModulePtr            mPS;
	vkr::PipelineInterfacePtr       mPipelineInterface;
	vkr::GraphicsPipelinePtr        mPipeline;
	vkr::BufferPtr                  mVertexBuffer;
	vkr::DescriptorPoolPtr          mDescriptorPool;
	vkr::DescriptorSetLayoutPtr     mDescriptorSetLayout;
	vkr::DescriptorSetPtr           mDescriptorSet;
	vkr::BufferPtr                  mUniformBuffer;
	vkr::VertexBinding              mVertexBinding;
};