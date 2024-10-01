#pragma once

struct GameEntity;

class ShadowPass final
{
public:
	bool Setup(vkr::RenderDevice& device);
	void Shutdown();

	void Draw(vkr::CommandBufferPtr cmd, const std::vector<GameEntity*>& entities);

	vkr::DescriptorSetLayoutPtr GetDescriptorSetLayout() { return m_shadowSetLayout; }
	vkr::SampledImageViewPtr GetSampledImageView() { return mShadowImageView; }
	vkr::SamplerPtr GetSampler() { return mShadowSampler; }

	bool& UsePCF() { return mUsePCF; }
	bool UsePCF() const { return mUsePCF; }

private:
	vkr::DescriptorSetLayoutPtr m_shadowSetLayout;
	vkr::PipelineInterfacePtr   mShadowPipelineInterface;
	vkr::GraphicsPipelinePtr    mShadowPipeline;
	vkr::RenderPassPtr          mShadowRenderPass;
	vkr::SampledImageViewPtr    mShadowImageView;
	vkr::SamplerPtr             mShadowSampler;

	bool                        mUsePCF = false;
};