#if defined(_MSC_VER)
#	pragma warning(push, 3)
//#	pragma warning(disable : 5219)
#endif

#define _USE_MATH_DEFINES

#include <GLFW/glfw3.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif

//-----------------------------------------------------------------------------
int main(
	[[maybe_unused]] int   argc,
	[[maybe_unused]] char* argv[])
{
	if (!glfwInit())
	{
		//Fatal("Failed to initialize GLFW");
		return false;
	}
	//error: " + std::string(desc)); });

}
//-----------------------------------------------------------------------------