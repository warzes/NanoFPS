#include "stdafx.h"
#include "GameApp.h"

отсюда всякое для шутера
https://github.com/blurrypiano/littleVulkanEngine
urchinengine
magma-engine

#if defined(_MSC_VER)
#	pragma comment( lib, "NanoEngine.lib" )
#	pragma comment( lib, "NanoEngine3rdparty.lib" )
#endif
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	GameApplication app;
	app.Run();
}
//-----------------------------------------------------------------------------