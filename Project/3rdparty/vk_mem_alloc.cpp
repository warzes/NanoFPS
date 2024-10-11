#if defined(_MSC_VER)
#	pragma warning(disable : 4061)
#	pragma warning(disable : 4100)
#	pragma warning(disable : 4127)
#	pragma warning(disable : 4189)
#	pragma warning(disable : 4191)
#	pragma warning(disable : 4324)
#	pragma warning(disable : 4355)
#	pragma warning(disable : 4365)
#	pragma warning(disable : 4505)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4625)
#	pragma warning(disable : 4626)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5027)
#	pragma warning(disable : 5045)
#	pragma warning(disable : 5026)
#endif

#define VK_NO_PROTOTYPES
#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include "vk_mem_alloc.h"