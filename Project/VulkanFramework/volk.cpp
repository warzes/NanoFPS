#if defined(_MSC_VER)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#if defined(_WIN32)
#	define VK_USE_PLATFORM_WIN32_KHR 1
#endif
#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif