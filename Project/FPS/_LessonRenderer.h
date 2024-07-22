#pragma once

namespace _Renderer
{
	bool Init();
	void Close();
	void Draw();

	VmaAllocator GetAllocator();
}