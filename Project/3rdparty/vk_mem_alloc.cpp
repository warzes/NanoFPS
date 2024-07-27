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

#define WIN32
#define VULKAN_HPP_NO_EXCEPTIONS 1
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_DEBUG_ALWAYS_DEDICATED_MEMORY 0
#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 0
#define VMA_DEBUG_GLOBAL_MUTEX 0
#define VMA_DEBUG_DONT_EXCEED_MAX_MEMORY_ALLOCATION_COUNT 0
#define VMA_RECORDING_ENABLED 0
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"