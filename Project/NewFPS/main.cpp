#include "GameApp.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "NanoEngine.lib" )
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