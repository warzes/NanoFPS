#include "GameApp.h"
#include "001Triangle.h"
#include "002TriangleSpinning.h"
#include "003_SquateTextured.h"
#include "004_Cube.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "NanoEngine.lib" )
#endif
//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	//GameApplication app;

	//Example_001 app;
	//Example_002 app;
	//Example_003 app;
	Example_004 app;

	app.Run();
}
//-----------------------------------------------------------------------------