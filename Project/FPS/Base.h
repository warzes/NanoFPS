﻿#pragma once

#pragma region Header

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 4191)
#endif

#define _USE_MATH_DEFINES
#define _CRT_SECURE_NO_WARNINGS// TODO: потом удалить когда удалю vulkan.hpp

#include <chrono>
#include <string>
#include <string_view>
#include <filesystem>
#include <fstream>
#include <span>
#include <deque>


#include <volk/volk.h>
#include <VkBootstrap/VkBootstrap.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.hpp> // TODO: потом удалить

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
#include <glm/glm.hpp>
//#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtc/type_ptr.hpp>
//#include <glm/gtx/hash.hpp>
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