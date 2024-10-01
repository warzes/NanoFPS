#include "stdafx.h"
#include "GameGraphics.h"

bool GameGraphics::CreateDescriptorPool(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo)
{
	// Create descriptor pool large enough for this project
	{
		vkr::DescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.uniformBuffer = createInfo.descriptorPool.maxUniformBuffer;
		poolCreateInfo.sampledImage = createInfo.descriptorPool.maxSampledImage;
		poolCreateInfo.sampler = createInfo.descriptorPool.maxSampler;
		CHECKED_CALL_AND_RETURN_FALSE(device.CreateDescriptorPool(poolCreateInfo, &m_descriptorPool));
	}

	return true;
}

bool GameGraphics::CreateShadowPass(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo, std::vector<GameEntity*>& entities)
{
	return m_shadowPass.Setup(device, entities);
}

bool GameGraphics::CreateFrameData(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo)
{
	VulkanPerFrameData perFrame;
	if (!perFrame.Setup(device)) return false;
	m_perFrame.emplace_back(perFrame);
	return true;
}

void GameGraphics::Shutdown(vkr::RenderDevice& device)
{
	m_shadowPass.Shutdown();

	for (size_t i = 0; i < m_perFrame.size(); i++)
	{
		m_perFrame[i].Shutdown();
	}
	m_shadowPass.Shutdown();
	//device.DestroyDescriptorPool(m_descriptorPool);
	m_descriptorPool.Reset();
}