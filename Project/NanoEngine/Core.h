#pragma once

#pragma region Base type

// Import GLM types as HLSL friendly names

// bool
using bool2     = glm::bool2;
using bool3     = glm::bool3;
using bool4     = glm::bool4;

// 8-bit signed integer
using i8        = glm::i8;
using i8vec2    = glm::i8vec2;
using i8vec3    = glm::i8vec3;
using i8vec4    = glm::i8vec4;

// 16-bit unsigned integer
using half      = glm::u16;
using half2     = glm::u16vec2;
using half3     = glm::u16vec3;
using half4     = glm::u16vec4;

// 32-bit signed integer
using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;
// 32-bit unsigned integer
using uint      = glm::uint;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;

// 32-bit float
using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
// 32-bit float2 matrices
using float2x2  = glm::mat2x2;
using float2x3  = glm::mat2x3;
using float2x4  = glm::mat2x4;
// 32-bit float3 matrices
using float3x2  = glm::mat3x2;
using float3x3  = glm::mat3x3;
using float3x4  = glm::mat3x4;
// 32-bit float4 matrices
using float4x2  = glm::mat4x2;
using float4x3  = glm::mat4x3;
using float4x4  = glm::mat4x4;
// 32-bit float quaternion
using quat      = glm::quat;

// 64-bit float
using double2   = glm::dvec2;
using double3   = glm::dvec3;
using double4   = glm::dvec4;
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

HLSL_PACK_BEGIN();

struct float2x2_aligned
{
	float2x2_aligned() = default;
	float2x2_aligned(const float2x2& m) : v0(m[0], 0, 0), v1(m[1]) {}

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

	float4 v0;
	float2 v1;
};

struct float3x3_aligned
{
	float3x3_aligned() = default;
	float3x3_aligned(const float3x3& m) : v0(m[0], 0), v1(m[1], 0), v2(m[2]) {}

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

	float4 v0;
	float4 v1;
	float3 v2;
};

template <typename T, size_t Size>
union hlsl_type
{
	hlsl_type& operator=(const T& rhs)
	{
		value = rhs;
		return *this;
	}

	T       value;
	uint8_t padded[Size];
};

template <size_t Size> using hlsl_float  = hlsl_type<float, Size>;
template <size_t Size> using hlsl_float2 = hlsl_type<float2, Size>;
template <size_t Size> using hlsl_float3 = hlsl_type<float3, Size>;
template <size_t Size> using hlsl_float4 = hlsl_type<float4, Size>;

template <size_t Size> using hlsl_float2x2 = hlsl_type<float2x2_aligned, Size>;
template <size_t Size> using hlsl_float3x3 = hlsl_type<float3x3_aligned, Size>;
template <size_t Size> using hlsl_float4x4 = hlsl_type<float4x4, Size>;

template <size_t Size> using hlsl_int  = hlsl_type<int, Size>;
template <size_t Size> using hlsl_int2 = hlsl_type<int2, Size>;
template <size_t Size> using hlsl_int3 = hlsl_type<int3, Size>;
template <size_t Size> using hlsl_int4 = hlsl_type<int4, Size>;

template <size_t Size> using hlsl_uint  = hlsl_type<uint, Size>;
template <size_t Size> using hlsl_uint2 = hlsl_type<uint2, Size>;
template <size_t Size> using hlsl_uint3 = hlsl_type<uint3, Size>;
template <size_t Size> using hlsl_uint4 = hlsl_type<uint4, Size>;

HLSL_PACK_END();

template <typename T>
constexpr T pi()
{
	return static_cast<T>(3.1415926535897932384626433832795);
}

#pragma endregion

#pragma region Type

struct RangeU32 final
{
	uint32_t start;
	uint32_t end;
};

struct Extent2D final
{
	uint32_t width;
	uint32_t height;
};

#pragma endregion

#pragma region Base Func

template <typename T>
bool IsNull(const T* ptr)
{
	bool res = (ptr == nullptr);
	return res;
}

template <typename T>
T InvalidValue(const T value = static_cast<T>(~0))
{
	return value;
}

template <typename T>
T RoundUp(T value, T multiple)
{
	static_assert(std::is_integral<T>::value, "T must be an integral type");
	assert(multiple && ((multiple & (multiple - 1)) == 0));
	return (value + multiple - 1) & ~(multiple - 1);
}

template <typename T>
uint32_t CountU32(const std::vector<T>& container)
{
	uint32_t n = static_cast<uint32_t>(container.size());
	return n;
}

template <typename T>
T* DataPtr(std::vector<T>& container)
{
	T* ptr = container.empty() ? nullptr : container.data();
	return ptr;
}

template <typename T>
const T* DataPtr(const std::vector<T>& container)
{
	const T* ptr = container.empty() ? nullptr : container.data();
	return ptr;
}

template <typename T>
uint32_t SizeInBytesU32(const std::vector<T>& container)
{
	uint32_t size = static_cast<uint32_t>(container.size() * sizeof(T));
	return size;
}

template <typename T>
uint64_t SizeInBytesU64(const std::vector<T>& container)
{
	uint64_t size = static_cast<uint64_t>(container.size() * sizeof(T));
	return size;
}

template <typename T>
bool IsIndexInRange(uint32_t index, const std::vector<T>& container)
{
	uint32_t n = CountU32(container);
	bool     res = (index < n);
	return res;
}

// Returns true if [a,b) overlaps with [c, d)
inline bool HasOverlapHalfOpen(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
	bool overlap = std::max<uint32_t>(a, c) < std::min<uint32_t>(c, d); // TODO: ошибка? вместо a,c - a,b?
	return overlap;
}

inline bool HasOverlapHalfOpen(const RangeU32& r0, const RangeU32& r1)
{
	return HasOverlapHalfOpen(r0.start, r0.end, r1.start, r1.end);
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
	if (!IsIndexInRange(index, container))
		return false;

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

inline std::vector<const char*> GetCStrings(const std::vector<std::string>& container)
{
	std::vector<const char*> cstrings;
	for (auto& str : container)
		cstrings.push_back(str.c_str());

	return cstrings;
}

inline std::vector<std::string> GetNotFound(const std::vector<std::string>& search, const std::vector<std::string>& container)
{
	std::vector<std::string> result;
	for (auto& elem : search)
	{
		auto it = std::find(
			std::begin(container),
			std::end(container),
			elem);
		if (it == std::end(container))
		{
			result.push_back(elem);
		}
	}
	return result;
}

inline std::string FloatString(float value, int precision = 6, int width = 6)
{
	std::stringstream ss;
	ss << std::setprecision(precision) << std::setw(6) << std::fixed << value;
	return ss.str();
}

#pragma endregion

#pragma region Result Status

enum Result
{
	SUCCESS                            = 0,
	ERROR_FAILED                       = -1,
	ERROR_ALLOCATION_FAILED            = -2,
	ERROR_OUT_OF_MEMORY                = -3,
	ERROR_ELEMENT_NOT_FOUND            = -4,
	ERROR_OUT_OF_RANGE                 = -5,
	ERROR_DUPLICATE_ELEMENT            = -6,
	ERROR_LIMIT_EXCEEDED               = -7,
	ERROR_PATH_DOES_NOT_EXIST          = -8,
	ERROR_SINGLE_INIT_ONLY             = -9,
	ERROR_UNEXPECTED_NULL_ARGUMENT     = -10,
	ERROR_UNEXPECTED_COUNT_VALUE       = -11,
	ERROR_UNSUPPORTED_API              = -12,
	ERROR_API_FAILURE                  = -13,
	ERROR_WAIT_FAILED                  = -14,
	ERROR_WAIT_TIMED_OUT               = -15,
	ERROR_NO_GPUS_FOUND                = -16,
	ERROR_REQUIRED_FEATURE_UNAVAILABLE = -17,
	ERROR_BAD_DATA_SOURCE              = -18,

	ERROR_GLFW_INIT_FAILED          = -200,
	ERROR_GLFW_CREATE_WINDOW_FAILED = -201,

	ERROR_INVALID_CREATE_ARGUMENT    = -300,
	ERROR_RANGE_ALIASING_NOT_ALLOWED = -301,

	ERROR_GR_INVALID_OWNERSHIP                    = -1000,
	ERROR_GR_OBJECT_OWNERSHIP_IS_RESTRICTED       = -1001,
	ERROR_GR_UNSUPPORTED_SWAPCHAIN_FORMAT         = -1002,
	ERROR_GR_UNSUPPORTED_PRESENT_MODE             = -1003,
	ERROR_GR_MAX_VERTEX_BINDING_EXCEEDED          = -1004,
	ERROR_GR_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED    = -1005,
	ERROR_GR_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER = -1006,
	ERROR_GR_CANNOT_MIX_VERTEX_INPUT_RATES        = -1007,
	ERROR_GR_UNKNOWN_DESCRIPTOR_TYPE              = -1008,
	ERROR_GR_INVALID_DESCRIPTOR_TYPE              = -1009,
	ERROR_GR_DESCRIPTOR_COUNT_EXCEEDED            = -1010,
	ERROR_GR_BINDING_NOT_IN_SET                   = -1011,
	ERROR_GR_NON_UNIQUE_SET                       = -1012,
	ERROR_GR_MINIMUM_BUFFER_SIZE_NOT_MET          = -1013,
	ERROR_GR_INVALID_SHADER_BYTE_CODE             = -1014,
	ERROR_GR_INVALID_PIPELINE_INTERFACE           = -1015,
	ERROR_GR_INVALID_QUERY_TYPE                   = -1016,
	ERROR_GR_INVALID_QUERY_COUNT                  = -1017,
	ERROR_GR_NO_QUEUES_AVAILABLE                  = -1018,
	ERROR_GR_INVALID_INDEX_TYPE                   = -1019,
	ERROR_GR_INVALID_GEOMETRY_CONFIGURATION       = -1020,
	ERROR_GR_INVALID_VERTEX_ATTRIBUTE_COUNT       = -1021,
	ERROR_GR_INVALID_VERTEX_ATTRIBUTE_STRIDE      = -1022,
	ERROR_GR_NON_UNIQUE_BINDING                   = -1023,
	ERROR_GR_INVALID_BINDING_NUMBER               = -1024,
	ERROR_GR_INVALID_SET_NUMBER                   = -1025,
	ERROR_GR_OPERATION_NOT_PERMITTED              = -1026,
	ERROR_GR_INVALID_SEMAPHORE_TYPE               = -1027,

	ERROR_IMAGE_FILE_LOAD_FAILED               = -2000,
	ERROR_IMAGE_FILE_SAVE_FAILED               = -2001,
	ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE = -2002,
	ERROR_IMAGE_INVALID_FORMAT                 = -2003,
	ERROR_IMAGE_RESIZE_FAILED                  = -2004,
	ERROR_BITMAP_CREATE_FAILED                 = -2005,
	ERROR_BITMAP_BAD_COPY_SOURCE               = -2006,
	ERROR_BITMAP_FOOTPRINT_MISMATCH            = -2007,

	ERROR_GEOMETRY_NO_INDEX_DATA           = -2400,
	ERROR_GEOMETRY_FILE_LOAD_FAILED        = -2500,
	ERROR_GEOMETRY_FILE_NO_DATA            = -2501,
	ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC = -2502,

	ERROR_WINDOW_EVENTS_ALREADY_REGISTERED = -3000,
	ERROR_IMGUI_INITIALIZATION_FAILED      = -3001,

	ERROR_FONT_PARSE_FAILED   = -4000,
	ERROR_INVALID_UTF8_STRING = -4001,

	ERROR_PPM_EXPORT_FORMAT_NOT_SUPPORTED = -5000,
	ERROR_PPM_EXPORT_INVALID_SIZE         = -5001,

	ERROR_SCENE_UNSUPPORTED_FILE_TYPE               = -6001,
	ERROR_SCENE_UNSUPPORTED_NODE_TYPE               = -6002,
	ERROR_SCENE_UNSUPPORTED_CAMERA_TYPE             = -6003,
	ERROR_SCENE_UNSUPPORTED_TOPOLOGY_TYPE           = -6004,
	ERROR_SCENE_SOURCE_FILE_LOAD_FAILED             = -6005,
	ERROR_SCENE_NO_SOURCE_DATA                      = -6006,
	ERROR_SCENE_INVALID_SOURCE_SCENE                = -6007,
	ERROR_SCENE_INVALID_SOURCE_NODE                 = -6008,
	ERROR_SCENE_INVALID_SOURCE_CAMERA               = -6009,
	ERROR_SCENE_INVALID_SOURCE_LIGHT                = -6010,
	ERROR_SCENE_INVALID_SOURCE_MESH                 = -6011,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_TYPE  = -6012,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_INDEX_DATA  = -6013,
	ERROR_SCENE_INVALID_SOURCE_GEOMETRY_VERTEX_DATA = -6014,
	ERROR_SCENE_INVALID_SOURCE_MATERIAL             = -6015,
	ERROR_SCENE_INVALID_SOURCE_TEXTURE              = -6016,
	ERROR_SCENE_INVALID_SOURCE_IMAGE                = -6017,
	ERROR_SCENE_INVALID_NODE_HIERARCHY              = -6018,
	ERROR_SCENE_INVALID_STANDALONE_OPERATION        = -6019,
	ERROR_SCENE_NODE_ALREADY_HAS_PARENT             = -6020,
};

inline const char* ToString(Result value)
{
	switch (value)
	{
		default: break;
		case Result::SUCCESS                                          : return "SUCCESS";
		case Result::ERROR_FAILED                                     : return "ERROR_FAILED";
		case Result::ERROR_ALLOCATION_FAILED                          : return "ERROR_ALLOCATION_FAILED";
		case Result::ERROR_OUT_OF_MEMORY                              : return "ERROR_OUT_OF_MEMORY";
		case Result::ERROR_ELEMENT_NOT_FOUND                          : return "ERROR_ELEMENT_NOT_FOUND";
		case Result::ERROR_OUT_OF_RANGE                               : return "ERROR_OUT_OF_RANGE";
		case Result::ERROR_DUPLICATE_ELEMENT                          : return "ERROR_DUPLICATE_ELEMENT";
		case Result::ERROR_LIMIT_EXCEEDED                             : return "ERROR_LIMIT_EXCEEDED";
		case Result::ERROR_PATH_DOES_NOT_EXIST                        : return "ERROR_PATH_DOES_NOT_EXIST";
		case Result::ERROR_SINGLE_INIT_ONLY                           : return "ERROR_SINGLE_INIT_ONLY";
		case Result::ERROR_UNEXPECTED_NULL_ARGUMENT                   : return "ERROR_UNEXPECTED_NULL_ARGUMENT";
		case Result::ERROR_UNEXPECTED_COUNT_VALUE                     : return "ERROR_UNEXPECTED_COUNT_VALUE";
		case Result::ERROR_UNSUPPORTED_API                            : return "ERROR_UNSUPPORTED_API";
		case Result::ERROR_API_FAILURE                                : return "ERROR_API_FAILURE";
		case Result::ERROR_WAIT_FAILED                                : return "ERROR_WAIT_FAILED";
		case Result::ERROR_WAIT_TIMED_OUT                             : return "ERROR_WAIT_TIMED_OUT";
		case Result::ERROR_NO_GPUS_FOUND                              : return "ERROR_NO_GPUS_FOUND";
		case Result::ERROR_REQUIRED_FEATURE_UNAVAILABLE               : return "ERROR_REQUIRED_FEATURE_UNAVAILABLE";
		case Result::ERROR_BAD_DATA_SOURCE                            : return "ERROR_BAD_DATA_SOURCE";

		case Result::ERROR_GLFW_INIT_FAILED                           : return "ERROR_GLFW_INIT_FAILED";
		case Result::ERROR_GLFW_CREATE_WINDOW_FAILED                  : return "ERROR_GLFW_CREATE_WINDOW_FAILED";

		case Result::ERROR_INVALID_CREATE_ARGUMENT                    : return "ERROR_INVALID_CREATE_ARGUMENT";
		case Result::ERROR_RANGE_ALIASING_NOT_ALLOWED                 : return "ERROR_RANGE_ALIASING_NOT_ALLOWED";

		case Result::ERROR_GR_INVALID_OWNERSHIP                     : return "ERROR_GR_INVALID_OWNERSHIP";
		case Result::ERROR_GR_OBJECT_OWNERSHIP_IS_RESTRICTED        : return "ERROR_GR_OBJECT_OWNERSHIP_IS_RESTRICTED";
		case Result::ERROR_GR_UNSUPPORTED_SWAPCHAIN_FORMAT          : return "ERROR_GR_UNSUPPORTED_SWAPCHAIN_FORMAT";
		case Result::ERROR_GR_UNSUPPORTED_PRESENT_MODE              : return "ERROR_GR_UNSUPPORTED_PRESENT_MODE";
		case Result::ERROR_GR_MAX_VERTEX_BINDING_EXCEEDED           : return "ERROR_GR_MAX_VERTEX_BINDING_EXCEEDED";
		case Result::ERROR_GR_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED     : return "ERROR_GR_VERTEX_ATTRIBUTE_FORMAT_UNDEFINED";
		case Result::ERROR_GR_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER  : return "ERROR_GR_VERTEX_ATTRIBUTE_OFFSET_OUT_OF_ORDER";
		case Result::ERROR_GR_CANNOT_MIX_VERTEX_INPUT_RATES         : return "ERROR_GR_CANNOT_MIX_VERTEX_INPUT_RATES";
		case Result::ERROR_GR_UNKNOWN_DESCRIPTOR_TYPE               : return "ERROR_GR_UNKNOWN_DESCRIPTOR_TYPE";
		case Result::ERROR_GR_INVALID_DESCRIPTOR_TYPE               : return "ERROR_GR_INVALID_DESCRIPTOR_TYPE";
		case Result::ERROR_GR_DESCRIPTOR_COUNT_EXCEEDED             : return "ERROR_GR_DESCRIPTOR_COUNT_EXCEEDED";
		case Result::ERROR_GR_BINDING_NOT_IN_SET                    : return "ERROR_GR_BINDING_NOT_IN_SET";
		case Result::ERROR_GR_NON_UNIQUE_SET                        : return "ERROR_GR_NON_UNIQUE_SET";
		case Result::ERROR_GR_MINIMUM_BUFFER_SIZE_NOT_MET           : return "ERROR_GR_MINIMUM_BUFFER_SIZE_NOT_MET";
		case Result::ERROR_GR_INVALID_SHADER_BYTE_CODE              : return "ERROR_GR_INVALID_SHADER_BYTE_CODE";
		case Result::ERROR_GR_INVALID_PIPELINE_INTERFACE            : return "ERROR_GR_INVALID_PIPELINE_INTERFACE";
		case Result::ERROR_GR_INVALID_QUERY_TYPE                    : return "ERROR_GR_INVALID_QUERY_TYPE";
		case Result::ERROR_GR_INVALID_QUERY_COUNT                   : return "ERROR_GR_INVALID_QUERY_COUNT";
		case Result::ERROR_GR_NO_QUEUES_AVAILABLE                   : return "ERROR_GR_NO_QUEUES_AVAILABLE";
		case Result::ERROR_GR_INVALID_INDEX_TYPE                    : return "ERROR_GR_INVALID_INDEX_TYPE";
		case Result::ERROR_GR_INVALID_GEOMETRY_CONFIGURATION        : return "ERROR_GR_INVALID_GEOMETRY_CONFIGURATION ";
		case Result::ERROR_GR_INVALID_VERTEX_ATTRIBUTE_COUNT        : return "ERROR_GR_INVALID_VERTEX_ATTRIBUTE_COUNT ";
		case Result::ERROR_GR_INVALID_VERTEX_ATTRIBUTE_STRIDE       : return "ERROR_GR_INVALID_VERTEX_ATTRIBUTE_STRIDE";
		case Result::ERROR_GR_NON_UNIQUE_BINDING                    : return "ERROR_GR_NON_UNIQUE_BINDING";
		case Result::ERROR_GR_INVALID_BINDING_NUMBER                : return "ERROR_GR_INVALID_BINDING_NUMBER";
		case Result::ERROR_GR_INVALID_SET_NUMBER                    : return "ERROR_GR_INVALID_SET_NUMBER";
		case Result::ERROR_GR_OPERATION_NOT_PERMITTED               : return "ERROR_GR_OPERATION_NOT_PERMITTED";
		case Result::ERROR_GR_INVALID_SEMAPHORE_TYPE                : return "ERROR_GR_INVALID_SEMAPHORE_TYPE";

		case Result::ERROR_IMAGE_FILE_LOAD_FAILED                     : return "ERROR_IMAGE_FILE_LOAD_FAILED";
		case Result::ERROR_IMAGE_FILE_SAVE_FAILED                     : return "ERROR_IMAGE_FILE_SAVE_FAILED";
		case Result::ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE       : return "ERROR_IMAGE_CANNOT_RESIZE_EXTERNAL_STORAGE";
		case Result::ERROR_IMAGE_INVALID_FORMAT                       : return "ERROR_IMAGE_INVALID_FORMAT";
		case Result::ERROR_IMAGE_RESIZE_FAILED                        : return "ERROR_IMAGE_RESIZE_FAILED";
		case Result::ERROR_BITMAP_CREATE_FAILED                       : return "ERROR_BITMAP_CREATE_FAILED";
		case Result::ERROR_BITMAP_BAD_COPY_SOURCE                     : return "ERROR_BITMAP_BAD_COPY_SOURCE";
		case Result::ERROR_BITMAP_FOOTPRINT_MISMATCH                  : return "ERROR_BITMAP_FOOTPRINT_MISMATCH";

		case Result::ERROR_GEOMETRY_NO_INDEX_DATA                     : return "ERROR_GEOMETRY_NO_INDEX_DATA";
		case Result::ERROR_GEOMETRY_FILE_LOAD_FAILED                  : return "ERROR_GEOMETRY_FILE_LOAD_FAILED";
		case Result::ERROR_GEOMETRY_FILE_NO_DATA                      : return "ERROR_GEOMETRY_FILE_NO_DATA";
		case Result::ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC           : return "ERROR_GEOMETRY_INVALID_VERTEX_SEMANTIC";

		case Result::ERROR_WINDOW_EVENTS_ALREADY_REGISTERED           : return "ERROR_WINDOW_EVENTS_ALREADY_REGISTERED";
		case Result::ERROR_IMGUI_INITIALIZATION_FAILED                : return "ERROR_IMGUI_INITIALIZATION_FAILED";

		case Result::ERROR_FONT_PARSE_FAILED                          : return "ERROR_FONT_PARSE_FAILED";
		case Result::ERROR_INVALID_UTF8_STRING                        : return "ERROR_INVALID_UTF8_STRING";
	}
	return "<unknown Result value>";
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

#pragma region ObjectPtr

class ObjPtrRefBase
{
public:
	ObjPtrRefBase() = default;
	virtual ~ObjPtrRefBase() = default;

private:
	friend class ObjPtrBase;
	virtual void set(void** ppObj) = 0;
};

class ObjPtrBase
{
public:
	ObjPtrBase() = default;
	virtual ~ObjPtrBase() = default;

protected:
	void set(void** ppObj, ObjPtrRefBase* pObjRef) const
	{
		pObjRef->set(ppObj);
	}
};

template <typename ObjectT>
class ObjPtrRef : public ObjPtrRefBase
{
public:
	ObjPtrRef(ObjectT** ptrRef) : m_ptrRef(ptrRef) {}
	~ObjPtrRef() = default;

	operator void** () const
	{
		void** addr = reinterpret_cast<void**>(m_ptrRef);
		return addr;
	}

	operator ObjectT** ()
	{
		return m_ptrRef;
	}

private:
	virtual void set(void** ppObj) override
	{
		*m_ptrRef = reinterpret_cast<ObjectT*>(*ppObj);
	}

	ObjectT** m_ptrRef = nullptr;
};

template <typename ObjectT>
class ObjPtr : public ObjPtrBase
{
public:
	using object_type = ObjectT;

	ObjPtr(ObjectT* ptr = nullptr) : m_ptr(ptr) {}
	~ObjPtr() = default;

	ObjPtr& operator=(const ObjPtr& rhs)
	{
		if (&rhs != this) m_ptr = rhs.m_ptr;
		return *this;
	}

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

	void Detach()
	{
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

#pragma region Log

constexpr const auto LOG_DEFAULT_PATH = "Log.txt";

std::ostream& operator<<(std::ostream& os, const float2& i);
std::ostream& operator<<(std::ostream& os, const float3& i);
std::ostream& operator<<(std::ostream& os, const float4& i);
std::ostream& operator<<(std::ostream& os, const uint3& i);

enum LogMode
{
	LOG_MODE_OFF = 0x0,
	LOG_MODE_CONSOLE = 0x1,
	LOG_MODE_FILE = 0x2,
};

enum LogLevel
{
	LOG_LEVEL_DEFAULT = 0x0,
	LOG_LEVEL_INFO = 0x1,
	LOG_LEVEL_WARN = 0x2,
	LOG_LEVEL_DEBUG = 0x3,
	LOG_LEVEL_ERROR = 0x4,
	LOG_LEVEL_FATAL = 0x5,
};

class Log final
{
public:
	Log() = default;
	~Log();

	static bool Initialize(uint32_t modes, const char* filePath = nullptr, std::ostream* consoleStream = &std::cout);
	static void Shutdown();

	static Log* Get();

	static bool IsActive();
	static bool IsModeActive(LogMode mode);

	void Lock();
	void Unlock();
	void Flush(LogLevel level);

	template <typename T>
	Log& operator<<(const T& value)
	{
		m_buffer << value;
		return *this;
	}

	Log& operator<<(std::ostream& (*manip)(std::ostream&))
	{
		m_buffer << *manip;
		return *this;
	}

private:
	bool createObjects(uint32_t mode, const char* filePath, std::ostream* consoleStream);
	void destroyObjects();

	void write(const char* msg, LogLevel level);

	uint32_t          m_modes = LOG_MODE_OFF;
	std::string       m_filePath;
	std::ofstream     m_fileStream;
	std::ostream* m_consoleStream = nullptr;
	std::stringstream m_buffer;
	std::mutex        m_writeMutex;
};

#define LOG_RAW(MSG)                          \
    if (Log::IsActive()) {                    \
        Log::Get()->Lock();                   \
        (*Log::Get()) << MSG << std::endl;    \
        Log::Get()->Flush(LOG_LEVEL_DEFAULT); \
        Log::Get()->Unlock();                 \
    }

#define LOG_INFO(MSG)                      \
    if (Log::IsActive()) {                 \
        Log::Get()->Lock();                \
        (*Log::Get()) << MSG << std::endl; \
        Log::Get()->Flush(LOG_LEVEL_INFO); \
        Log::Get()->Unlock();              \
    }

#define LOG_WARN(MSG)                      \
    if (Log::IsActive()) {                 \
        Log::Get()->Lock();                \
        (*Log::Get()) << MSG << std::endl; \
        Log::Get()->Flush(LOG_LEVEL_WARN); \
        Log::Get()->Unlock();              \
    }

#define LOG_WARN_ONCE(MSG)                     \
    if (Log::IsActive()) {                     \
        Log::Get()->Lock();                    \
        static bool LogWarnOnce = false;       \
        if(!LogWarnOnce) {                     \
            (*Log::Get()) << MSG << std::endl; \
        }                                      \
        LogWarnOnce = true;                    \
        Log::Get()->Flush(LOG_LEVEL_WARN);     \
        Log::Get()->Unlock();                  \
    }

#define LOG_DEBUG(MSG)                      \
    if (Log::IsActive()) {                  \
        Log::Get()->Lock();                 \
        (*Log::Get()) << MSG << std::endl;  \
        Log::Get()->Flush(LOG_LEVEL_DEBUG); \
        Log::Get()->Unlock();               \
    }

#define LOG_ERROR(MSG)                      \
    if (Log::IsActive()) {                  \
        Log::Get()->Lock();                 \
        (*Log::Get()) << MSG << std::endl;  \
        Log::Get()->Flush(LOG_LEVEL_ERROR); \
        Log::Get()->Unlock();               \
    }

#define LOG_FATAL(MSG)                      \
    if (Log::IsActive()) {                  \
        Log::Get()->Lock();                 \
        (*Log::Get()) << MSG << std::endl;  \
        Log::Get()->Flush(LOG_LEVEL_FATAL); \
        Log::Get()->Unlock();               \
    }

#define ASSERT_MSG(COND, MSG)                                              \
    if ((COND) == false) {                                                 \
        LOG_RAW(NE_ENDL                                                    \
            << "*** ASSERT ***" << NE_ENDL                                 \
            << "Message   : " << MSG << " " << NE_ENDL                     \
            << "Condition : " << #COND << " " << NE_ENDL                   \
            << "Function  : " << __FUNCTION__ << NE_ENDL                   \
            << "Location  : " << __FILE__ << " : " << NE_LINE << NE_ENDL); \
        assert(false);                                                     \
    }

#define ASSERT_NULL_ARG(ARG)                                               \
    if ((ARG) == nullptr) {                                                \
        LOG_RAW(NE_ENDL                                                    \
            << "*** NULL ARGUMNET ***" << NE_ENDL                          \
            << "Argument  : " << #ARG << " " << NE_ENDL                    \
            << "Function  : " << __FUNCTION__ << NE_ENDL                   \
            << "Location  : " << __FILE__ << " : " << NE_LINE << NE_ENDL); \
        assert(false);                                                     \
    }

#define CHECKED_CALL(EXPR)                                                        \
    {                                                                             \
        Result _checked_result_0xdeadbeef = EXPR;                                 \
        if (_checked_result_0xdeadbeef != SUCCESS) {                              \
            LOG_RAW(NE_ENDL                                                       \
                << "*** Engine Call Failed ***" << NE_ENDL                        \
                << "Return     : " << checked_result_0xdeadbeef << " " << NE_ENDL \
                << "Expression : " << #EXPR << " " << NE_ENDL                     \
                << "Function   : " << __FUNCTION__ << NE_ENDL                     \
                << "Location   : " << __FILE__ << " : " << NE_LINE << NE_ENDL);   \
            assert(false);                                                        \
        }                                                                         \
    }

#pragma endregion

#pragma region String Utils

namespace string_util 
{
	// -------------------------------------------------------------------------------------------------
	// Misc
	// -------------------------------------------------------------------------------------------------

	std::string ToLowerCopy(const std::string& s);
	std::string ToUpperCopy(const std::string& s);

	void TrimLeft(std::string& s);
	void TrimRight(std::string& s);

	std::string TrimCopy(const std::string& s);

	// Trims all characters specified in c from both the left and right sides of s
	std::string_view TrimBothEnds(std::string_view s, std::string_view c = " \t");

	// Splits s at the first instance of delimiter and returns two substrings
	// Returns an empty second element if there is no delimiter
	std::pair<std::string_view, std::string_view> SplitInTwo(std::string_view s, char delimiter);

	// -------------------------------------------------------------------------------------------------
	// Formatting Strings
	// -------------------------------------------------------------------------------------------------

	// Formats string for printing with the specified width and left indent.
	// Words will be pushed to the subsequent line to avoid line breaks in the
	// middle of a word if possible.
	// Leading and trailing whitespace is trimmed from each line.
	std::string WrapText(const std::string& s, size_t width, size_t indent = 0);

	// Provides string representation of value for printing or display
	template <typename T>
	std::string ToString(T value)
	{
		std::stringstream ss;
		if constexpr (std::is_same_v<T, bool>)
		{
			ss << std::boolalpha;
		}
		ss << value;
		return ss.str();
	}

	// Same as above, for vectors
	template <typename T>
	std::string ToString(std::vector<T> values)
	{
		std::stringstream ss;
		for (const auto& value : values)
		{
			ss << ToString<T>(value) << ", ";
		}
		std::string valueStr = ss.str();
		return valueStr.substr(0, valueStr.length() - 2);
	}

	// Same as above, for pairs
	template <typename T>
	std::string ToString(std::pair<T, T> values)
	{
		std::stringstream ss;
		ss << ToString<T>(values.first) << ", " << ToString<T>(values.second);
		return ss.str();
	}

	// -------------------------------------------------------------------------------------------------
	// Parsing Strings
	// -------------------------------------------------------------------------------------------------

	// Parse() attempts to parse valueStr into the specified type
	// If successful, overwrites parsedValue and returns std::nullopt
	// If unsucessful, does not overwrite parsedValue, logs error and returns ERROR_FAILED instead

	// For strings
	// e.g. "a string" -> "a string"
	Result Parse(std::string_view valueStr, std::string& parsedValue);

	// For bool
	// e.g. "true", "1", "" -> true
	// e.g. "false", "0" -> false
	Result Parse(std::string_view valueStr, bool& parsedValue);

	// For integers, chars and floats
	// e.g. "1.0" -> 1.0f
	// e.g. "-20" -> -20
	// e.g. "c" -> 'c'
	template <typename T>
	Result Parse(std::string_view valueStr, T& parsedValue)
	{
		static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "attempting to parse unsupported type");

		std::stringstream ss((std::string(valueStr)));
		T                 valueAsNum;
		ss >> valueAsNum;
		if (ss.fail())
		{
			LOG_ERROR("could not be parsed as integral or float: " << valueStr);
			return ERROR_FAILED;
		}
		parsedValue = valueAsNum;
		return SUCCESS;
	}

	// For resolution with x-separated string representation
	// e.g. "600x800" -> (600, 800)
	Result Parse(std::string_view valueStr, std::pair<int, int>& parsedValues);

} // namespace string_util

#pragma endregion

#pragma region CpuInfo

class CpuInfo final
{
public:
	struct Features final
	{
		bool sse                 = false;
		bool sse2                = false;
		bool sse3                = false;
		bool ssse3               = false;
		bool sse4_1              = false;
		bool sse4_2              = false;
		bool sse4a               = false;
		bool avx                 = false;
		bool avx2                = false;
		bool avx512f             = false;
		bool avx512cd            = false;
		bool avx512er            = false;
		bool avx512pf            = false;
		bool avx512bw            = false;
		bool avx512dq            = false;
		bool avx512vl            = false;
		bool avx512ifma          = false;
		bool avx512vbmi          = false;
		bool avx512vbmi2         = false;
		bool avx512vnni          = false;
		bool avx512bitalg        = false;
		bool avx512vpopcntdq     = false;
		bool avx512_4vnniw       = false;
		bool avx512_4vbmi2       = false;
		bool avx512_second_fma   = false;
		bool avx512_4fmaps       = false;
		bool avx512_bf16         = false;
		bool avx512_vp2intersect = false;
		bool amx_bf16            = false;
		bool amx_tile            = false;
		bool amx_int8            = false;
	};

	CpuInfo() = default;
	~CpuInfo() = default;

	const char*     GetBrandString() const { return m_brandString.c_str(); }
	const char*     GetVendorString() const { return m_vendorString.c_str(); }
	const char*     GetMicroarchitectureString() const { return m_microarchitectureString.c_str(); }
	uint32_t        GetL1CacheSize() const { return m_L1CacheSize; }
	uint32_t        GetL2CacheSize() const { return m_L2CacheSize; }
	uint32_t        GetL3CacheSize() const { return m_L3CacheSize; }
	uint32_t        GetL1CacheLineSize() const { return m_L1CacheLineSize; }
	uint32_t        GetL2CacheLineSize() const { return m_L2CacheLineSize; }
	uint32_t        GetL3CacheLineSize() const { return m_L3CacheLineSize; }
	const Features& GetFeatures() const { return m_features; }

private:
	friend CpuInfo getX86CpuInfo();

	std::string m_brandString;
	std::string m_vendorString;
	std::string m_microarchitectureString;
	uint32_t    m_L1CacheSize = 0;
	uint32_t    m_L2CacheSize = 0;
	uint32_t    m_L3CacheSize = 0;
	uint32_t    m_L1CacheLineSize = 0;
	uint32_t    m_L2CacheLineSize = 0;
	uint32_t    m_L3CacheLineSize = 0;
	Features    m_features = { 0 };
};

#pragma endregion

#pragma region Platform Info

enum PlatformId
{
	PLATFORM_ID_UNDEFINED = 0,
	PLATFORM_ID_LINUX,
	PLATFORM_ID_MSW,
};

class Platform final
{
public:
	Platform();

	static PlatformId     GetPlatformId();
	static const char*    GetPlatformString();
	static const CpuInfo& GetCpuInfo();

private:
	CpuInfo m_cpuInfo;
};

#pragma endregion

#pragma region Timer

#define TIMER_SECONDS_TO_NANOS 1000000000
#define TIMER_MILLIS_TO_NANOS  1000000
#define TIMER_MICROS_TO_NANOS  1000
#define TIMER_NANOS_TO_SECONDS 0.000000001
#define TIMER_NANOS_TO_MILLIS  0.000001
#define TIMER_NANOS_TO_MICROS  0.001

// Select which clock to use
#if defined(TIMER_FORCE_MONOTONIC)
#	define TIMER_CLK_ID CLOCK_MONOTONIC
#else
#	define TIMER_CLK_ID CLOCK_MONOTONIC_RAW
#endif

enum TimerResult
{
	TIMER_RESULT_SUCCESS                 = 0,
	TIMER_RESULT_ERROR_NULL_POINTER      = -1,
	TIMER_RESULT_ERROR_INITIALIZE_FAILED = -2,
	TIMER_RESULT_ERROR_TIMESTAMP_FAILED  = -3,
	TIMER_RESULT_ERROR_SLEEP_FAILED      = -4,
	TIMER_RESULT_ERROR_CORRUPTED_DATA    = -5,
};

class Timer final
{
public:
	static TimerResult InitializeStaticData();

	static TimerResult Timestamp(uint64_t* pTimestamp);
	static double      TimestampToSeconds(uint64_t timestamp);
	static double      TimestampToMillis(uint64_t timestamp);
	static double      TimestampToMicros(uint64_t timestamp);
	static double      TimestampToNanos(uint64_t timestamp);

	static TimerResult SleepSeconds(double seconds);
	static TimerResult SleepMillis(double millis);
	static TimerResult SleepMicros(double micros);
	static TimerResult SleepNanos(double nanos);

	TimerResult Start();
	TimerResult Stop();
	uint64_t    StartTimestamp() const;
	uint64_t    StopTimestamp() const;
	double      SecondsSinceStart() const;
	double      MillisSinceStart() const;
	double      MicrosSinceStart() const;
	double      NanosSinceStart() const;

private:
	bool     m_initialized = false;
	uint64_t m_startTimestamp = 0;
	uint64_t m_stopTimestamp = 0;
};

class ScopedTimer
{
public:
	ScopedTimer(const std::string& message);
	~ScopedTimer();

	ScopedTimer(const ScopedTimer&) = delete;
	ScopedTimer(ScopedTimer&&) = delete;
private:
	Timer             m_timer;
	const std::string m_message;
};

#pragma endregion

#pragma region File System

namespace fs
{
#if defined(_ANDROID)
	void set_android_context(android_app* androidContext);
#endif

	// Abstract a static, regular file on all platforms.
	// This class doesn't handle sockets, or file which content is not constant
	// for the lifetime of this class.
	class File
	{
		// Internal enum to know which handle is currently valid.
		enum FileHandleType
		{
			// Default handle, no file associated.
			BAD_HANDLE = 0,
			// The file is accessible through a stream.
			STREAM_HANDLE = 1,
			// The file is accessible through an Android asset handle.
			ASSET_HANDLE = 2,
		};

	public:
		File();
		File(const File& other) = delete;
		File(const File&& other) = delete;
		~File();

		// Opens a file given a specific path.
		// path: the path of the file to open.
		//  - On desktop, loads the regular file at `path`. (memory-mapping availability is implementation defined).
		//  - On Android, relative path are assumed to be loaded from the APK, those are memory mapped.
		//                absolute path are loaded as regular files (mapping availability is implementation defined).
		//
		// - This API only supports regular files.
		// - This API expects the file not to change size or content while this handle is open.
		// - This class supports RAII. File will be closed on destroy.
		bool Open(const std::filesystem::path& path);

		// Reads `size` bytes from the file into `buffer`.
		// buffer: a pointer to a buffer with at least `count` writable bytes.
		// count: the maximum number of bytes to write to `buffer`.
		// Returns the number of bytes written to `buffer`.
		//
		// - The file has an internal cursor, meaning the next read will start at the end of the last read.
		// - If the file size is larger than `count`, the read stops at `count` bytes.
		size_t Read(void* buffer, size_t count);

		// Returns true if the file is readable.
		bool IsValid() const;
		// Returns true if the file is mapped in memory. See `File::GetMappedData()`.
		bool IsMapped() const;

		// Returns the total size in bytes of the file from the start.
		size_t GetLength() const;
		// Returns a readable pointer to a beginning of the file. Behavior undefined if `File::IsMapped()` is false.
		const void* GetMappedData() const;

	private:
#if !defined(_ANDROID)
		typedef void AAsset;
#endif

		FileHandleType m_handleType = BAD_HANDLE;
		AAsset*        m_asset = nullptr;
		const void*    m_buffer = nullptr;
		std::ifstream  m_stream;
		size_t         m_fileSize = 0;
		size_t         m_fileOffset = 0;
	};

	class FileStream : public std::streambuf
	{
	public:
		bool Open(const char* path);

	private:
		std::vector<char> m_buffer;
	};

	// Opens a regular file and returns its content if the read succeeded.
	// `path`: the path of the file to load.
	// The path is handled differently depending on the platform:
	//  - desktop: all paths are treated the same.
	//  - android: relative paths are assumed to be in APK's storage (Asset API). Absolute are loaded from disk.
	std::optional<std::vector<char>> load_file(const std::filesystem::path& path);

	// Returns true if a given path exists (file or directory).
	// `path`: the path to check.
	// The path is handled differently depending on the platform:
	//  - desktop: all path are treated the same.
	//  - android: relative path are assumed to be in APK's storage (Asset API). Absolute are loaded from disk.
	bool path_exists(const std::filesystem::path& path);

#if defined(_ANDROID)
	// Returns a path to the application's internal data directory (can be used for output).
	// NOTE: The internal data path on Android is extremely limited in terms of filesize!
	std::filesystem::path GetInternalDataPath();

	// Returns a path to the application's external data directory (can be used for output).
	std::filesystem::path GetExternalDataPath();
#endif

	// Returns the default output directory where artifacts will be written.
	std::filesystem::path GetDefaultOutputDirectory();

	// Constructs the full path from a possibly partial path
	// If `regexToReplace` is specified, replace all instances of that symbol in the filename with `replaceString`
	std::filesystem::path GetFullPath(const std::filesystem::path& partialPath, const std::filesystem::path& defaultFolder, const std::string& regexToReplace = "", const std::string& replaceString = "");

} // namespace fs

#pragma endregion

#pragma region Random

class Random
{
public:
	Random()
	{
		Seed(0xDEAD, 0xBEEF);
	}

	Random(uint64_t initialState, uint64_t initialSequence)
	{
		Seed(initialSequence, initialSequence);
	}

	void Seed(uint64_t initialState, uint64_t initialSequence)
	{
		m_rng.seed(initialSequence, initialSequence);
	}

	uint32_t UInt32()
	{
		uint32_t value = m_rng.nextUInt();
		return value;
	}

	float Float()
	{
		float value = m_rng.nextFloat();
		return value;
	}

	float Float(float a, float b)
	{
		float value = glm::lerp(a, b, Float());
		return value;
	}

	float2 Float2()
	{
		float2 value(
			Float(),
			Float());
		return value;
	}

	float2 Float2(const float2& a, const float2& b)
	{
		float2 value(
			Float(a.x, b.x),
			Float(a.y, b.y));
		return value;
	}

	float3 Float3()
	{
		float3 value(
			Float(),
			Float(),
			Float());
		return value;
	}

	float3 Float3(const float3& a, const float3& b)
	{
		float3 value(
			Float(a.x, b.x),
			Float(a.y, b.y),
			Float(a.z, b.z));
		return value;
	}

	float4 Float4()
	{
		float4 value(
			Float(),
			Float(),
			Float(),
			Float());
		return value;
	}

	float4 Float4(const float4& a, const float4& b)
	{
		float4 value(
			Float(a.x, b.x),
			Float(a.y, b.y),
			Float(a.z, b.z),
			Float(a.w, b.w));
		return value;
	}

private:
	pcg32 m_rng;
};

#pragma endregion

#pragma region Bounding Volume

class AABB;
class OBB;

class AABB
{
public:
	AABB() = default;
	AABB(const float3& pos)
	{
		Set(pos);
	}

	AABB(const float3& minPos, const float3& maxPos)
	{
		Set(minPos, maxPos);
	}

	AABB(const OBB& obb);


	AABB& operator=(const OBB& rhs);

	void Set(const float3& pos)
	{
		m_min = m_max = pos;
	}

	void Set(const float3& minPos, const float3& maxPos)
	{
		m_min = glm::min(minPos, maxPos);
		m_max = glm::max(minPos, maxPos);
	}

	void Set(const OBB& obb);

	void Expand(const float3& pos)
	{
		m_min = glm::min(pos, m_min);
		m_max = glm::max(pos, m_max);
	}

	const float3& GetMin() const
	{
		return m_min;
	}

	const float3& GetMax() const
	{
		return m_max;
	}

	float3 GetCenter() const
	{
		float3 center = (m_min + m_max) / 2.0f;
		return center;
	}

	float3 GetSize() const
	{
		float3 size = (m_max - m_min);
		return size;
	}

	float GetWidth() const
	{
		return (m_max.x - m_min.x);
	}

	float GetHeight() const
	{
		return (m_max.y - m_min.y);
	}

	float GetDepth() const
	{
		return (m_max.y - m_min.y);
	}

	float3 GetU() const
	{
		float3 P = float3(m_max.x, m_min.y, m_min.z);
		float3 U = glm::normalize(P - m_min);
		return U;
	}

	float3 GetV() const
	{
		float3 P = float3(m_min.x, m_max.y, m_min.z);
		float3 V = glm::normalize(P - m_min);
		return V;
	}

	float3 GetW() const
	{
		float3 P = float3(m_min.x, m_min.y, m_max.z);
		float3 W = glm::normalize(P - m_min);
		return W;
	}

	void Transform(const float4x4& matrix, float3 obbVertices[8]) const;

private:
	float3 m_min = float3(0, 0, 0);
	float3 m_max = float3(0, 0, 0);
};

class OBB
{
public:
	OBB() {}

	OBB(const float3& center, const float3& size, const float3& U, const float3& V, const float3& W)
		: m_center(center)
		, m_size(size)
		, m_u(glm::normalize(U))
		, m_v(glm::normalize(V))
		, m_w(glm::normalize(W)) {}

	OBB(const AABB& aabb);

	void Set(const AABB& aabb);

	const float3& GetPos() const
	{
		return m_center;
	}

	const float3& GetSize() const
	{
		return m_size;
	}

	const float3& GetU() const
	{
		return m_u;
	}

	const float3& GetV() const
	{
		return m_v;
	}

	const float3& GetW() const
	{
		return m_w;
	}

	void GetPoints(float3 obbVertices[8]) const;

private:
	float3 m_center = float3(0, 0, 0);
	float3 m_size = float3(0, 0, 0);
	float3 m_u = float3(1, 0, 0);
	float3 m_v = float3(0, 1, 0);
	float3 m_w = float3(0, 0, 1);
};

#pragma endregion

#pragma region Transform

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
	Transform(const float3& translation);
	virtual ~Transform() = default;

	bool IsDirty() const { return (m_dirty.mask != 0); }

	const float3& GetTranslation() const { return m_translation; }
	const float3& GetRotation() const { return m_rotation; }
	const float3& GetScale() const { return m_scale; }
	RotationOrder GetRotationOrder() const { return m_rotationOrder; }

	virtual void SetTranslation(const float3& value);
	void         SetTranslation(float x, float y, float z);
	virtual void SetRotation(const float3& value);
	void         SetRotation(float x, float y, float z);
	virtual void SetScale(const float3& value);
	void         SetScale(float x, float y, float z);
	virtual void SetRotationOrder(Transform::RotationOrder value);

	const float4x4& GetTranslationMatrix() const;
	const float4x4& GetRotationMatrix() const;
	const float4x4& GetScaleMatrix() const;
	const float4x4& GetConcatenatedMatrix() const;

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

	float3           m_translation = float3(0, 0, 0);
	float3           m_rotation = float3(0, 0, 0);
	float3           m_scale = float3(1, 1, 1);
	RotationOrder    m_rotationOrder = RotationOrder::XYZ;
	mutable float4x4 m_translationMatrix;
	mutable float4x4 m_rotationMatrix;
	mutable float4x4 m_scaleMatrix;
	mutable float4x4 m_concatenatedMatrix;
};


#pragma endregion

#pragma region Camera

#define CAMERA_DEFAULT_NEAR_CLIP      0.1f
#define CAMERA_DEFAULT_FAR_CLIP       10000.0f
#define CAMERA_DEFAULT_EYE_POSITION   float3(0, 0, 1)
#define CAMERA_DEFAULT_LOOK_AT        float3(0, 0, 0)
#define CAMERA_DEFAULT_WORLD_UP       float3(0, 1, 0)
#define CAMERA_DEFAULT_VIEW_DIRECTION float3(0, 0, -1)

enum CameraType
{
	CAMERA_TYPE_UNKNOWN = 0,
	CAMERA_TYPE_PERSPECTIVE = 1,
	CAMERA_TYPE_ORTHOGRAPHIC = 2,
};

class Camera
{
public:
	Camera(bool pixelAligned = false);

	Camera(float nearClip, float farClip, bool pixelAligned = false);

	virtual ~Camera() {}

	virtual CameraType GetCameraType() const = 0;

	float GetNearClip() const { return mNearClip; }
	float GetFarClip() const { return mFarClip; }

	virtual void LookAt(const float3& eye, const float3& target, const float3& up = CAMERA_DEFAULT_WORLD_UP);

	const float3& GetEyePosition() const { return mEyePosition; }
	const float3& GetTarget() const { return mTarget; }
	const float3& GetViewDirection() const { return mViewDirection; }
	const float3& GetWorldUp() const { return mWorldUp; }

	const float4x4& GetViewMatrix() const { return mViewMatrix; }
	const float4x4& GetProjectionMatrix() const { return mProjectionMatrix; }
	const float4x4& GetViewProjectionMatrix() const { return mViewProjectionMatrix; }

	float3 WorldToViewPoint(const float3& worldPoint) const;
	float3 WorldToViewVector(const float3& worldVector) const;

	void MoveAlongViewDirection(float distance);

protected:
	bool             mPixelAligned = false;
	float            mNearClip = CAMERA_DEFAULT_NEAR_CLIP;
	float            mFarClip = CAMERA_DEFAULT_FAR_CLIP;
	float3           mEyePosition = CAMERA_DEFAULT_EYE_POSITION;
	float3           mTarget = CAMERA_DEFAULT_LOOK_AT;
	float3           mViewDirection = CAMERA_DEFAULT_VIEW_DIRECTION;
	float3           mWorldUp = CAMERA_DEFAULT_WORLD_UP;
	mutable float4x4 mViewMatrix = float4x4(1);
	mutable float4x4 mProjectionMatrix = float4x4(1);
	mutable float4x4 mViewProjectionMatrix = float4x4(1);
	mutable float4x4 mInverseViewMatrix = float4x4(1);
};

class PerspCamera : public Camera
{
public:
	PerspCamera();

	explicit PerspCamera(
		float horizFovDegrees,
		float aspect,
		float nearClip = CAMERA_DEFAULT_NEAR_CLIP,
		float farClip = CAMERA_DEFAULT_FAR_CLIP);

	explicit PerspCamera(
		const float3& eye,
		const float3& target,
		const float3& up,
		float         horizFovDegrees,
		float         aspect,
		float         nearClip = CAMERA_DEFAULT_NEAR_CLIP,
		float         farClip = CAMERA_DEFAULT_FAR_CLIP);

	explicit PerspCamera(
		uint32_t pixelWidth,
		uint32_t pixelHeight,
		float    horizFovDegrees = 60.0f);

	// Pixel aligned camera
	explicit PerspCamera(
		uint32_t pixelWidth,
		uint32_t pixelHeight,
		float    horizFovDegrees,
		float    nearClip,
		float    farClip);

	virtual ~PerspCamera();

	virtual CameraType GetCameraType() const { return CAMERA_TYPE_PERSPECTIVE; }

	void SetPerspective(
		float horizFovDegrees,
		float aspect,
		float nearClip = CAMERA_DEFAULT_NEAR_CLIP,
		float farClip = CAMERA_DEFAULT_FAR_CLIP);

	void FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace);

private:
	float mHorizFovDegrees = 60.0f;
	float mVertFovDegrees = 36.98f;
	float mAspect = 1.0f;
};

class OrthoCamera : public Camera
{
public:
	OrthoCamera();

	OrthoCamera(
		float left,
		float right,
		float bottom,
		float top,
		float nearClip,
		float farClip);

	virtual ~OrthoCamera();

	virtual CameraType GetCameraType() const { return CAMERA_TYPE_ORTHOGRAPHIC; }

	void SetOrthographic(
		float left,
		float right,
		float bottom,
		float top,
		float nearClip,
		float farClip);

private:
	float mLeft = -1.0f;
	float mRight = 1.0f;
	float mBottom = -1.0f;
	float mTop = 1.0f;
};

// Adapted from: https://github.com/Twinklebear/arcball-cpp
class ArcballCamera : public PerspCamera
{
public:
	ArcballCamera();

	ArcballCamera(
		float horizFovDegrees,
		float aspect,
		float nearClip = CAMERA_DEFAULT_NEAR_CLIP,
		float farClip = CAMERA_DEFAULT_FAR_CLIP);

	ArcballCamera(
		const float3& eye,
		const float3& target,
		const float3& up,
		float         horizFovDegrees,
		float         aspect,
		float         nearClip = CAMERA_DEFAULT_NEAR_CLIP,
		float         farClip = CAMERA_DEFAULT_FAR_CLIP);

	virtual ~ArcballCamera() {}

	void LookAt(const float3& eye, const float3& target, const float3& up = CAMERA_DEFAULT_WORLD_UP) override;

	//! @fn void Rotate(const float2& prevPos, const float2& curPos)
	//!
	//! @param prevPos previous mouse position in normalized device coordinates
	//! @param curPos current mouse position in normalized device coordinates
	//!
	void Rotate(const float2& prevPos, const float2& curPos);

	//! @fn void Pan(const float2& delta)
	//!
	//! @param delta mouse delta in normalized device coordinates
	//!
	void Pan(const float2& delta);

	//! @fn void Zoom(float amount)
	//!
	void Zoom(float amount);

private:
	void UpdateCamera();

	float4x4 mCenterTranslationMatrix;
	float4x4 mTranslationMatrix;
	quat     mRotationQuat;
};
#pragma endregion
