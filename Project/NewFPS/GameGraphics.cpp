#include "stdafx.h"
#include "GameGraphics.h"

bool GameGraphics::Setup(vkr::RenderDevice& device)
{
	VulkanPerFrameData perFrame;
	if (!perFrame.Setup(device)) return false;
	m_perFrame.emplace_back(perFrame);

	return false;
}

void GameGraphics::Shutdown()
{
	for (size_t i = 0; i < m_perFrame.size(); i++)
	{
		m_perFrame[i].Shutdown();
	}
}
