#pragma once

#pragma region Camera

constexpr auto CAMERA_DEFAULT_NEAR_CLIP      = 0.1f;
constexpr auto CAMERA_DEFAULT_FAR_CLIP       = 10000.0f;
constexpr auto CAMERA_DEFAULT_EYE_POSITION   = float3(0.0f, 0.0f,  1.0f);
constexpr auto CAMERA_DEFAULT_LOOK_AT        = float3(0.0f, 0.0f,  0.0f);
constexpr auto CAMERA_DEFAULT_WORLD_UP       = float3(0.0f, 1.0f,  0.0f);
constexpr auto CAMERA_DEFAULT_VIEW_DIRECTION = float3(0.0f, 0.0f, -1.0f);

enum class CameraType : uint8_t
{
	Unknown,
	Perspective,
	Orthographic
};

class Camera
{
public:
	Camera(bool pixelAligned = false);
	Camera(float nearClip, float farClip, bool pixelAligned = false);
	virtual ~Camera() = default;

	virtual CameraType GetCameraType() const = 0;

	float GetNearClip() const { return m_nearClip; }
	float GetFarClip() const { return m_farClip; }

	virtual void LookAt(const float3& eye, const float3& target, const float3& up = CAMERA_DEFAULT_WORLD_UP);

	const float3& GetEyePosition() const { return m_eyePosition; }
	const float3& GetTarget() const { return m_target; }
	const float3& GetViewDirection() const { return m_viewDirection; }
	const float3& GetWorldUp() const { return m_worldUp; }

	const float4x4& GetViewMatrix() const { return m_viewMatrix; }
	const float4x4& GetProjectionMatrix() const { return m_projectionMatrix; }
	const float4x4& GetViewProjectionMatrix() const { return m_viewProjectionMatrix; }

	float3 WorldToViewPoint(const float3& worldPoint) const;
	float3 WorldToViewVector(const float3& worldVector) const;

	void MoveAlongViewDirection(float distance);

protected:
	bool             m_pixelAligned = false;
	float            m_nearClip = CAMERA_DEFAULT_NEAR_CLIP;
	float            m_farClip = CAMERA_DEFAULT_FAR_CLIP;
	float3           m_eyePosition = CAMERA_DEFAULT_EYE_POSITION;
	float3           m_target = CAMERA_DEFAULT_LOOK_AT;
	float3           m_viewDirection = CAMERA_DEFAULT_VIEW_DIRECTION;
	float3           m_worldUp = CAMERA_DEFAULT_WORLD_UP;
	mutable float4x4 m_viewMatrix = float4x4(1.0f);
	mutable float4x4 m_projectionMatrix = float4x4(1.0f);
	mutable float4x4 m_viewProjectionMatrix = float4x4(1.0f);
	mutable float4x4 m_inverseViewMatrix = float4x4(1.0f);
};

class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera() = default;
	explicit PerspectiveCamera(float horizFovDegrees, float aspect, float nearClip = CAMERA_DEFAULT_NEAR_CLIP, float farClip = CAMERA_DEFAULT_FAR_CLIP);
	explicit PerspectiveCamera(const float3& eye, const float3& target, const float3& up, float horizFovDegrees, float aspect, float nearClip = CAMERA_DEFAULT_NEAR_CLIP, float farClip = CAMERA_DEFAULT_FAR_CLIP);
	explicit PerspectiveCamera(uint32_t pixelWidth, uint32_t pixelHeight, float horizFovDegrees = 60.0f);
	// Pixel aligned camera
	explicit PerspectiveCamera(uint32_t pixelWidth, uint32_t pixelHeight, float horizFovDegrees, float nearClip, float farClip);

	CameraType GetCameraType() const override { return CameraType::Perspective; }

	void SetPerspective(float horizFovDegrees, float aspect, float nearClip = CAMERA_DEFAULT_NEAR_CLIP, float farClip = CAMERA_DEFAULT_FAR_CLIP);

	void FitToBoundingBox(const float3& bboxMinWorldSpace, const float3& bbxoMaxWorldSpace);

private:
	float m_horizFovDegrees = 60.0f;
	float m_vertFovDegrees = 36.98f;
	float m_aspect = 1.0f;
};

class OrthoCamera : public Camera
{
public:
	OrthoCamera() = default;
	OrthoCamera(float left, float right, float bottom, float top, float nearClip, float farClip);

	CameraType GetCameraType() const override { return CameraType::Orthographic; }

	void SetOrthographic(float left, float right, float bottom, float top, float nearClip, float farClip);

private:
	float m_left = -1.0f;
	float m_right = 1.0f;
	float m_bottom = -1.0f;
	float m_top = 1.0f;
};

// Adapted from: https://github.com/Twinklebear/arcball-cpp
class ArcballCamera : public PerspectiveCamera
{
public:
	ArcballCamera(float horizFovDegrees, float aspect, float nearClip = CAMERA_DEFAULT_NEAR_CLIP, float farClip = CAMERA_DEFAULT_FAR_CLIP);
	ArcballCamera(const float3& eye, const float3& target, const float3& up, float horizFovDegrees, float aspect, float nearClip = CAMERA_DEFAULT_NEAR_CLIP, float farClip = CAMERA_DEFAULT_FAR_CLIP);

	void LookAt(const float3& eye, const float3& target, const float3& up = CAMERA_DEFAULT_WORLD_UP) override;

	// prevPos previous mouse position in normalized device coordinates
	// curPos current mouse position in normalized device coordinates
	void Rotate(const float2& prevPos, const float2& curPos);

	// delta mouse delta in normalized device coordinates
	void Pan(const float2& delta);

	void Zoom(float amount);

private:
	void updateCamera();

	float4x4 m_centerTranslationMatrix;
	float4x4 m_translationMatrix;
	quat     m_rotationQuat;
};

#pragma endregion