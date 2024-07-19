#include "Base.h"
#include "Core.h"
#include "Renderer.h"
#include "EngineWindow.h"

namespace
{
	bool UseValidationLayers = false;
}

bool initVulkan()
{
	return true;
}

bool Renderer::Init()
{
#ifdef _DEBUG
	UseValidationLayers = true;
#endif

	if (!initVulkan()) return false;

	return true;
}

void Renderer::Close()
{
}

void Renderer::Draw()
{
}