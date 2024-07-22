#include "NanoEngineVK.h"
#include "GameApp.h"
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	if (EngineApp::Create())
	{
		if (GameApp::Create())
		{
			while (!EngineApp::ShouldClose())
			{
				GameApp::Update();

				EngineApp::BeginFrame();
				GameApp::Frame();
				EngineApp::EndFrame();
			}
		}
		GameApp::Destroy();
	}
	EngineApp::Destroy();
}
//-----------------------------------------------------------------------------