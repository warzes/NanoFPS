#pragma once

// Import GLM types as HLSL friendly names
#pragma region Base type

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


