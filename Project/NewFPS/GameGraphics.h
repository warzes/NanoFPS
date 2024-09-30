#pragma once

#include "PerFrame.h"

class GameGraphics final
{
public:
	bool Setup(vkr::RenderDevice& device);
	void Shutdown();

	VulkanPerFrameData& FrameData(size_t id) { assert(id < m_perFrame.size()); return m_perFrame[id]; }

private:
	std::vector<VulkanPerFrameData> m_perFrame;
};