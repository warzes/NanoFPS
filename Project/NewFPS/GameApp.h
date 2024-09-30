#pragma 

#include "PerFrame.h"
#include "World.h"
#include "Entity.h"
#include "Light.h"

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

	vkr::DescriptorPoolPtr GetDescriptorPool() { return m_descriptorPool; }

private:
	std::vector<VulkanPerFrameData> m_perFrame;
	vkr::DescriptorPoolPtr m_descriptorPool;

	World m_world;

	std::set<KeyCode> m_pressedKeys;

private:
	bool setupDescriptors();
	bool setupEntities();
	bool setupPipelines();
	bool setupShadowRenderPass();
	bool setupShadowInfo();

	void processInput();
	void updateUniformBuffer();

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

	bool                   mUsePCF = false;

	bool m_cursorVisible = true;
};