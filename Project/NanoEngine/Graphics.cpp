#include "stdafx.h"
#include "Core.h"
#include "Graphics.h"

#pragma region Camera

Camera::Camera(bool pixelAligned)
	: m_pixelAligned(pixelAligned)
{
	LookAt(CAMERA_DEFAULT_EYE_POSITION, CAMERA_DEFAULT_LOOK_AT, CAMERA_DEFAULT_WORLD_UP);
}

Camera::Camera(float nearClip, float farClip, bool pixelAligned)
	: m_pixelAligned(pixelAligned)
	, m_nearClip(nearClip)
	, m_farClip(farClip)
{
}

void Camera::LookAt(const float3& eye, const float3& target, const float3& up)
{
	const float3 yAxis = m_pixelAligned ? float3(1, -1, 1) : float3(1, 1, 1);
	m_eyePosition = eye;
	m_target = target;
	m_worldUp = up;
	m_viewDirection = glm::normalize(m_target - m_eyePosition);
	m_viewMatrix = glm::scale(yAxis) * glm::lookAt(m_eyePosition, m_target, m_worldUp);
	m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
	m_inverseViewMatrix = glm::inverse(m_viewMatrix);
}

float3 Camera::WorldToViewPoint(const float3& worldPoint) const
{
	float3 viewPoint = float3(m_viewMatrix * float4(worldPoint, 1.0f));
	return viewPoint;
}

float3 Camera::WorldToViewVector(const float3& worldVector) const
{
	float3 viewPoint = float3(m_viewMatrix * float4(worldVector, 0.0f));
	return viewPoint;
}

void Camera::MoveAlongViewDirection(float distance)
{
	float3 eyePosition = m_eyePosition + (distance * m_viewDirection);
	LookAt(eyePosition, m_target, m_worldUp);
}

PerspectiveCamera::PerspectiveCamera(float horizFovDegrees, float aspect, float nearClip, float farClip)
	: Camera(nearClip, farClip)
{
	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
}

PerspectiveCamera::PerspectiveCamera(const float3& eye, const float3& target, const float3& up, float horizFovDegrees, float aspect, float nearClip, float farClip)
	: Camera(nearClip, farClip)
{
	LookAt(eye, target, up);

	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
}

PerspectiveCamera::PerspectiveCamera(uint32_t pixelWidth, uint32_t pixelHeight, float horizFovDegrees)
	: Camera(true)
{
	float aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
	float eyeX = static_cast<float>(pixelWidth) / 2.0f;
	float eyeY = static_cast<float>(pixelHeight) / 2.0f;
	float halfFov = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;
	float nearClip = dist / 10.0f;
	float farClip = dist * 10.0f;

	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
	LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

PerspectiveCamera::PerspectiveCamera(uint32_t pixelWidth, uint32_t pixelHeight, float horizFovDegrees, float nearClip, float farClip)
	: Camera(nearClip, farClip, true)
{
	float aspect = static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight);
	float eyeX = static_cast<float>(pixelWidth) / 2.0f;
	float eyeY = static_cast<float>(pixelHeight) / 2.0f;
	float halfFov = atan(tan(glm::radians(horizFovDegrees) / 2.0f) / aspect); // horiz fov -> vert fov
	float theTan = tanf(halfFov);
	float dist = eyeY / theTan;

	SetPerspective(horizFovDegrees, aspect, nearClip, farClip);
	LookAt(float3(eyeX, eyeY, dist), float3(eyeX, eyeY, 0.0f));
}

void PerspectiveCamera::SetPerspective(float horizFovDegrees, float aspect, float nearClip, float farClip)
{
	m_horizFovDegrees = horizFovDegrees;
	m_aspect = aspect;
	m_nearClip = nearClip;
	m_farClip = farClip;

	float horizFovRadians = glm::radians(m_horizFovDegrees);
	float vertFovRadians = 2.0f * atan(tan(horizFovRadians / 2.0f) / m_aspect);
	m_vertFovDegrees = glm::degrees(vertFovRadians);

	m_projectionMatrix = glm::perspective(vertFovRadians, m_aspect, m_nearClip, m_farClip);
	m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;
}

void PerspectiveCamera::FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace)
{
	float3   min = bboxMinWorldSpace;
	float3   max = bbxoMaxWorldSpace;
	float3   target = (min + max) / 2.0f;
	float3   up = glm::normalize(m_inverseViewMatrix * float4(0, 1, 0, 0));
	float4x4 viewSpaceMatrix = glm::lookAt(m_eyePosition, target, up);

	// World space oriented bounding box
	float3 obb[8] = {
		{min.x, max.y, min.z},
		{min.x, min.y, min.z},
		{max.x, min.y, min.z},
		{max.x, max.y, min.z},
		{min.x, max.y, max.z},
		{min.x, min.y, max.z},
		{max.x, min.y, max.z},
		{max.x, max.y, max.z},
	};

	// Tranform obb from world space to view space
	for (uint32_t i = 0; i < 8; ++i)
		obb[i] = viewSpaceMatrix * float4(obb[i], 1.0f);

	// Get aabb from obb in view space
	min = max = obb[0];
	for (uint32_t i = 1; i < 8; ++i)
	{
		min = glm::min(min, obb[i]);
		max = glm::max(max, obb[i]);
	}

	// Get x,y extent max
	float xmax = glm::max(glm::abs(min.x), glm::abs(max.x));
	float ymax = glm::max(glm::abs(min.y), glm::abs(max.y));
	float rad = glm::max(xmax, ymax);
	float fov = m_aspect < 1.0f ? m_horizFovDegrees : m_vertFovDegrees;

	// Calculate distance
	float dist = rad / tan(glm::radians(fov / 2.0f));

	// Calculate eye position
	float3 dir = glm::normalize(m_eyePosition - target);
	float3 eye = target + (dist + m_nearClip) * dir;

	// Adjust camera look at
	LookAt(eye, target, up);
}

OrthoCamera::OrthoCamera(float left, float right, float bottom, float top, float nearClip, float farClip)
{
	SetOrthographic(left, right, bottom, top, nearClip, farClip);
}

void OrthoCamera::SetOrthographic(float left, float right, float bottom, float top, float nearClip, float farClip)
{
	m_left = left;
	m_right = right;
	m_bottom = bottom;
	m_top = top;
	m_nearClip = nearClip;
	m_farClip = farClip;

	m_projectionMatrix = glm::ortho(m_left, m_right, m_bottom, m_top, m_nearClip, m_farClip);
}

ArcballCamera::ArcballCamera(float horizFovDegrees, float aspect, float nearClip, float farClip)
	: PerspectiveCamera(horizFovDegrees, aspect, nearClip, farClip)
{
}

ArcballCamera::ArcballCamera(const float3& eye, const float3& target, const float3& up, float horizFovDegrees, float aspect, float nearClip, float farClip)
	: PerspectiveCamera(eye, target, up, horizFovDegrees, aspect, nearClip, farClip)
{
}

void ArcballCamera::updateCamera()
{
	m_viewMatrix = m_translationMatrix * glm::mat4_cast(m_rotationQuat) * m_centerTranslationMatrix;
	m_inverseViewMatrix = glm::inverse(m_viewMatrix);
	m_viewProjectionMatrix = m_projectionMatrix * m_viewMatrix;

	// Transform the view space origin into world space for eye position
	m_eyePosition = m_inverseViewMatrix * float4(0, 0, 0, 1);
}

void ArcballCamera::LookAt(const float3& eye, const float3& target, const float3& up)
{
	Camera::LookAt(eye, target, up);

	float3 viewDir = target - eye;
	float3 zAxis = glm::normalize(viewDir);
	float3 xAxis = glm::normalize(glm::cross(zAxis, glm::normalize(up)));
	float3 yAxis = glm::normalize(glm::cross(xAxis, zAxis));
	xAxis = glm::normalize(glm::cross(zAxis, yAxis));

	m_centerTranslationMatrix = glm::inverse(glm::translate(target));
	m_translationMatrix = glm::translate(float3(0.0f, 0.0f, -glm::length(viewDir)));
	m_rotationQuat = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(xAxis, yAxis, -zAxis))));

	updateCamera();
}

quat ScreenToArcball(const float2& p)
{
	float dist = glm::dot(p, p);

	// If we're on/in the sphere return the point on it
	if (dist <= 1.0f) return glm::quat(0.0f, p.x, p.y, glm::sqrt(1.0f - dist));

	// Otherwise we project the point onto the sphere
	const glm::vec2 proj = glm::normalize(p);
	return glm::quat(0.0f, proj.x, proj.y, 0.0f);
}

void ArcballCamera::Rotate(const float2& prevPos, const float2& curPos)
{
	const float2 kNormalizeDeviceCoordinatesMin = float2(-1, -1);
	const float2 kNormalizeDeviceCoordinatesMax = float2(1, 1);

	// Clamp mouse positions to stay in NDC
	float2 clampedCurPos = glm::clamp(curPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);
	float2 clampedPrevPos = glm::clamp(prevPos, kNormalizeDeviceCoordinatesMin, kNormalizeDeviceCoordinatesMax);

	quat mouseCurBall = ScreenToArcball(clampedCurPos);
	quat mousePrevBall = ScreenToArcball(clampedPrevPos);

	m_rotationQuat = mouseCurBall * mousePrevBall * m_rotationQuat;

	updateCamera();
}

void ArcballCamera::Pan(const float2& delta)
{
	float  zoomAmount = glm::abs(m_translationMatrix[3][2]);
	float4 motion = float4(delta.x * zoomAmount, delta.y * zoomAmount, 0.0f, 0.0f);

	// Find the panning amount in the world space
	motion = m_inverseViewMatrix * motion;

	m_centerTranslationMatrix = glm::translate(float3(motion)) * m_centerTranslationMatrix;

	updateCamera();
}

void ArcballCamera::Zoom(float amount)
{
	float3 motion = float3(0.0f, 0.0f, amount);

	m_translationMatrix = glm::translate(motion) * m_translationMatrix;

	updateCamera();
}

#pragma endregion