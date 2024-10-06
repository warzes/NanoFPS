#pragma once

#if defined(_MSC_VER)
#	pragma warning(push, 3)
//#	pragma warning(disable : 4191)
#endif

#include <memory>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <unordered_set>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <volk/volk.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif