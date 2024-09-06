#include "GameApp.h"

https://www.youtube.com/watch?v=kh1zqOVvBVo

#if defined(_MSC_VER)
#	pragma comment( lib, "NanoEngine.lib" )
#endif
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	EngineApplication::Run<GameApplication>();
}
//-----------------------------------------------------------------------------