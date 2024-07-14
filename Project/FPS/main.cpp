#include "NanoEngineVK.h"
// TODO: написать VK_CHECK
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	if (EngineApp::Create())
	{
		while (!EngineApp::ShouldClose())
		{
			EngineApp::BeginFrame();

			EngineApp::EndFrame();
		}
	}
	EngineApp::Destroy();
}
//-----------------------------------------------------------------------------