#pragma once

#define SMALL_NUMBER       (1.e-8f)
#define KINDA_SMALL_NUMBER (1.e-4f)
#define BIG_NUMBER         (3.4e+38f)
#define EULERS_NUMBER      (2.71828182845904523536f)
#define MAX_FLT            (3.402823466e+38F)
#define INV_PI             (0.31830988618f)
#define HALF_PI            (1.57079632679f)
#define DELTA              (0.00001f)

#pragma region Core Math Func

template< class T >
[[nodiscard]] constexpr T Square(const T a)
{
	return a * a;
}

template< class T >
[[nodiscard]] constexpr T Max3(const T a, const T b, const T c)
{
	return glm::max(glm::max(a, b), c);
}

template< class T >
[[nodiscard]] constexpr T Min3(const T a, const T b, const T c)
{
	return glm::min(glm::min(a, b), c);
}

template< class T, class U >
[[nodiscard]] constexpr T Lerp(const T& a, const T& b, const U& alpha)
{
	return (T)(a + alpha * (b - a));
}

[[nodiscard]] inline float Fmod(float x, float y)
{
	if (fabsf(y) <= 1.e-8f) {
		return 0.f;
	}

	const float quotient = (float)(int32_t)(x / y);
	float intPortion = y * quotient;

	if (fabsf(intPortion) > fabsf(x)) {
		intPortion = x;
	}

	const float result = x - intPortion;
	return result;
}

[[nodiscard]] constexpr size_t RoundUp(size_t value, size_t multiple)
{
	assert(multiple && ((multiple & (multiple - 1)) == 0));
	return (value + multiple - 1) & ~(multiple - 1);
}

[[nodiscard]] inline int NumMipmap(int width, int height)
{
	return static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
}

#pragma endregion

#pragma region Color

class Color;

enum class GammaSpace
{
	Linear,
	Pow22,
	sRGB,
};

class LinearColor final
{
public:
	static const LinearColor White;
	static const LinearColor Gray;
	static const LinearColor Black;
	static const LinearColor Transparent;
	static const LinearColor Red;
	static const LinearColor Green;
	static const LinearColor Blue;
	static const LinearColor Yellow;

	static double pow22OneOver255Table[256];
	static double sRGBToLinearTable[256];

	LinearColor() = default;
	explicit LinearColor(const Color& Color);
	constexpr LinearColor(float inR, float inG, float inB, float inA = 1.0f)
		: r(inR)
		, g(inG)
		, b(inB)
		, a(inA)
	{
	}

	Color ToRGBE() const;
	LinearColor LinearRGBToHSV() const;
	LinearColor HSVToLinearRGB() const;
	Color ToFColor(const bool sRGB) const;
	LinearColor Desaturate(float desaturation) const;

	float& Component(uint32_t index)
	{
		return (&r)[index];
	}

	const float& Component(uint32_t index) const
	{
		return (&r)[index];
	}

	LinearColor operator+(const LinearColor& rhs) const
	{
		return LinearColor(
			this->r + rhs.r,
			this->g + rhs.g,
			this->b + rhs.b,
			this->a + rhs.a
		);
	}

	LinearColor& operator+=(const LinearColor& rhs)
	{
		r += rhs.r;
		g += rhs.g;
		b += rhs.b;
		a += rhs.a;

		return *this;
	}

	LinearColor operator-(const LinearColor& rhs) const
	{
		return LinearColor(
			this->r - rhs.r,
			this->g - rhs.g,
			this->b - rhs.b,
			this->a - rhs.a
		);
	}

	LinearColor& operator-=(const LinearColor& rhs)
	{
		r -= rhs.r;
		g -= rhs.g;
		b -= rhs.b;
		a -= rhs.a;

		return *this;
	}

	LinearColor operator*(const LinearColor& rhs) const
	{
		return LinearColor(
			this->r * rhs.r,
			this->g * rhs.g,
			this->b * rhs.b,
			this->a * rhs.a
		);
	}

	LinearColor& operator*=(const LinearColor& rhs)
	{
		r *= rhs.r;
		g *= rhs.g;
		b *= rhs.b;
		a *= rhs.a;

		return *this;
	}

	LinearColor operator*(float scalar) const
	{
		return LinearColor(
			this->r * scalar,
			this->g * scalar,
			this->b * scalar,
			this->a * scalar
		);
	}

	LinearColor& operator*=(float scalar)
	{
		r *= scalar;
		g *= scalar;
		b *= scalar;
		a *= scalar;

		return *this;
	}

	LinearColor operator/(const LinearColor& rhs) const
	{
		return LinearColor(
			this->r / rhs.r,
			this->g / rhs.g,
			this->b / rhs.b,
			this->a / rhs.a
		);
	}

	LinearColor& operator/=(const LinearColor& rhs)
	{
		r /= rhs.r;
		g /= rhs.g;
		b /= rhs.b;
		a /= rhs.a;

		return *this;
	}

	LinearColor operator/(float scalar) const
	{
		const float invScalar = 1.0f / scalar;

		return LinearColor(
			this->r * invScalar,
			this->g * invScalar,
			this->b * invScalar,
			this->a * invScalar
		);
	}

	LinearColor& operator/=(float scalar)
	{
		const float invScalar = 1.0f / scalar;

		r *= invScalar;
		g *= invScalar;
		b *= invScalar;
		a *= invScalar;

		return *this;
	}

	LinearColor GetClamped(float inMin = 0.0f, float inMax = 1.0f) const
	{
		LinearColor ret;
		ret.r = glm::clamp(r, inMin, inMax);
		ret.g = glm::clamp(g, inMin, inMax);
		ret.b = glm::clamp(b, inMin, inMax);
		ret.a = glm::clamp(a, inMin, inMax);

		return ret;
	}

	bool operator==(const LinearColor& other) const
	{
		return this->r == other.r && this->g == other.g && this->b == other.b && this->a == other.a;
	}

	bool operator!=(const LinearColor& Other) const
	{
		return this->r != Other.r || this->g != Other.g || this->b != Other.b || this->a != Other.a;
	}

	bool Equals(const LinearColor& other, float tolerance = KINDA_SMALL_NUMBER) const
	{
		return glm::abs(this->r - other.r) < tolerance && glm::abs(this->g - other.g) < tolerance && glm::abs(this->b - other.b) < tolerance && glm::abs(this->a - other.a) < tolerance;
	}

	LinearColor CopyWithNewOpacity(float newOpacicty) const
	{
		LinearColor newCopy = *this;
		newCopy.a = newOpacicty;
		return newCopy;
	}

	float ComputeLuminance() const
	{
		return r * 0.3f + g * 0.59f + b * 0.11f;
	}

	float GetMax() const
	{
		return glm::max(glm::max(glm::max(r, g), b), a);
	}

	bool IsAlmostBlack() const
	{
		return Square(r) < DELTA && Square(g) < DELTA && Square(b) < DELTA;
	}

	float GetMin() const
	{
		return glm::min(glm::min(glm::min(r, g), b), a);
	}

	float GetLuminance() const
	{
		return r * 0.3f + g * 0.59f + b * 0.11f;
	}

	static float Dist(const LinearColor& v1, const LinearColor& v2)
	{
		return sqrtf(Square(v2.r - v1.r) + Square(v2.g - v1.g) + Square(v2.b - v1.b) + Square(v2.a - v1.a));
	}

	static LinearColor GetHSV(uint8_t h, uint8_t s, uint8_t v);

	static LinearColor MakeFromColorTemperature(float temp);

	static LinearColor FromSRGBColor(const Color& Color);

	static LinearColor FromPow22Color(const Color& Color);

	static LinearColor LerpUsingHSV(const LinearColor& from, const LinearColor& to, const float progress);

	float r = 0.0f;
	float g = 0.0f;
	float b = 0.0f;
	float a = 0.0f;
};

class Color final
{
public:
	static const Color White;
	static const Color Black;
	static const Color Transparent;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Orange;
	static const Color Purple;
	static const Color Turquoise;
	static const Color Silver;
	static const Color Emerald;

	Color() = default;
	constexpr Color(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inA = 255)
		: b(inB)
		, g(inG)
		, r(inR)
		, a(inA)
	{
	}

	explicit Color(uint32_t inColor)
	{
		DWColor() = inColor;
	}

	uint32_t& DWColor()
	{
		return *((uint32_t*)this);
	}

	const uint32_t& DWColor() const
	{
		return *((uint32_t*)this);
	}

	bool operator==(const Color& C) const
	{
		return DWColor() == C.DWColor();
	}

	bool operator!=(const Color& C) const
	{
		return DWColor() != C.DWColor();
	}

	void operator+=(const Color& C)
	{
		r = (uint8_t)glm::min((int32_t)r + (int32_t)C.r, 255);
		g = (uint8_t)glm::min((int32_t)g + (int32_t)C.g, 255);
		b = (uint8_t)glm::min((int32_t)b + (int32_t)C.b, 255);
		a = (uint8_t)glm::min((int32_t)a + (int32_t)C.a, 255);
	}

	Color WithAlpha(uint8_t alpha) const
	{
		return Color(r, g, b, alpha);
	}

	LinearColor ReinterpretAsLinear() const
	{
		return LinearColor(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
	}

	uint32_t ToPackedARGB() const
	{
		return (a << 24) | (r << 16) | (g << 8) | (b << 0);
	}

	uint32_t ToPackedABGR() const
	{
		return (a << 24) | (b << 16) | (g << 8) | (r << 0);
	}

	uint32_t ToPackedRGBA() const
	{
		return (r << 24) | (g << 16) | (b << 8) | (a << 0);
	}

	uint32_t ToPackedBGRA() const
	{
		return (b << 24) | (g << 16) | (r << 8) | (a << 0);
	}

	LinearColor FromRGBE() const;

	static Color MakeRedToGreenColorFromScalar(float scalar);
	static Color MakeFromColorTemperature(float temp);

	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t a;

private:
	explicit Color(const LinearColor& linearColor);
};

inline LinearColor operator*(float scalar, const LinearColor& Color)
{
	return Color.operator*(scalar);
}

#pragma endregion

class AABB final
{
public:
	AABB() = default;
	AABB(const glm::vec3& inMin, const glm::vec3& inMax);
	AABB(const std::vector<glm::vec3>& points);

	[[nodiscard]] float GetVolume() const;

	[[nodiscard]] glm::vec3 GetCenter() const;
	[[nodiscard]] glm::vec3 GetHalfSize() const;
	[[nodiscard]] glm::vec3 GetDiagonal() const;

	[[nodiscard]] float GetSurfaceArea() const;

	[[nodiscard]] void Combine(const AABB& anotherAABB);
	[[nodiscard]] void Combine(const glm::vec3& point);
	[[nodiscard]] bool Overlaps(const AABB& anotherAABB);
	[[nodiscard]] bool Inside(const glm::vec3& point);

	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

#pragma region inline AABB

inline AABB::AABB(const glm::vec3& inMin, const glm::vec3& inMax)
	: min(inMin), max(inMax)
{
}

inline AABB::AABB(const std::vector<glm::vec3>& points)
{
	for (size_t i = 0; i < points.size(); i++)
		Combine(points[i]);
}

inline float AABB::GetVolume() const
{
	glm::vec3 diagonal = max - min;
	return diagonal.x * diagonal.y * diagonal.z;
}

inline glm::vec3 AABB::GetCenter() const
{
	return (min + max) * 0.5f;
}

inline glm::vec3 AABB::GetHalfSize() const
{
	return (max - min) * 0.5f;
}

inline glm::vec3 AABB::GetDiagonal() const
{
	return max - min;
}

inline float AABB::GetSurfaceArea() const
{
	glm::vec3 diagonal = max - min;
	return 2.0f * (diagonal.x * diagonal.y + diagonal.y * diagonal.z + diagonal.z * diagonal.x);
}

inline void AABB::Combine(const AABB& anotherAABB)
{
	min = glm::min(min, anotherAABB.min);
	max = glm::max(max, anotherAABB.max);
}

inline void AABB::Combine(const glm::vec3& point)
{
	min = glm::min(min, point);
	max = glm::max(max, point);
}

inline bool AABB::Overlaps(const AABB& anotherAABB)
{
	return max.x > anotherAABB.min.x && min.x < anotherAABB.max.x
		&& max.y > anotherAABB.min.y && min.y < anotherAABB.max.y
		&& max.z > anotherAABB.min.z && min.z < anotherAABB.max.z;
}

inline bool AABB::Inside(const glm::vec3& point)
{
	return max.x > point.x && min.x < point.x
		&& max.y > point.y && min.y < point.y
		&& max.z > point.z && min.z < point.x;
}

#pragma endregion

class OBB final
{
public:
	glm::mat4 tm = glm::mat4(1.0f);
	glm::vec3 halfExtents = glm::vec3(0.0f);
};

class Plane final
{
public:
	// The form is ax + by + cz + d = 0
	// where: d = dot(n, p)
	glm::vec3 n = glm::vec3(0.0f);
	float d = 0.0f;
};

class Frustum final
{
public:
	Plane planes[6] = {};
};

class Sphere final
{
public:
	Sphere() = default;

	[[nodiscard]] float GetVolume() const;

	[[nodiscard]] bool Inside(const glm::vec3& point);

	glm::vec3 center = glm::vec3(0.0f);
	float radius = 0.0f;
};

#pragma region inline Sphere

inline float Sphere::GetVolume() const
{
	return(4.0f / 3.0f * glm::pi<float>()) * (radius * radius * radius);
}

inline bool Sphere::Inside(const glm::vec3& point)
{
	float dist = glm::length2(point - center);
	return dist < radius * radius;
}

#pragma endregion

class Transform final
{
public:
	Transform() = default;

	Transform& SetPosition(const glm::vec3& position);
	Transform& SetEulerAngles(const glm::vec3& eulerAngles);
	Transform& Translate(const glm::vec3& translation);
	Transform& RotateX(const float radians);
	Transform& RotateY(const float radians);
	Transform& RotateZ(const float radians);

	Transform& ClampPitch();

	[[nodiscard]] const glm::vec3& GetPosition() const { return m_position; }

	[[nodiscard]] const glm::vec3& GetEulerAngles() const { return m_eulerAngles; }

	[[nodiscard]] const glm::mat4& GetTranslationMatrix() const;
	[[nodiscard]] const glm::mat4& GetRotationMatrix() const;

	[[nodiscard]] glm::vec3 GetRightVector() const;
	[[nodiscard]] glm::vec3 GetUpVector() const;
	[[nodiscard]] glm::vec3 GetForwardVector() const;
	[[nodiscard]] glm::vec3 GetHorizontalRightVector() const;
	[[nodiscard]] glm::vec3 GetHorizontalForwardVector() const;

	[[nodiscard]] glm::mat4 GetMatrix() const { return GetTranslationMatrix() * GetRotationMatrix(); }
	[[nodiscard]] glm::mat4 GetInverseMatrix() const { return inverse(GetMatrix()); }

private:
	float clampPitch(const float radians) { return glm::clamp(radians, -glm::half_pi<float>(), glm::half_pi<float>()); }
	// Wrap to (-PI..PI]
	float wrapAngle(const float radians) { return std::remainder(radians, glm::two_pi<float>()); }

	glm::vec3 m_position = glm::vec3(0.0f);
	glm::vec3 m_eulerAngles = glm::vec3(0.0f);

	mutable glm::mat4 m_translationMatrix{ 1.0f };
	mutable glm::mat4 m_rotationMatrix{ 1.0f };

	mutable bool m_translationDirty = true;
	mutable bool m_rotationDirty = true;
};

#pragma region inline Transform

inline Transform& Transform::SetPosition(const glm::vec3& position)
{
	m_translationDirty = true;
	m_position = position;
	return *this;
}

inline Transform& Transform::SetEulerAngles(const glm::vec3& eulerAngles)
{
	m_rotationDirty = true;
	m_eulerAngles.x = wrapAngle(eulerAngles.x);
	m_eulerAngles.y = wrapAngle(eulerAngles.y);
	m_eulerAngles.z = wrapAngle(eulerAngles.z);
	return *this;
}

inline Transform& Transform::Translate(const glm::vec3& translation)
{
	m_translationDirty = true;
	m_position += translation;
	return *this;
}

inline Transform& Transform::RotateX(const float radians)
{
	m_rotationDirty = true;
	m_eulerAngles.x = wrapAngle(m_eulerAngles.x + radians);
	return *this;
}

inline Transform& Transform::RotateY(const float radians)
{
	m_rotationDirty = true;
	m_eulerAngles.y = wrapAngle(m_eulerAngles.y + radians);
	return *this;
}

inline Transform& Transform::RotateZ(const float radians)
{
	m_rotationDirty = true;
	m_eulerAngles.z = wrapAngle(m_eulerAngles.z + radians);
	return *this;
}

inline Transform& Transform::ClampPitch()
{
	m_rotationDirty = true;
	m_eulerAngles.x = clampPitch(m_eulerAngles.x);
	return *this;
}

inline const glm::mat4& Transform::GetTranslationMatrix() const
{
	if (m_translationDirty)
	{
		static constexpr glm::mat4 IDENTITY(1.0f);
		m_translationMatrix = translate(IDENTITY, m_position);
		m_translationDirty = false;
	}
	return m_translationMatrix;
}

inline const glm::mat4& Transform::GetRotationMatrix() const
{
	if (m_rotationDirty)
	{
		m_rotationMatrix = glm::eulerAngleYXZ(m_eulerAngles.y, m_eulerAngles.x, m_eulerAngles.z);
		m_rotationDirty = false;
	}
	return m_rotationMatrix;
}

inline glm::vec3 Transform::GetRightVector() const
{
	static constexpr glm::vec4 RIGHT{ 1, 0, 0, 0 };
	return GetRotationMatrix() * RIGHT;
}

inline glm::vec3 Transform::GetUpVector() const
{
	static constexpr glm::vec4 UP{ 0, 1, 0, 0 };
	return GetRotationMatrix() * UP;
}

inline glm::vec3 Transform::GetForwardVector() const
{
	static constexpr glm::vec4 FORWARD{ 0, 0, 1, 0 };
	return GetRotationMatrix() * FORWARD;
}

inline glm::vec3 Transform::GetHorizontalRightVector() const
{
	const float yaw = m_eulerAngles.y;
	return { glm::cos(yaw), 0, -glm::sin(yaw) };
}

inline glm::vec3 Transform::GetHorizontalForwardVector() const
{
	const float yaw = m_eulerAngles.y;
	return { glm::sin(yaw), 0, glm::cos(yaw) };
}

#pragma endregion

class ShadowMatrixCalculator final
{
public:
	void SetCameraInfo(const glm::mat4& view, float fov, float aspectRatio);
	void SetLightDirection(const glm::vec3& lightDir);
	void SetWorldBounds(const glm::vec3& min, const glm::vec3& max);
	[[nodiscard]] glm::mat4 CalcShadowMatrix(float near, float far) const;

private:
	[[nodiscard]] std::array<glm::vec4, 8> getFrustumCorners(float near, float far) const;

	[[nodiscard]] glm::mat4 getLightProjection(const std::array<glm::vec4, 8>& frustumCorners, const glm::mat4& lightView) const;

	glm::mat4 m_view{ 1.0f };
	float     m_fov = 0.0f;
	float     m_aspectRatio = 1.0f;

	glm::vec3 m_lightDir{};

	std::array<glm::vec4, 8> m_worldCorners{};
};
