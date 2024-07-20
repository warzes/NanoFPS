#pragma once

#include "RenderResource.h"

namespace Renderer
{
	bool Init();
	void Close();
	void Draw();

	VmaAllocator GetAllocator();
}