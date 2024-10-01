#pragma once

#include "PerFrame.h"
#include "ShadowPass.h"

struct GameGraphicsCreateInfo final
{
	struct
	{
		uint32_t maxUniformBuffer = 1024;
		uint32_t maxSampledImage = 1024;
		uint32_t maxSampler = 1024;
	} descriptorPool;
};

class GameGraphics final
{
public:
	bool CreateDescriptorPool(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo);
	bool CreateShadowPass(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo, std::vector<GameEntity*>& entities);
	bool CreateFrameData(vkr::RenderDevice& device, const GameGraphicsCreateInfo& createInfo);

	void Shutdown(vkr::RenderDevice& device);

	VulkanPerFrameData& FrameData(size_t id) { assert(id < m_perFrame.size()); return m_perFrame[id]; }
	vkr::DescriptorPoolPtr GetDescriptorPool() { return m_descriptorPool; }
	ShadowPass& GetShadowPass() { return m_shadowPass; }

private:
	std::vector<VulkanPerFrameData> m_perFrame;
	vkr::DescriptorPoolPtr m_descriptorPool;
	ShadowPass m_shadowPass;
};