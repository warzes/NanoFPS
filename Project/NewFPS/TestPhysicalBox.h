#pragma once

#include "Entity.h"

class GameApplication;

class TestPhysicalBox final
{
public:
	bool Setup(GameApplication* game);
	void Shutdown();

	void DrawDebug(vkr::CommandBufferPtr cmd, const float4x4& matPV);

	//void UpdateShaderUniform(uint32_t dataSize, const void* srcData);

private:
	vkr::DescriptorSetLayoutPtr m_setLayout;
	vkr::PipelineInterfacePtr   m_pipelineInterface;
	vkr::GraphicsPipelinePtr    m_pipeline;
	GameEntity                  m_model;

	ph::RigidBodyPtr rb;
	ph::StaticBodyPtr plane;
};