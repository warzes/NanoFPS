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
	Transform(const glm::vec3& position, const glm::mat3& orientation);
	Transform(const glm::vec3& position, const glm::quat& orientation);

	[[nodiscard]] const glm::vec3& GetPosition() const;
	[[nodiscard]] const glm::quat& GetOrientation() const;

	void SetIdentity();
	void SetPosition(const glm::vec3& position);
	void SetOrientation(const glm::quat& orientation);

	[[nodiscard]] Transform GetInverse() const;

	glm::vec3 operator*(const glm::vec3& v) const;
	Transform operator*(const Transform& tr) const;

private:
	glm::vec3 m_position = glm::vec3(0.0f);
	glm::quat m_orientation = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
};

#pragma region inline Transform

inline Transform::Transform(const glm::vec3& position, const glm::mat3& orientation)
	: m_position(position)
	, m_orientation(glm::quat(orientation))
{
}

inline Transform::Transform(const glm::vec3& position, const glm::quat& orientation)
	: m_position(position)
	, m_orientation(orientation)
{
}

inline const glm::vec3& Transform::GetPosition() const
{
	return m_position;
}

inline const glm::quat& Transform::GetOrientation() const
{
	return m_orientation;
}

inline void Transform::SetIdentity()
{
	m_position = glm::vec3(0.0f);
	m_orientation = glm::quat::wxyz(1.0f, 0.0f, 0.0f, 0.0f);
}

inline void Transform::SetPosition(const glm::vec3& position)
{
	m_position = position;
}

inline void Transform::SetOrientation(const glm::quat& orientation)
{
	m_orientation = orientation;
}

inline Transform Transform::GetInverse() const
{
	const glm::quat invOrientation = glm::inverse(m_orientation);
	return Transform(invOrientation * (-m_position), invOrientation);
}

inline glm::vec3 Transform::operator*(const glm::vec3& v) const
{
	return (m_orientation * v) + m_position;
}

inline Transform Transform::operator*(const Transform& transform2) const
{
	// The following code is equivalent to this
	//return Transform(mPosition + mOrientation * transform2.mPosition,
	//                 mOrientation * transform2.mOrientation);

	const float prodX = m_orientation.w * transform2.m_position.x + m_orientation.y * transform2.m_position.z
		- m_orientation.z * transform2.m_position.y;
	const float prodY = m_orientation.w * transform2.m_position.y + m_orientation.z * transform2.m_position.x
		- m_orientation.x * transform2.m_position.z;
	const float prodZ = m_orientation.w * transform2.m_position.z + m_orientation.x * transform2.m_position.y
		- m_orientation.y * transform2.m_position.x;
	const float prodW = -m_orientation.x * transform2.m_position.x - m_orientation.y * transform2.m_position.y
		- m_orientation.z * transform2.m_position.z;

	return Transform(glm::vec3(
		m_position.x + m_orientation.w * prodX - prodY * m_orientation.z + prodZ * m_orientation.y - prodW * m_orientation.x,
		m_position.y + m_orientation.w * prodY - prodZ * m_orientation.x + prodX * m_orientation.z - prodW * m_orientation.y,
		m_position.z + m_orientation.w * prodZ - prodX * m_orientation.y + prodY * m_orientation.x - prodW * m_orientation.z),

		glm::quat::wxyz(m_orientation.w * transform2.m_orientation.x + transform2.m_orientation.w * m_orientation.x
			+ m_orientation.y * transform2.m_orientation.z - m_orientation.z * transform2.m_orientation.y,
			m_orientation.w * transform2.m_orientation.y + transform2.m_orientation.w * m_orientation.y
			+ m_orientation.z * transform2.m_orientation.x - m_orientation.x * transform2.m_orientation.z,
			m_orientation.w * transform2.m_orientation.z + transform2.m_orientation.w * m_orientation.z
			+ m_orientation.x * transform2.m_orientation.y - m_orientation.y * transform2.m_orientation.x,
			m_orientation.w * transform2.m_orientation.w - m_orientation.x * transform2.m_orientation.x
			- m_orientation.y * transform2.m_orientation.y - m_orientation.z * transform2.m_orientation.z));
}

#pragma endregion