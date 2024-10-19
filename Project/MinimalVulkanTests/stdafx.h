#pragma once

#if defined(_MSC_VER)
#	pragma warning(push, 3)
//#	pragma warning(disable : 4191)
#endif

#define _USE_MATH_DEFINES
#include <cmath>

#include <chrono>
#include <bitset>
#include <deque>
#include <unordered_map>

#include <memory>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>
#include <set>
#include <unordered_set>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <Windows.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <volk/volk.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif