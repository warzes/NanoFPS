#include "NanoEngineVK.h"
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