#pragma once

#pragma region Core Func

template <typename T>
[[nodiscard]] constexpr T RoundUp(T value, T multiple)
{
	static_assert(std::is_integral<T>::value, "T must be an integral type");
	assert(multiple && ((multiple & (multiple - 1)) == 0));
	return (value + multiple - 1) & ~(multiple - 1);
}

template <typename T>
[[nodiscard]] T* DataPtr(std::vector<T>& container)
{
	T* ptr = container.empty() ? nullptr : container.data();
	return ptr;
}

template <typename T>
[[nodiscard]] uint32_t CountU32(const std::vector<T>& container)
{
	uint32_t n = static_cast<uint32_t>(container.size());
	return n;
}

template <typename T>
[[nodiscard]] const T* DataPtr(const std::vector<T>& container)
{
	const T* ptr = container.empty() ? nullptr : container.data();
	return ptr;
}

template <typename T>
[[nodiscard]] uint32_t SizeInBytesU32(const std::vector<T>& container)
{
	uint32_t size = static_cast<uint32_t>(container.size() * sizeof(T));
	return size;
}

template <typename T>
[[nodiscard]] uint64_t SizeInBytesU64(const std::vector<T>& container)
{
	uint64_t size = static_cast<uint64_t>(container.size() * sizeof(T));
	return size;
}

template <typename T>
[[nodiscard]] bool IsIndexInRange(uint32_t index, const std::vector<T>& container)
{
	uint32_t n = CountU32(container);
	bool     res = (index < n);
	return res;
}

template <typename T>
typename std::vector<T>::iterator Find(std::vector<T>& container, const T& element)
{
	typename std::vector<T>::iterator it = std::find(
		std::begin(container),
		std::end(container),
		element);
	return it;
}

template <typename T>
typename std::vector<T>::const_iterator Find(const std::vector<T>& container, const T& element)
{
	typename std::vector<T>::iterator it = std::find(
		std::begin(container),
		std::end(container),
		element);
	return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::iterator FindIf(std::vector<T>& container, UnaryPredicate predicate)
{
	typename std::vector<T>::iterator it = std::find_if(
		std::begin(container),
		std::end(container),
		predicate);
	return it;
}

template <typename T, class UnaryPredicate>
typename std::vector<T>::const_iterator FindIf(const std::vector<T>& container, UnaryPredicate predicate)
{
	typename std::vector<T>::const_iterator it = std::find_if(
		std::begin(container),
		std::end(container),
		predicate);
	return it;
}

template <typename T>
bool ElementExists(const T& elem, const std::vector<T>& container)
{
	auto it = std::find(std::begin(container), std::end(container), elem);
	bool exists = (it != std::end(container));
	return exists;
}

template <typename T>
bool GetElement(uint32_t index, const std::vector<T>& container, T* pElem)
{
	if (!IsIndexInRange(index, container)) {
		return false;
	}
	*pElem = container[index];
	return true;
}

template <typename T>
void AppendElements(const std::vector<T>& elements, std::vector<T>& container)
{
	if (!elements.empty())
	{
		std::copy(
			std::begin(elements),
			std::end(elements),
			std::back_inserter(container));
	}
}

template <typename T>
void RemoveElement(const T& elem, std::vector<T>& container)
{
	container.erase(
		std::remove(std::begin(container), std::end(container), elem),
		container.end());
}

template <typename T, class UnaryPredicate>
void RemoveElementIf(std::vector<T>& container, UnaryPredicate predicate)
{
	container.erase(
		std::remove_if(std::begin(container), std::end(container), predicate),
		container.end());
}

template <typename T>
void Unique(std::vector<T>& container)
{
	auto it = std::unique(std::begin(container), std::end(container));
	container.resize(std::distance(std::begin(container), it));
}

inline std::string FloatString(float value, int precision = 6)
{
	std::stringstream ss;
	ss << std::setprecision(precision) << std::setw(6) << std::fixed << value;
	return ss.str();
}

template <typename T>
T InvalidValue(const T value = static_cast<T>(~0))
{
	return value;
}

#pragma endregion

#pragma region Time

class Time final
{
public:
	constexpr Time() = default;
	template <typename Rep, typename Period>
	constexpr Time(const std::chrono::duration<Rep, Period>& duration);

	[[nodiscard]] constexpr float AsSeconds() const;
	[[nodiscard]] constexpr int32_t AsMilliseconds() const;
	[[nodiscard]] constexpr int64_t AsMicroseconds() const;

	[[nodiscard]] constexpr std::chrono::microseconds ToDuration() const;
	template <typename Rep, typename Period>
	constexpr operator std::chrono::duration<Rep, Period>() const;

private:
	std::chrono::microseconds m_microseconds{};
};

#pragma region inline Time 

template<typename Rep, typename Period>
inline constexpr Time::Time(const std::chrono::duration<Rep, Period>& duration) : m_microseconds(duration)
{
}

inline constexpr float Time::AsSeconds() const
{
	return std::chrono::duration<float>(m_microseconds).count();
}

inline constexpr int32_t Time::AsMilliseconds() const
{
	return std::chrono::duration_cast<std::chrono::duration<std::int32_t, std::milli>>(m_microseconds).count();
}

inline constexpr int64_t Time::AsMicroseconds() const
{
	return m_microseconds.count();
}

inline constexpr std::chrono::microseconds Time::ToDuration() const
{
	return m_microseconds;
}

template<typename Rep, typename Period>
inline constexpr Time::operator std::chrono::duration<Rep, Period>() const
{
	return m_microseconds;
}

#pragma endregion

class Clock final
{
public:
	[[nodiscard]] Time GetElapsedTime() const;

	[[nodiscard]] bool IsRunning() const;

	void Start();
	void Stop();
	Time Restart();
	Time Reset();
private:
	using ClockImpl = std::conditional_t<std::chrono::high_resolution_clock::is_steady, std::chrono::high_resolution_clock, std::chrono::steady_clock>;

	ClockImpl::time_point m_refPoint{ ClockImpl::now() };
	ClockImpl::time_point m_stopPoint;
};

#pragma endregion

#pragma region IO

class File final
{
public:
	File() = default;
	File(const File&) = delete;
	File(const File&&) = delete;
	~File();

	bool Open(const std::filesystem::path& path);
	bool IsValid() const;
	size_t Read(void* buffer, size_t count);

	size_t GetLength() const;

private:
	std::ifstream m_stream;
	size_t        m_fileSize = 0;
	size_t        m_fileOffset = 0;
};

class FileStream : public std::streambuf
{
public:
	bool Open(const char* path);

private:
	std::vector<char> m_buffer;
};

std::optional<std::vector<char>> LoadFile(const std::filesystem::path& path);

[[nodiscard]] std::optional<std::string> LoadSourceFile(const std::filesystem::path& path);


#pragma endregion

#pragma region Log

void Print(const std::string& msg);
void Warning(const std::string& msg);
void Error(const std::string& msg);
void Fatal(const std::string& msg);

#pragma endregion

#pragma region Core Math

#define SMALL_NUMBER       (1.e-8f)
#define KINDA_SMALL_NUMBER (1.e-4f)
#define BIG_NUMBER         (3.4e+38f)
#define EULERS_NUMBER      (2.71828182845904523536f)
#define MAX_FLT            (3.402823466e+38F)
#define INV_PI             (0.31830988618f)
#define HALF_PI            (1.57079632679f)
#define DELTA              (0.00001f)

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

#pragma region Math Geometry

class OBB;

class AABB final
{
public:
	AABB() = default;
	AABB(const glm::vec3& pos)
	{
		Set(pos);
	}
	AABB(const glm::vec3& minPos, const glm::vec3& maxPos)
	{
		Set(minPos, maxPos);
	}
	AABB(const OBB& obb);
	AABB(const std::vector<glm::vec3>& points)
	{
		for (size_t i = 0; i < points.size(); i++)
			Combine(points[i]);
	}

	AABB& operator=(const OBB& rhs);

	void Set(const glm::vec3& pos)
	{
		min = max = pos;
	}

	void Set(const glm::vec3& minPos, const glm::vec3& maxPos)
	{
		min = glm::min(minPos, maxPos);
		max = glm::max(minPos, maxPos);
	}

	void Set(const OBB& obb);

	void Combine(const AABB& anotherAABB)
	{
		min = glm::min(min, anotherAABB.min);
		max = glm::max(max, anotherAABB.max);
	}
	void Combine(const glm::vec3& point)
	{
		min = glm::min(min, point);
		max = glm::max(max, point);
	}

	void Expand(const glm::vec3& pos)
	{
		min = glm::min(pos, min);
		max = glm::max(pos, max);
	}

	[[nodiscard]] float GetVolume() const
	{
		glm::vec3 diagonal = max - min;
		return diagonal.x * diagonal.y * diagonal.z;
	}

	[[nodiscard]] glm::vec3 GetCenter() const
	{
		glm::vec3 center = (min + max) / 2.0f;
		return center;
	}

	[[nodiscard]] glm::vec3 GetSize() const
	{
		glm::vec3 size = (max - min);
		return size;
	}

	[[nodiscard]] glm::vec3 GetHalfSize() const
	{
		return GetSize() * 0.5f;
	}

	[[nodiscard]] float GetWidth() const
	{
		return (max.x - min.x);
	}

	[[nodiscard]] float GetHeight() const
	{
		return (max.y - min.y);
	}

	[[nodiscard]] float GetDepth() const
	{
		return (max.y - min.y);
	}

	[[nodiscard]] glm::vec3 GetU() const
	{
		glm::vec3 P = glm::vec3(max.x, min.y, min.z);
		glm::vec3 U = glm::normalize(P - min);
		return U;
	}

	[[nodiscard]] glm::vec3 GetV() const
	{
		glm::vec3 P = glm::vec3(min.x, max.y, min.z);
		glm::vec3 V = glm::normalize(P - min);
		return V;
	}

	[[nodiscard]] glm::vec3 GetW() const
	{
		glm::vec3 P = glm::vec3(min.x, min.y, max.z);
		glm::vec3 W = glm::normalize(P - min);
		return W;
	}

	void Transform(const glm::mat4x4& matrix, glm::vec3 obbVertices[8]) const;

	[[nodiscard]] bool Overlaps(const AABB& anotherAABB) const
	{
		return max.x > anotherAABB.min.x && min.x < anotherAABB.max.x
			&& max.y > anotherAABB.min.y && min.y < anotherAABB.max.y
			&& max.z > anotherAABB.min.z && min.z < anotherAABB.max.z;
	}
	[[nodiscard]] bool Inside(const glm::vec3& point) const
	{
		return max.x > point.x && min.x < point.x
			&& max.y > point.y && min.y < point.y
			&& max.z > point.z && min.z < point.x;
	}

	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
};

class OBB final
{
public:
	OBB() = default;
	OBB(const glm::vec3& center, const glm::vec3& size, const glm::vec3& U, const glm::vec3& V, const glm::vec3& W)
		: m_center(center)
		, m_size(size)
		, m_U(glm::normalize(U))
		, m_V(glm::normalize(V))
		, m_W(glm::normalize(W)) {}
	OBB(const AABB& aabb);

	void Set(const AABB& aabb);

	const glm::vec3& GetPos() const
	{
		return m_center;
	}

	const glm::vec3& GetSize() const
	{
		return m_size;
	}

	const glm::vec3& GetU() const
	{
		return m_U;
	}

	const glm::vec3& GetV() const
	{
		return m_V;
	}

	const glm::vec3& GetW() const
	{
		return m_W;
	}

	void GetPoints(glm::vec3 obbVertices[8]) const;

private:
	glm::vec3 m_center = glm::vec3(0, 0, 0);
	glm::vec3 m_size = glm::vec3(0, 0, 0);
	glm::vec3 m_U = glm::vec3(1, 0, 0);
	glm::vec3 m_V = glm::vec3(0, 1, 0);
	glm::vec3 m_W = glm::vec3(0, 0, 1);
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

	[[nodiscard]] float GetVolume() const
	{
		return(4.0f / 3.0f * glm::pi<float>()) * (radius * radius * radius);
	}

	[[nodiscard]] bool Inside(const glm::vec3& point)
	{
		float dist = glm::length2(point - center);
		return dist < radius * radius;
	}

	glm::vec3 center = glm::vec3(0.0f);
	float radius = 0.0f;
};

class Transform final
{
public:
	enum class RotationOrder
	{
		XYZ,
		XZY,
		YZX,
		YXZ,
		ZXY,
		ZYX,
	};

	Transform() = default;
	Transform(const glm::vec3& translation);

	bool IsDirty() const { return (m_dirty.mask != 0); }

	const glm::vec3& GetTranslation() const { return m_translation; }
	const glm::vec3& GetRotation() const { return m_rotation; }
	const glm::vec3& GetScale() const { return m_scale; }
	RotationOrder GetRotationOrder() const { return m_rotationOrder; }

	void SetTranslation(const glm::vec3& value);
	void SetTranslation(float x, float y, float z);
	void SetRotation(const glm::vec3& value);
	void SetRotation(float x, float y, float z);
	void SetScale(const glm::vec3& value);
	void SetScale(float x, float y, float z);
	void SetRotationOrder(Transform::RotationOrder value);

	const glm::mat4x4& GetTranslationMatrix() const;
	const glm::mat4x4& GetRotationMatrix() const;
	const glm::mat4x4& GetScaleMatrix() const;
	const glm::mat4x4& GetConcatenatedMatrix() const;

protected:
	mutable struct
	{
		union
		{
			struct
			{
				bool translation : 1;
				bool rotation : 1;
				bool scale : 1;
				bool concatenated : 1;
			};
			uint32_t mask = 0xF;
		};
	} m_dirty;

	glm::vec3           m_translation = glm::vec3(0, 0, 0);
	glm::vec3           m_rotation = glm::vec3(0, 0, 0);
	glm::vec3           m_scale = glm::vec3(1, 1, 1);
	RotationOrder       m_rotationOrder = RotationOrder::XYZ;
	mutable glm::mat4x4 m_translationMatrix;
	mutable glm::mat4x4 m_rotationMatrix;
	mutable glm::mat4x4 m_scaleMatrix;
	mutable glm::mat4x4 m_concatenatedMatrix;
};

#pragma endregion
