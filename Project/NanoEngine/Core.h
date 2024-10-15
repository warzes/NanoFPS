#pragma once

//=============================================================================
#pragma region [ Macros ]

#define STRINGIFY_(x)   #x
#define STRINGIFY(x)    STRINGIFY_(x)
#define LINE            STRINGIFY(__LINE__)
#define SOURCE_LOCATION __FUNCTION__ << " @ " __FILE__ ":" LINE
#define VAR_VALUE(var)  #var + std::string(":") + var

#pragma endregion

//=============================================================================
#pragma region [ Constants ]

constexpr auto WHOLE_SIZE = UINT64_MAX;
constexpr auto VALUE_IGNORED = UINT32_MAX;
constexpr auto APPEND_OFFSET_ALIGNED = UINT32_MAX;

#pragma endregion

//=============================================================================
#pragma region [ Base Types ]

// Import GLM types as HLSL friendly names

// bool
using bool2 = glm::bool2;
using bool3 = glm::bool3;
using bool4 = glm::bool4;

// 8-bit signed integer
using i8 = glm::i8;
using i8vec2 = glm::i8vec2;
using i8vec3 = glm::i8vec3;
using i8vec4 = glm::i8vec4;

// 16-bit unsigned integer
using half = glm::u16;
using half2 = glm::u16vec2;
using half3 = glm::u16vec3;
using half4 = glm::u16vec4;

// 32-bit signed integer
using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;
// 32-bit unsigned integer
using uint = glm::uint;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

// 32-bit float
using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
// 32-bit float2 matrices
using float2x2 = glm::mat2x2;
using float2x3 = glm::mat2x3;
using float2x4 = glm::mat2x4;
// 32-bit float3 matrices
using float3x2 = glm::mat3x2;
using float3x3 = glm::mat3x3;
using float3x4 = glm::mat3x4;
// 32-bit float4 matrices
using float4x2 = glm::mat4x2;
using float4x3 = glm::mat4x3;
using float4x4 = glm::mat4x4;
// 32-bit float quaternion
using quat = glm::quat;

// 64-bit float
using double2 = glm::dvec2;
using double3 = glm::dvec3;
using double4 = glm::dvec4;
// 64-bit float2 matrices
using double2x2 = glm::dmat2x2;
using double2x3 = glm::dmat2x3;
using double2x4 = glm::dmat2x4;
// 64-bit float3 matrices
using double3x2 = glm::dmat3x2;
using double3x3 = glm::dmat3x3;
using double3x4 = glm::dmat3x4;
// 64-bit float4 matrices
using double4x2 = glm::dmat4x2;
using double4x3 = glm::dmat4x3;
using double4x4 = glm::dmat4x4;

// Output overloads for common data types.
std::ostream& operator<<(std::ostream& os, const float2& i);
std::ostream& operator<<(std::ostream& os, const float3& i);
std::ostream& operator<<(std::ostream& os, const float4& i);
std::ostream& operator<<(std::ostream& os, const uint3& i);

#if defined(_MSC_VER)
#define PRAGMA(X) __pragma(X)
#else
#define PRAGMA(X) _Pragma(#X)
#endif
#define HLSL_PACK_BEGIN() PRAGMA(pack(push, 1))
#define HLSL_PACK_END()   PRAGMA(pack(pop))

HLSL_PACK_BEGIN();

struct float2x2_aligned
{
	float4 v0;
	float2 v1;

	float2x2_aligned() {}

	float2x2_aligned(const float2x2& m)
		: v0(m[0], 0, 0), v1(m[1]) {}

	float2x2_aligned& operator=(const float2x2& rhs)
	{
		v0 = float4(rhs[0], 0, 0);
		v1 = rhs[1];
		return *this;
	}

	operator float2x2() const
	{
		float2x2 m;
		m[0] = float2(v0);
		m[1] = float2(v1);
		return m;
	}
};

struct float3x3_aligned
{
	float4 v0;
	float4 v1;
	float3 v2;

	float3x3_aligned() {}

	float3x3_aligned(const float3x3& m)
		: v0(m[0], 0), v1(m[1], 0), v2(m[2]) {}

	float3x3_aligned& operator=(const float3x3& rhs)
	{
		v0 = float4(rhs[0], 0);
		v1 = float4(rhs[1], 0);
		v2 = rhs[2];
		return *this;
	}

	operator float3x3() const
	{
		float3x3 m;
		m[0] = float3(v0);
		m[1] = float3(v1);
		m[2] = v2;
		return m;
	}
};

template <typename T, size_t Size>
union hlsl_type
{
	T       value;
	uint8_t padded[Size];

	hlsl_type& operator=(const T& rhs)
	{
		value = rhs;
		return *this;
	}
};

template <size_t Size> using hlsl_float = hlsl_type<float, Size>;
template <size_t Size> using hlsl_float2 = hlsl_type<float2, Size>;
template <size_t Size> using hlsl_float3 = hlsl_type<float3, Size>;
template <size_t Size> using hlsl_float4 = hlsl_type<float4, Size>;

template <size_t Size> using hlsl_float2x2 = hlsl_type<float2x2_aligned, Size>;
template <size_t Size> using hlsl_float3x3 = hlsl_type<float3x3_aligned, Size>;
template <size_t Size> using hlsl_float4x4 = hlsl_type<float4x4, Size>;

template <size_t Size> using hlsl_int = hlsl_type<int, Size>;
template <size_t Size> using hlsl_int2 = hlsl_type<int2, Size>;
template <size_t Size> using hlsl_int3 = hlsl_type<int3, Size>;
template <size_t Size> using hlsl_int4 = hlsl_type<int4, Size>;

template <size_t Size> using hlsl_uint = hlsl_type<uint, Size>;
template <size_t Size> using hlsl_uint2 = hlsl_type<uint2, Size>;
template <size_t Size> using hlsl_uint3 = hlsl_type<uint3, Size>;
template <size_t Size> using hlsl_uint4 = hlsl_type<uint4, Size>;

HLSL_PACK_END();

struct RangeU32 final
{
	uint32_t start;
	uint32_t end;
};

template <typename T>
constexpr T pi()
{
	return static_cast<T>(3.1415926535897932384626433832795);
}
#pragma endregion

//=============================================================================
#pragma region [ Core Func ]

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

template <typename T>
bool IsNull(const T* ptr)
{
	bool res = (ptr == nullptr);
	return res;
}

// Returns true if [a,b) overlaps with [c, d)
inline bool HasOverlapHalfOpen(uint32_t a, [[maybe_unused]] uint32_t b, uint32_t c, uint32_t d)
{
	bool overlap = std::max<uint32_t>(a, c) < std::min<uint32_t>(c, d); // TODO: возможно тут ошибка
	return overlap;
}

inline bool HasOverlapHalfOpen(const RangeU32& r0, const RangeU32& r1)
{
	return HasOverlapHalfOpen(r0.start, r0.end, r1.start, r1.end);
}

#pragma endregion

//=============================================================================
#pragma region [ Flags ]

// Wrapper around an enum that allows simple use of bitwise logic operations.
template<typename Enum, typename Storage = uint32_t>
class Flags
{
public:
	using InternalType = Storage;

	Flags() = default;
	Flags(Enum value) { m_bits = static_cast<Storage>(value); }
	Flags(const Flags<Enum, Storage>& value) { m_bits = value.m_bits; }
	explicit Flags(Storage bits) { m_bits = bits; }

	// Checks whether all of the provided bits are set
	bool IsSet(Enum value) const
	{
		return (m_bits & static_cast<Storage>(value)) == static_cast<Storage>(value);
	}

	// Checks whether any of the provided bits are set
	bool IsSetAny(Enum value) const
	{
		return (m_bits & static_cast<Storage>(value)) != 0;
	}

	// Checks whether any of the provided bits are set
	bool IsSetAny(const Flags<Enum, Storage>& value) const
	{
		return (m_bits & value.m_bits) != 0;
	}

	// Activates all of the provided bits.
	Flags<Enum, Storage>& Set(Enum value)
	{
		m_bits |= static_cast<Storage>(value);
		return *this;
	}

	// Deactivates all of the provided bits.
	void Unset(Enum value)
	{
		m_bits &= ~static_cast<Storage>(value);
	}

	bool operator==(Enum rhs) const
	{
		return m_bits == static_cast<Storage>(rhs);
	}

	bool operator==(const Flags<Enum, Storage>& rhs) const
	{
		return m_bits == rhs.m_bits;
	}

	bool operator==(bool rhs) const
	{
		return ((bool)*this) == rhs;
	}

	bool operator!=(Enum rhs) const
	{
		return m_bits != static_cast<Storage>(rhs);
	}

	bool operator!=(const Flags<Enum, Storage>& rhs) const
	{
		return m_bits != rhs.m_bits;
	}

	Flags<Enum, Storage>& operator=(Enum rhs)
	{
		m_bits = static_cast<Storage>(rhs);
		return *this;
	}

	Flags<Enum, Storage>& operator=(const Flags<Enum, Storage>& rhs)
	{
		m_bits = rhs.m_bits;
		return *this;
	}

	Flags<Enum, Storage>& operator|=(Enum rhs)
	{
		m_bits |= static_cast<Storage>(rhs);
		return *this;
	}

	Flags<Enum, Storage>& operator|=(const Flags<Enum, Storage>& rhs)
	{
		m_bits |= rhs.m_bits;
		return *this;
	}

	Flags<Enum, Storage> operator|(Enum rhs) const
	{
		Flags<Enum, Storage> out(*this);
		out |= rhs;
		return out;
	}

	Flags<Enum, Storage> operator|(const Flags<Enum, Storage>& rhs) const
	{
		Flags<Enum, Storage> out(*this);
		out |= rhs;
		return out;
	}

	Flags<Enum, Storage>& operator&=(Enum rhs)
	{
		m_bits &= static_cast<Storage>(rhs);
		return *this;
	}

	Flags<Enum, Storage>& operator&=(const Flags<Enum, Storage>& rhs)
	{
		m_bits &= rhs.m_bits;
		return *this;
	}

	Flags<Enum, Storage> operator&(Enum rhs) const
	{
		Flags<Enum, Storage> out = *this;
		out.m_bits &= static_cast<Storage>(rhs);
		return out;
	}

	Flags<Enum, Storage> operator&(const Flags<Enum, Storage>& rhs) const
	{
		Flags<Enum, Storage> out = *this;
		out.m_bits &= rhs.m_bits;
		return out;
	}

	Flags<Enum, Storage>& operator^=(Enum rhs)
	{
		m_bits ^= static_cast<Storage>(rhs);
		return *this;
	}

	Flags<Enum, Storage>& operator^=(const Flags<Enum, Storage>& rhs)
	{
		m_bits ^= rhs.m_bits;
		return *this;
	}

	Flags<Enum, Storage> operator^(Enum rhs) const
	{
		Flags<Enum, Storage> out = *this;
		out.m_bits ^= static_cast<Storage>(rhs);
		return out;
	}

	Flags<Enum, Storage> operator^(const Flags<Enum, Storage>& rhs) const
	{
		Flags<Enum, Storage> out = *this;
		out.m_bits ^= rhs.m_bits;
		return out;
	}

	Flags<Enum, Storage> operator~() const
	{
		Flags<Enum, Storage> out;
		out.m_bits = (Storage)~m_bits;

		return out;
	}

	operator bool() const
	{
		return m_bits ? true : false;
	}

	operator uint8_t() const { return static_cast<uint8_t>(m_bits); }
	operator uint16_t() const { return static_cast<uint16_t>(m_bits); }
	operator uint32_t() const { return static_cast<uint32_t>(m_bits); }

	friend Flags<Enum, Storage> operator&(Enum a, Flags<Enum, Storage>& b)
	{
		Flags<Enum, Storage> out;
		out.m_bits = a & b.m_bits;
		return out;
	}

private:
	InternalType m_bits{ 0 };
};

// Defines global operators for a Flags<Enum, Storage> implementation.
#define FLAGS_OPERATORS(Enum) FLAGS_OPERATORS_EXT(Enum, uint32_t)

#define FLAGS_OPERATORS_EXT(Enum, Storage)                                                                     \
		inline Flags<Enum, Storage> operator|(Enum a, Enum b) { Flags<Enum, Storage> r(a); r |= b; return r; } \
		inline Flags<Enum, Storage> operator&(Enum a, Enum b) { Flags<Enum, Storage> r(a); r &= b; return r; } \
		inline Flags<Enum, Storage> operator~(Enum a) { return ~Flags<Enum, Storage>(a); }

#pragma endregion

//=============================================================================
#pragma region [ Ptr ]

template <typename ObjectT>
class ObjPtrRef final
{
public:
	ObjPtrRef(ObjectT** ptrRef) : m_ptrRef(ptrRef) {}

	operator void**() const
	{
		void** addr = reinterpret_cast<void**>(m_ptrRef);
		return addr;
	}
	operator ObjectT**() { return m_ptrRef; }

private:
	ObjectT** m_ptrRef = nullptr;
};

template <typename ObjectT>
class ObjPtr final
{
public:
	using object_type = ObjectT;

	ObjPtr(ObjectT* ptr = nullptr) : m_ptr(ptr) {}

	ObjPtr& operator=(const ObjectT* rhs)
	{
		if (rhs != m_ptr) m_ptr = const_cast<ObjectT*>(rhs);
		return *this;
	}

	operator bool() const
	{
		return (m_ptr != nullptr);
	}

	bool operator==(const ObjPtr<ObjectT>& rhs) const
	{
		return m_ptr == rhs.m_ptr;
	}

	ObjectT* Get() const
	{
		return m_ptr;
	}

	void Reset()
	{
		m_ptr = nullptr;
	}

	bool IsNull() const
	{
		return m_ptr == nullptr;
	}

	operator ObjectT*() const
	{
		return m_ptr;
	}

	ObjectT* operator->() const
	{
		return m_ptr;
	}

	ObjPtrRef<ObjectT> operator&()
	{
		return ObjPtrRef<ObjectT>(&m_ptr);
	}

private:
	ObjectT* m_ptr = nullptr;
};

#pragma endregion

//=============================================================================
#pragma region [ Time ]

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

//=============================================================================
#pragma region [ IO ]

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

//=============================================================================
#pragma region [ Log ]

void Print(const std::string& msg);
void Warning(const std::string& msg);
void Error(const std::string& msg);
void Fatal(const std::string& msg);

#define ASSERT_MSG(COND, MSG)                                         \
    if ((COND) == false) {                                            \
        ::Fatal(std::string("*** ASSERT ***\n")                       \
            + std::string("Message   : ") + MSG + "\n"                \
            + std::string("Condition : ") + #COND + "\n"              \
            + std::string("Function  : ") + __FUNCTION__ + "\n"       \
            + std::string("Location  : ") + __FILE__ + " : " + LINE); \
        assert(false);                                                \
    }

#define ASSERT_NULL_ARG(ARG)                                          \
    if ((ARG) == nullptr) {                                           \
        ::Fatal(std::string("*** NULL ARGUMNET ***\n")                \
            + std::string("Argument  : ") + #ARG + "\n"               \
            + std::string("Function  : ") + __FUNCTION__ + "\n"       \
            + std::string("Location  : ") + __FILE__ + " : " + LINE); \
        assert(false);                                                \
    }

#define CHECKED_CALL(EXPR)                                                        \
    {                                                                             \
        Result checkedResult = EXPR;                                              \
        if (checkedResult != SUCCESS) {                                           \
            ::Fatal(std::string("*** Call Failed ***\n")                          \
                + std::string("\tReturn     : ") + ToString(checkedResult) + "\n" \
                + std::string("\tExpression : ") + #EXPR + "\n"                   \
                + std::string("\tFunction   : ") + __FUNCTION__ + "\n"            \
                + std::string("\tLocation   : ") + __FILE__ + " : " + LINE);      \
            assert(false);                                                        \
        }                                                                         \
    }

#define CHECKED_CALL_AND_RETURN_FALSE(EXPR)                                       \
    {                                                                             \
        Result checkedResult = EXPR;                                              \
        if (checkedResult != SUCCESS) {                                           \
            ::Fatal(std::string("*** Call Failed ***\n")                          \
                + std::string("\tReturn     : ") + ToString(checkedResult) + "\n" \
                + std::string("\tExpression : ") + #EXPR + "\n"                   \
                + std::string("\tFunction   : ") + __FUNCTION__ + "\n"            \
                + std::string("\tLocation   : ") + __FILE__ + " : " + LINE);      \
            return false;                                                         \
        }                                                                         \
    }

// TODO: удалить
enum Result
{
	SUCCESS = 0,
	ERROR_FAILED = -1,
	ERROR_ALLOCATION_FAILED = -2,
	ERROR_OUT_OF_MEMORY = -3,
	ERROR_ELEMENT_NOT_FOUND = -4,
	ERROR_OUT_OF_RANGE = -5,
	ERROR_DUPLICATE_ELEMENT = -6,
	ERROR_LIMIT_EXCEEDED = -7,
	ERROR_PATH_DOES_NOT_EXIST = -8,
	ERROR_SINGLE_INIT_ONLY = -9,
	ERROR_UNEXPECTED_NULL_ARGUMENT = -10,
	ERROR_UNEXPECTED_COUNT_VALUE = -11,
	ERROR_UNSUPPORTED_API = -12,
	ERROR_API_FAILURE = -13,
	ERROR_WAIT_FAILED = -14,
	ERROR_WAIT_TIMED_OUT = -15,
	ERROR_NO_GPUS_FOUND = -16,
	ERROR_REQUIRED_FEATURE_UNAVAILABLE = -17,
	ERROR_BAD_DATA_SOURCE = -18,

	ERROR_INVALID_CREATE_ARGUMENT = -300,
	ERROR_RANGE_ALIASING_NOT_ALLOWED = -301,

	ERROR_GRFX_INVALID_OWNERSHIP = -1000,
	ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED = -1001,
	ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT = -1002,
	ERROR_GRFX_UNSUPPORTED_PRESENT_MODE = -1003,
	ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED = -1004,
	ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED = -1005,
	ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER = -1006,
	ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES = -1007,
	ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE = -1008,
	ERROR_GRFX_INVALID_DESCRIPTOR_TYPE = -1009,
	ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED = -1010,
	ERROR_GRFX_BINDING_NOT_IN_SET = -1011,
	ERROR_GRFX_NON_UNIQUE_SET = -1012,
	ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET = -1013,
	ERROR_GRFX_INVALID_SHADER_BYTE_CODE = -1014,
	ERROR_GRFX_INVALID_PIPELINE_INTERFACE = -1015,
	ERROR_GRFX_INVALID_QUERY_TYPE = -1016,
	ERROR_GRFX_INVALID_QUERY_COUNT = -1017,
	ERROR_GRFX_NO_QUEUES_AVAILABLE = -1018,
	ERROR_GRFX_INVALID_INDEX_TYPE = -1019,
	ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION = -1020,
	ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT = -1021,
	ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE = -1022,
	ERROR_GRFX_NON_UNIQUE_BINDING = -1023,
	ERROR_GRFX_INVALID_BINDING_NUMBER = -1024,
	ERROR_GRFX_INVALID_SET_NUMBER = -1025,
	ERROR_GRFX_OPERATION_NOT_PERMITTED = -1026,
	ERROR_GRFX_INVALID_SEMAPHORE_TYPE = -1027,

	ERROR_IMAGE_FILE_LOAD_FAILED = -2000,
	ERROR_IMAGE_FILE_SAVE_FAILED = -2001,
	ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE = -2002,
	ERROR_IMAGE_INVALID_FORMAT = -2003,
	ERROR_IMAGE_RESIZE_FAILED = -2004,	
	ERROR_BITMAP_CREATE_FAILED = -2005,
	ERROR_BITMAP_BAD_COPY_SOURCE = -2006,
	ERROR_BITMAP_FOOTPRINT_MISMATCH = -2007,

	ERROR_GEOMETRY_NO_INDEX_DATA = -2400,
	ERROR_GEOMETRY_FILE_LOAD_FAILED = -2500,
	ERROR_GEOMETRY_FILE_NO_DATA = -2501,
	ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC = -2502,

	ERROR_IMGUI_INITIALIZATION_FAILED = -3001,

	ERROR_FONT_PARSE_FAILED = -4000,
	ERROR_INVALID_UTF8_STRING = -4001,

	ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED = -5000,
	ERROR_PPM_EXPORT_INVALID_SIZE = -5001,

	ERROR_SCENE_UNSUPPORTED_FILE_TYPE = -6001,
	ERROR_SCENE_UNSUPPORTED_NODE_TYPE = -6002,
	ERROR_SCENE_UNSUPPORTED_CAMERA_TYPE = -6003,
	ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE = -6004,
	ERROR_SCENE_SOURCE_FILE_LOAD_FAILED = -6005,
	ERROR_SCENE_NO_SOURCE_DATA = -6006,
	ERROR_SCENE_INVALID_SOURCE_SCENE = -6007,
	ERROR_SCENE_INVALID_SOURCE_NODE = -6008,
	ERROR_SCENE_INVALID_SOURCE_CAMERA = -6009,
	ERROR_SCENE_INVALID_SOURCE_LIGHT = -6010,
	ERROR_SCENE_INVALID_SOURCE_MESH = -6011,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE = -6012,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA = -6013,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA = -6014,
	ERROR_SCENE_INVALID_SOURCE_MATERIAL = -6015,
	ERROR_SCENE_INVALID_SOURCE_TEXTURE = -6016,
	ERROR_SCENE_INVALID_SOURCE_IMAGE = -6017,
	ERROR_SCENE_INVALID_NODE_HIERARCHY = -6018,
	ERROR_SCENE_INVALID_STANDALONE_OPERATION = -6019,
	ERROR_SCENE_NODE_ALREADY_HAS_PARENT = -6020,
};

inline const char* ToString(Result value)// TODO: удалить
{
	switch (value) {
	default: break;
	case Result::SUCCESS: return "SUCCESS";
	case Result::ERROR_FAILED: return "ERROR_FAILED";
	case Result::ERROR_ALLOCATION_FAILED: return "ERROR_ALLOCATION_FAILED";
	case Result::ERROR_OUT_OF_MEMORY: return "ERROR_OUT_OF_MEMORY";
	case Result::ERROR_ELEMENT_NOT_FOUND: return "ERROR_ELEMENT_NOT_FOUND";
	case Result::ERROR_OUT_OF_RANGE: return "ERROR_OUT_OF_RANGE";
	case Result::ERROR_DUPLICATE_ELEMENT: return "ERROR_DUPLICATE_ELEMENT";
	case Result::ERROR_LIMIT_EXCEEDED: return "ERROR_LIMIT_EXCEEDED";
	case Result::ERROR_PATH_DOES_NOT_EXIST: return "ERROR_PATH_DOES_NOT_EXIST";
	case Result::ERROR_SINGLE_INIT_ONLY: return "ERROR_SINGLE_INIT_ONLY";
	case Result::ERROR_UNEXPECTED_NULL_ARGUMENT: return "ERROR_UNEXPECTED_NULL_ARGUMENT";
	case Result::ERROR_UNEXPECTED_COUNT_VALUE: return "ERROR_UNEXPECTED_COUNT_VALUE";
	case Result::ERROR_UNSUPPORTED_API: return "ERROR_UNSUPPORTED_API";
	case Result::ERROR_API_FAILURE: return "ERROR_API_FAILURE";
	case Result::ERROR_WAIT_FAILED: return "ERROR_WAIT_FAILED";
	case Result::ERROR_WAIT_TIMED_OUT: return "ERROR_WAIT_TIMED_OUT";
	case Result::ERROR_NO_GPUS_FOUND: return "ERROR_NO_GPUS_FOUND";
	case Result::ERROR_REQUIRED_FEATURE_UNAVAILABLE: return "ERROR_REQUIRED_FEATURE_UNAVAILABLE";
	case Result::ERROR_BAD_DATA_SOURCE: return "ERROR_BAD_DATA_SOURCE";

	case Result::ERROR_INVALID_CREATE_ARGUMENT: return "ERROR_INVALID_CREATE_ARGUMENT";
	case Result::ERROR_RANGE_ALIASING_NOT_ALLOWED: return "ERROR_RANGE_ALIASING_NOT_ALLOWED";

	case Result::ERROR_GRFX_INVALID_OWNERSHIP: return "ERROR_GRFX_INVALID_OWNERSHIP";
	case Result::ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED: return "ERROR_GRFX_OBJECT_OWNERSHIP_IS_RESTRICTED";
	case Result::ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT: return "ERROR_GRFX_UNSUPPORTED_SWAPCHAIN_FORMAT";
	case Result::ERROR_GRFX_UNSUPPORTED_PRESENT_MODE: return "ERROR_GRFX_UNSUPPORTED_PRESENT_MODE";
	case Result::ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED: return "ERROR_GRFX_MAX_VERTEX_BINDING_EXCEEDED";
	case Result::ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED: return "ERROR_GRFX_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED";
	case Result::ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER: return "ERROR_GRFX_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER";
	case Result::ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES: return "ERROR_GRFX_CANNOT_MIX_VERTEX_INPUT_RATES";
	case Result::ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE: return "ERROR_GRFX_UNKNOWN_DESCRIPTOR_TYPE";
	case Result::ERROR_GRFX_INVALID_DESCRIPTOR_TYPE: return "ERROR_GRFX_INVALID_DESCRIPTOR_TYPE";
	case Result::ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED: return "ERROR_GRFX_DESCRIPTOR_COUNT_EXCEEDED";
	case Result::ERROR_GRFX_BINDING_NOT_IN_SET: return "ERROR_GRFX_BINDING_NOT_IN_SET";
	case Result::ERROR_GRFX_NON_UNIQUE_SET: return "ERROR_GRFX_NON_UNIQUE_SET";
	case Result::ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET: return "ERROR_GRFX_MINIMUM_BUFFER_SIZE_NOT_MET";
	case Result::ERROR_GRFX_INVALID_SHADER_BYTE_CODE: return "ERROR_GRFX_INVALID_SHADER_BYTE_CODE";
	case Result::ERROR_GRFX_INVALID_PIPELINE_INTERFACE: return "ERROR_GRFX_INVALID_PIPELINE_INTERFACE";
	case Result::ERROR_GRFX_INVALID_QUERY_TYPE: return "ERROR_GRFX_INVALID_QUERY_TYPE";
	case Result::ERROR_GRFX_INVALID_QUERY_COUNT: return "ERROR_GRFX_INVALID_QUERY_COUNT";
	case Result::ERROR_GRFX_NO_QUEUES_AVAILABLE: return "ERROR_GRFX_NO_QUEUES_AVAILABLE";
	case Result::ERROR_GRFX_INVALID_INDEX_TYPE: return "ERROR_GRFX_INVALID_INDEX_TYPE";
	case Result::ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION: return "ERROR_GRFX_INVALID_GEOMETRY_CONFIGURATION ";
	case Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT: return "ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_COUNT ";
	case Result::ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE: return "ERROR_GRFX_INVALID_VERTEX_ATTRIBUTE_STRIDE";
	case Result::ERROR_GRFX_NON_UNIQUE_BINDING: return "ERROR_GRFX_NON_UNIQUE_BINDING";
	case Result::ERROR_GRFX_INVALID_BINDING_NUMBER: return "ERROR_GRFX_INVALID_BINDING_NUMBER";
	case Result::ERROR_GRFX_INVALID_SET_NUMBER: return "ERROR_GRFX_INVALID_SET_NUMBER";
	case Result::ERROR_GRFX_OPERATION_NOT_PERMITTED: return "ERROR_GRFX_OPERATION_NOT_PERMITTED";
	case Result::ERROR_GRFX_INVALID_SEMAPHORE_TYPE: return "ERROR_GRFX_INVALID_SEMAPHORE_TYPE";

	case Result::ERROR_IMAGE_FILE_LOAD_FAILED: return "ERROR_IMAGE_FILE_LOAD_FAILED";
	case Result::ERROR_IMAGE_FILE_SAVE_FAILED: return "ERROR_IMAGE_FILE_SAVE_FAILED";
	case Result::ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE: return "ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE";
	case Result::ERROR_IMAGE_INVALID_FORMAT: return "ERROR_IMAGE_INVALID_FORMAT";
	case Result::ERROR_IMAGE_RESIZE_FAILED: return "ERROR_IMAGE_RESIZE_FAILED";
	case Result::ERROR_BITMAP_CREATE_FAILED: return "ERROR_BITMAP_CREATE_FAILED";
	case Result::ERROR_BITMAP_BAD_COPY_SOURCE: return "ERROR_BITMAP_BAD_COPY_SOURCE";
	case Result::ERROR_BITMAP_FOOTPRINT_MISMATCH: return "ERROR_BITMAP_FOOTPRINT_MISMATCH";

	case Result::ERROR_GEOMETRY_NO_INDEX_DATA: return "ERROR_GEOMETRY_NO_INDEX_DATA";
	case Result::ERROR_GEOMETRY_FILE_LOAD_FAILED: return "ERROR_GEOMETRY_FILE_LOAD_FAILED";
	case Result::ERROR_GEOMETRY_FILE_NO_DATA: return "ERROR_GEOMETRY_FILE_NO_DATA";
	case Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC: return "ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC";

	case Result::ERROR_IMGUI_INITIALIZATION_FAILED: return "ERROR_IMGUI_INITIALIZATION_FAILED";

	case Result::ERROR_FONT_PARSE_FAILED: return "ERROR_FONT_PARSE_FAILED";
	case Result::ERROR_INVALID_UTF8_STRING: return "ERROR_INVALID_UTF8_STRING";

	case Result::ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED: return "ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED";
	case Result::ERROR_PPM_EXPORT_INVALID_SIZE: return "ERROR_PPM_EXPORT_INVALID_SIZE";
	
	case Result::ERROR_SCENE_UNSUPPORTED_FILE_TYPE: return "ERROR_SCENE_UNSUPPORTED_FILE_TYPE";
	case Result::ERROR_SCENE_UNSUPPORTED_NODE_TYPE: return "ERROR_SCENE_UNSUPPORTED_NODE_TYPE";
	case Result::ERROR_SCENE_UNSUPPORTED_CAMERA_TYPE: return "ERROR_SCENE_UNSUPPORTED_CAMERA_TYPE";
	case Result::ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE: return "ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE";
	case Result::ERROR_SCENE_SOURCE_FILE_LOAD_FAILED: return "ERROR_SCENE_SOURCE_FILE_LOAD_FAILED";
	case Result::ERROR_SCENE_NO_SOURCE_DATA: return "ERROR_SCENE_NO_SOURCE_DATA";
	case Result::ERROR_SCENE_INVALID_SOURCE_SCENE: return "ERROR_SCENE_INVALID_SOURCE_SCENE";
	case Result::ERROR_SCENE_INVALID_SOURCE_NODE: return "ERROR_SCENE_INVALID_SOURCE_NODE";
	case Result::ERROR_SCENE_INVALID_SOURCE_CAMERA: return "ERROR_SCENE_INVALID_SOURCE_CAMERA";
	case Result::ERROR_SCENE_INVALID_SOURCE_LIGHT: return "ERROR_SCENE_INVALID_SOURCE_LIGHT";
	case Result::ERROR_SCENE_INVALID_SOURCE_MESH: return "ERROR_SCENE_INVALID_SOURCE_MESH";
	case Result::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE: return "ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE";
	case Result::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA: return "ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA";
	case Result::ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA: return "ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA";
	case Result::ERROR_SCENE_INVALID_SOURCE_MATERIAL: return "ERROR_SCENE_INVALID_SOURCE_MATERIAL";
	case Result::ERROR_SCENE_INVALID_SOURCE_TEXTURE: return "ERROR_SCENE_INVALID_SOURCE_TEXTURE";
	case Result::ERROR_SCENE_INVALID_SOURCE_IMAGE: return "ERROR_SCENE_INVALID_SOURCE_IMAGE";
	case Result::ERROR_SCENE_INVALID_NODE_HIERARCHY: return "ERROR_SCENE_INVALID_NODE_HIERARCHY";
	case Result::ERROR_SCENE_INVALID_STANDALONE_OPERATION: return "ERROR_SCENE_INVALID_STANDALONE_OPERATION";
	case Result::ERROR_SCENE_NODE_ALREADY_HAS_PARENT: return "ERROR_SCENE_NODE_ALREADY_HAS_PARENT";
	}
	return "<unknown ppx::Result value>";
}

inline bool Success(Result value)
{
	bool res = (value == SUCCESS);
	return res;
}

inline bool Failed(Result value)
{
	bool res = (value < SUCCESS);
	return res;
}

#pragma endregion

//=============================================================================
#pragma region [ Core Math ]

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

// Converts spherical coordinate 'sc' to unit cartesian position.
//
// theta is the azimuth angle between [0, 2pi].
// phi is the polar angle between [0, pi].
//
// theta = 0, phi =[0, pi] sweeps the positive X axis:
//    SphericalToCartesian(0, 0)    = (0,  1, 0)
//    SphericalToCartesian(0, pi/2) = (1,  0, 0)
//    SphericalToCartesian(0, pi)   = (0, -1, 0)
//
// theta = [0, 2pi], phi = [pi/2] sweeps a circle:
//    SphericalToCartesian(0,     pi/2) = ( 1, 0, 0)
//    SphericalToCartesian(pi/2,  pi/2) = ( 0, 0, 1)
//    SphericalToCartesian(pi  ,  pi/2) = (-1, 0, 0)
//    SphericalToCartesian(3pi/2, pi/2) = ( 0, 0,-1)
//    SphericalToCartesian(2pi,   pi/2) = ( 1, 0, 0)
//
inline float3 SphericalToCartesian(float theta, float phi)
{
	return float3(
		cos(theta) * sin(phi), // x
		cos(phi),              // y
		sin(theta) * sin(phi)  // z
	);
}

inline float3 SphericalTangent(float theta, [[maybe_unused]] float phi)
{
	return float3(
		sin(theta), // x
		0,          // y
		-cos(theta) // z
	);
}

#pragma endregion

//=============================================================================
#pragma region [ Color ]

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
		return static_cast<uint32_t>((a << 24) | (r << 16) | (g << 8) | (b << 0));
	}

	uint32_t ToPackedABGR() const
	{
		return static_cast<uint32_t>((a << 24) | (b << 16) | (g << 8) | (r << 0));
	}

	uint32_t ToPackedRGBA() const
	{
		return static_cast<uint32_t>((r << 24) | (g << 16) | (b << 8) | (a << 0));
	}

	uint32_t ToPackedBGRA() const
	{
		return static_cast<uint32_t>((b << 24) | (g << 16) | (r << 8) | (a << 0));
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

//=============================================================================
#pragma region [ Math Geometry ]

class OBB;

// Axis aligned box represented by minimum and maximum point.
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

class Transform
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
	virtual ~Transform() = default;

	bool IsDirty() const { return (m_dirty.mask != 0); }

	const glm::vec3& GetTranslation() const { return m_translation; }
	const glm::vec3& GetRotation() const { return m_rotation; }
	const glm::vec3& GetScale() const { return m_scale; }
	RotationOrder GetRotationOrder() const { return m_rotationOrder; }

	virtual void SetTranslation(const glm::vec3& value);
	virtual void SetRotation(const glm::vec3& eulerAngles);
	virtual void SetScale(const glm::vec3& value);
	virtual void SetRotationOrder(Transform::RotationOrder value);

	Transform& Translate(const glm::vec3& translation);
	Transform& RotateX(const float radians);
	Transform& RotateY(const float radians);
	Transform& RotateZ(const float radians);
	Transform& ClampPitch();

	const glm::mat4x4& GetTranslationMatrix() const;
	const glm::mat4x4& GetRotationMatrix() const;
	const glm::mat4x4& GetScaleMatrix() const;
	const glm::mat4x4& GetConcatenatedMatrix() const;

	[[nodiscard]] glm::vec3 GetRightVector() const;
	[[nodiscard]] glm::vec3 GetUpVector() const;
	[[nodiscard]] glm::vec3 GetForwardVector() const;
	[[nodiscard]] glm::vec3 GetHorizontalRightVector() const;
	[[nodiscard]] glm::vec3 GetHorizontalForwardVector() const;

protected:
#if defined(_MSC_VER)
#	pragma warning(push)
#	pragma warning(disable : 4201)
#endif
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
#if defined(_MSC_VER)
#	pragma warning(pop)
#endif

	float clampPitch(const float radians) { return glm::clamp(radians, -glm::half_pi<float>(), glm::half_pi<float>()); }
	// Wrap to (-PI..PI]
	float wrapAngle(const float radians) { return std::remainder(radians, glm::two_pi<float>()); }

	glm::vec3           m_translation = glm::vec3(0, 0, 0);
	glm::vec3           m_rotation = glm::vec3(0, 0, 0);
	glm::vec3           m_scale = glm::vec3(1, 1, 1);
	RotationOrder       m_rotationOrder = RotationOrder::XYZ;
	mutable glm::mat4x4 m_translationMatrix{ 1.0f };
	mutable glm::mat4x4 m_rotationMatrix{ 1.0f };
	mutable glm::mat4x4 m_scaleMatrix{ 1.0f };
	mutable glm::mat4x4 m_concatenatedMatrix{ 1.0f };
};

#pragma endregion

//=============================================================================
#pragma region [ Random ]

class Random final
{
public:
	Random()
	{
		Seed(0xDEAD, 0xBEEF);
	}

	Random(uint64_t initialState, uint64_t initialSequence)
	{
		Seed(initialState, initialSequence);
	}

	void Seed(uint64_t initialState, uint64_t initialSequence)
	{
		mRng.seed(initialState, initialSequence);
	}

	uint32_t UInt32()
	{
		uint32_t value = mRng.nextUInt();
		return value;
	}

	float Float()
	{
		float value = mRng.nextFloat();
		return value;
	}

	float Float(float a, float b)
	{
		float value = glm::lerp(a, b, Float());
		return value;
	}

	float2 Float2()
	{
		float2 value(Float(), Float());
		return value;
	}

	float2 Float2(const float2& a, const float2& b)
	{
		float2 value(Float(a.x, b.x), Float(a.y, b.y));
		return value;
	}

	float3 Float3()
	{
		float3 value(Float(), Float(), Float());
		return value;
	}

	float3 Float3(const float3& a, const float3& b)
	{
		float3 value(Float(a.x, b.x), Float(a.y, b.y), Float(a.z, b.z));
		return value;
	}

	float4 Float4()
	{
		float4 value(Float(), Float(), Float(), Float());
		return value;
	}

	float4 Float4(const float4& a, const float4& b)
	{
		float4 value(Float(a.x, b.x), Float(a.y, b.y), Float(a.z, b.z), Float(a.w, b.w));
		return value;
	}

private:
	pcg32 mRng;
};

#pragma endregion
