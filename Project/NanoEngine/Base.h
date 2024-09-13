#pragma once

#pragma region Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4625)
#	pragma warning(disable : 4626)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#if defined(_MSC_VER)
#	if !defined(VC_EXTRALEAN)
#		define VC_EXTRALEAN
#	endif
#	if !defined(WIN32_LEAN_AND_MEAN)
#		define WIN32_LEAN_AND_MEAN
#	endif
#	if !defined(NOMINMAX)
#		define NOMINMAX
#	endif
#endif
#if !defined(_USE_MATH_DEFINES)
#	define _USE_MATH_DEFINES
#endif

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cfloat>
#include <cmath>

#include <chrono>
#include <memory>
#include <filesystem>
#include <ostream>
#include <fstream>
#include <sstream>
//#include <iostream>
//#include <iomanip>
//#include <functional>
#include <algorithm>
//#include <regex>
#include <optional>
#include <bitset>
#include <span>
#include <array>
#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <mutex>

#define VK_NO_PROTOTYPES

#include <vulkan/vulkan_core.h>
#include <VkBootstrap/VkBootstrap.h>
#include <vulkan/vk_enum_string_helper.h>
//#define VMA_STATIC_VULKAN_FUNCTIONS 0
//#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
//#define VMA_DEBUG_ALWAYS_DEDICATED_MEMORY 0
//#define VMA_DEBUG_INITIALIZE_ALLOCATIONS 0
//#define VMA_DEBUG_GLOBAL_MUTEX 0
//#define VMA_DEBUG_DONT_EXCEED_MAX_MEMORY_ALLOCATION_COUNT 0
//#define VMA_RECORDING_ENABLED 0
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include <vk_mem_alloc.h>
//#define WIN32
//#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
//#define VULKAN_HPP_NO_EXCEPTIONS
//#define VULKAN_HPP_TYPESAFE_CONVERSION 1
////#define VK_NO_PROTOTYPES
////#define VK_USE_PLATFORM_WIN32_KHR 1
//#define VK_PROTOTYPES 1
//#include <vulkan/vulkan.hpp>

#include <volk/volk.h>

//#include <glslang/Public/ResourceLimits.h>
//#include <glslang/Public/ShaderLang.h>
//#include <glslang/SPIRV/GlslangToSpv.h>

#define GLFW_INCLUDE_NONE
//#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
//#if defined(_WIN32)
//#	define GLFW_EXPOSE_NATIVE_WIN32
//#endif
//#include <GLFW/glfw3native.h>

#if defined(_WIN32)
#	include <ShellScalingApi.h>
#endif

/*
Left handed
	Y   Z
	|  /
	| /
	|/___X
*/
//#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_INLINE
#define GLM_ENABLE_EXPERIMENTAL
//#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Depth buffer range, OpenGL default -1.0 to 1.0, but Vulkan default as 0.0 to 1.0
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/normal.hpp>

#include <stb/stb_image.h>
#include <stb/stb_image_resize.h>
#include <stb/stb_truetype.h>

#include <gli/gli.hpp>
#include <gli/target.hpp>
#include <gli/format.hpp>

#include <tiny_obj_loader.h>

#include <cgltf.h>

#include <utf8.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>

#include <xxhash.h>

#undef near
#undef far

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion