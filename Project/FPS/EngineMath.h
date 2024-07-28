#pragma once

[[nodiscard]] constexpr size_t RoundUp(size_t numberToRoundUp, size_t multipleOf)
{
	assert(multipleOf);
	return ((numberToRoundUp + multipleOf - 1) / multipleOf) * multipleOf;
}

[[nodiscard]] inline int NumMipmap(int width, int height)
{
	return static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;
}

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
	static float clampPitch(const float radians) { return glm::clamp(radians, -glm::half_pi<float>(), glm::half_pi<float>()); }

	// Wrap to (-PI..PI]
	static float wrapAngle(const float radians) { return std::remainder(radians, glm::two_pi<float>()); }

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
