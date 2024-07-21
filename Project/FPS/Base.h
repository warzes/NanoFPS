#pragma once

#pragma region Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#define _USE_MATH_DEFINES

#include <chrono>
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <bitset>
#include <span>
#include <array>
#include <deque>

#include <volk/volk.h>
#include <VkBootstrap/VkBootstrap.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

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
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_SILENT_WARNINGS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Depth buffer range, OpenGL default -1.0 to 1.0, but Vulkan default as 0.0 to 1.0
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/hash.hpp>
//#include <glm/gtx/rotate_vector.hpp>
//#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
//#include <glm/gtx/transform.hpp>
//#include <glm/gtx/matrix_decompose.hpp>
//#include <glm/gtx/normal.hpp>

#include <stb/stb_image.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

#pragma endregion