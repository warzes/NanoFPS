#pragma once

#include "Entity.h"

class GameApplication;

class DirectionalLight final
{
public:
	bool Setup(GameApplication* game);
	void Shutdown();
	void Update(float deltaTime);
	void DrawDebug(vkr::CommandBufferPtr cmd);

private:
	GameApplication* m_game;
	vkr::DescriptorSetLayoutPtr mLightSetLayout;
	vkr::PipelineInterfacePtr   mLightPipelineInterface;
	vkr::GraphicsPipelinePtr    mLightPipeline;
	GameEntity                  mLight;
	float3                      mLightPosition = float3(0, 5, 5);
	PerspCamera                 mLightCamera;
};