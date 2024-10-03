#pragma once

constexpr auto NO_TEX = -1;
constexpr auto NO_MODEL = -1;

typedef int TexID;
typedef int ModelID;

struct Tile final
{
	Tile() = default;
	Tile(ModelID s, int a, TexID t, int p) : shape(s), angle(a), texture(t), pitch(p) {}

	operator bool() const
	{
		return shape > NO_MODEL && texture > NO_TEX;
	}

	ModelID shape = NO_MODEL;
	int     angle = 0.0f; // Yaw in whole number of degrees
	TexID   texture = NO_TEX;
	int     pitch = 0;    // Pitch in whole number of degrees
};

inline bool operator==(const Tile& lhs, const Tile& rhs)
{
	return (lhs.shape == rhs.shape) && (lhs.texture == rhs.texture) && (lhs.angle == rhs.angle) && (lhs.pitch == rhs.pitch);
}

inline bool operator!=(const Tile& lhs, const Tile& rhs)
{
	return !(lhs == rhs);
}