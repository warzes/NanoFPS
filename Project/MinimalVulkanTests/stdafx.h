#pragma once

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(push, 3)
#	pragma warning(disable : 5039)
#endif

#define _USE_MATH_DEFINES
#include <cmath>

#include <chrono>
#include <array>
#include <filesystem>
#include <fstream>

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <Windows.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif