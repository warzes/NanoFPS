#include "stdafx.h"
#include "GameApp.h"
#include "001Triangle.h"
#include "002TriangleSpinning.h"
#include "003_SquateTextured.h"
#include "004_Cube.h"
#include "005_CubeTextured.h"
#include "006_ComputeFill.h"
#include "007_DrawIndexed.h"
#include "008_BasicGeometry.h"
#include "009_ObjGeometry.h"
#include "010_CubeMap.h"
#include "011_CompressedTexture.h"
#include "012_Shadows.h"
#include "013_NormalMap.h"
#include "014_ArcballCamera.h"

#include "016_ImguiAndInput.h"

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
	//Example_004 app;
	//Example_005 app;
	//Example_006 app;
	//Example_007 app;
	//Example_008 app;
	//Example_009 app;
	//Example_010 app;
	//Example_011 app;
	//Example_012 app;
	//Example_013 app;
	Example_014 app;
	//Example_015 app;

	app.Run();
}
//-----------------------------------------------------------------------------