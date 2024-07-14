#include "Base.h"
#include "Graphics.h"


#pragma region Camera

void Camera::Set(glm::vec3 Position, glm::vec3 Up, float Yaw, float Pitch)
{
	position = Position;
	front = CAMERA_FRONT;
	up = CAMERA_UP;
	right = CAMERA_RIGHT;
	worldUp = Up;
	yaw = Yaw;
	pitch = Pitch;
	update();
}

void Camera::Move(MovementDir direction, float deltaTime)
{
	const float velocity = movementSpeed * deltaTime;
	if (direction == Forward) position += front * velocity;
	else if (direction == Backward) position -= front * velocity;
	else if (direction == Left) position -= right * velocity;
	else if (direction == Right) position += right * velocity;
	update();
}

void Camera::Rotate(float xOffset, float yOffset)
{
	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f) pitch = 89.0f;
	else if (pitch < -89.0f) pitch = -89.0f;
	update();
}

const glm::mat4& Camera::GetViewMatrix() const
{
	return m_view;
}

void Camera::update()
{
	front = glm::normalize(
		glm::vec3
		(
			cos(glm::radians(yaw)) * cos(glm::radians(pitch)), // x
			sin(glm::radians(pitch)), // y
			sin(glm::radians(yaw)) * cos(glm::radians(pitch)) // z
		)
	);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));

	m_view = glm::lookAt(position, position + front, up);
}

#pragma endregion
