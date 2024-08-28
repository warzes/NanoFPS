#pragma once

#pragma region Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4625)
#	pragma warning(disable : 4626)
#	pragma warning(disable : 4820)
#	pragma warning(push, 3)
#	pragma warning(disable : 5039)
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

#include <chrono>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>
#include <bitset>
#include <span>
#include <array>
#include <deque>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>

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
//#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
//#include <glm/gtx/normal.hpp>

#include <pcg32/pcg32.h>

#if defined(_WIN32)
#	include <Windows.h>
#endif

#undef near
#undef far

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion

#pragma region Base Macros

#if defined(_MSC_VER)
#	define PRAGMA(X) __pragma(X)
#else
#	define PRAGMA(X) _Pragma(#X)
#endif
#define HLSL_PACK_BEGIN() PRAGMA(pack(push, 1))
#define HLSL_PACK_END()   PRAGMA(pack(pop))

#define NE_STRINGIFY_(x)   #x
#define NE_STRINGIFY(x)    NE_STRINGIFY_(x)
#define NE_LINE            NE_STRINGIFY(__LINE__)
#define NE_SOURCE_LOCATION __FUNCTION__ << " @ " __FILE__ ":" NE_LINE
#define NE_VAR_VALUE(var)  #var << ":" << var
#define NE_ENDL            std::endl

#pragma endregion
