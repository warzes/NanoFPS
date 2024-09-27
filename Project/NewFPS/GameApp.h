#pragma 

#include "PerFrame.h"
#include "World.h"
#include "Entity.h"

namespace game
{
	constexpr auto NumMaxEntities = 512u;
} // game

class GameApplication final : public EngineApplication
{
public:
	EngineApplicationCreateInfo Config() const final;

	bool Setup() final;
	void Shutdown() final;
	void Update() final;
	void Render() final;

	void MouseMove(int32_t x, int32_t y, int32_t dx, int32_t dy, MouseButton buttons) final;
	void KeyDown(KeyCode key) final;
	void KeyUp(KeyCode key) final;

private:
	std::vector<VulkanPerFrameData> m_perFrame;
	World m_world;

private:
	bool setupDescriptors();
	bool setupEntities();
	bool setupPipelines();
	bool setupShadowRenderPass();
	bool setupShadowInfo();
	bool setupLight();

	void updateLight();
	void processInput();

	void updateUniformBuffer();

	std::set<KeyCode> m_pressedKeys;

	vkr::DescriptorPoolPtr      m_descriptorPool;

	vkr::DescriptorSetLayoutPtr m_drawObjectSetLayout;
	vkr::PipelineInterfacePtr   mDrawObjectPipelineInterface;
	vkr::GraphicsPipelinePtr    mDrawObjectPipeline;
	GameEntity                 mGroundPlane;
	GameEntity                 mCube;
	GameEntity                 mKnob;
	std::vector<GameEntity*>   mEntities;

	vkr::DescriptorSetLayoutPtr m_shadowSetLayout;
	vkr::PipelineInterfacePtr   mShadowPipelineInterface;
	vkr::GraphicsPipelinePtr    mShadowPipeline;
	vkr::RenderPassPtr          mShadowRenderPass;
	vkr::SampledImageViewPtr    mShadowImageView;
	vkr::SamplerPtr             mShadowSampler;

	vkr::DescriptorSetLayoutPtr mLightSetLayout;
	vkr::PipelineInterfacePtr   mLightPipelineInterface;
	vkr::GraphicsPipelinePtr    mLightPipeline;
	GameEntity                 mLight;
	float3                 mLightPosition = float3(0, 5, 5);
	PerspCamera            mLightCamera;
	bool                   mUsePCF = false;

	bool m_cursorVisible = true;
};