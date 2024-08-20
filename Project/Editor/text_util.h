#pragma once

#include "raylib.h"
#include "math_stuff.h"

#include <string>
#include <initializer_list>
#include <filesystem>
namespace fs = std::filesystem;

inline std::string BuildPath(std::initializer_list<std::string> components) {
	std::string output;
	for (const std::string& s : components) {
		output += s;
		if (s != *(components.end() - 1) && s[s.length() - 1] != '/') {
			output += '/';
		}
	}
	return output;
}

//Returns the approximate width, in pixels, of a string written in the given font.
//Based off of Raylib's DrawText functions
inline int GetStringWidth(Font font, float fontSize, const std::string& string)
{
	float scaleFactor = fontSize / font.baseSize;
	int maxWidth = 0;
	int lineWidth = 0;
	for (int i = 0; i < int(string.size());)
	{
		int codepointByteCount = 0;
		int codepoint = GetCodepoint(&string[i], &codepointByteCount);
		int g = GetGlyphIndex(font, codepoint);

		if (codepoint == 0x3f) codepointByteCount = 1;

		if (codepoint == '\n')
		{
			maxWidth = Max(lineWidth, maxWidth);
			lineWidth = 0;
		}
		else
		{
			if (font.glyphs[g].advanceX == 0)
				lineWidth += int(font.recs[g].width * scaleFactor);
			else
				lineWidth += int(font.glyphs[g].advanceX * scaleFactor);
		}

		i += codepointByteCount;
	}
	maxWidth = Max(lineWidth, maxWidth);
	return maxWidth;
}

// Returns sections of the input string that are located between occurances of the delimeter.
inline std::vector<std::string> SplitString(const std::string& input, const std::string delimeter)
{
	std::vector<std::string> result;
	size_t tokenStart = 0, tokenEnd = 0;
	while ((tokenEnd = input.find(delimeter, tokenStart)) != std::string::npos)
	{
		result.push_back(input.substr(tokenStart, tokenEnd - tokenStart));
		tokenStart = tokenEnd + delimeter.length();
	}
	// Add the remaining text after the last delimeter
	if (tokenStart < input.length())
		result.push_back(input.substr(tokenStart));
	return result;
}