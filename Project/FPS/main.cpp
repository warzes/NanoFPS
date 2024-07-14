#if defined(_MSC_VER)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#define _USE_MATH_DEFINES

#include <GLFW/glfw3.h>
#include <VkBootstrap/VkBootstrap.h>

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
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Example Vulkan Application")
		.request_validation_layers()
		.use_default_debug_messenger()
		.build();
	if (!inst_ret)
	{
		//std::cerr << "Failed to create Vulkan instance. Error: " << inst_ret.error().message() << "\n";
		return false;
	}
	vkb::Instance vkb_inst = inst_ret.value();

	if (!glfwInit())
	{
		//Fatal("Failed to initialize GLFW");
		return false;
	}
	//error: " + std::string(desc)); });
}
//-----------------------------------------------------------------------------