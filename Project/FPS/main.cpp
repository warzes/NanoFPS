#include "GameApp.h"
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	FileSystem::Init();
	FileSystem::Mount("Data", "/");
	if (EngineApp::Create({}))
	{
		if (GameApp::Create())
		{
			while (!EngineApp::ShouldClose())
			{
				EngineApp::Update();
				GameApp::Update();

				EngineApp::BeginFrame();
				GameApp::Frame();
				EngineApp::EndFrame();
			}
		}
		GameApp::Destroy();
	}
	EngineApp::Destroy();
	FileSystem::Shutdown();
}
//-----------------------------------------------------------------------------