#pragma once

#pragma region MyRegion

constexpr auto CAMERA_UP = glm::vec3(0.0f, 1.0f, 0.0f);
#ifdef GLM_FORCE_LEFT_HANDED
constexpr auto CAMERA_FRONT = glm::vec3(0.0f, 0.0f, 1.0f);
#else
constexpr auto CAMERA_FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
#endif
constexpr auto CAMERA_RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr float CAMERA_YAW = -90.0f;
constexpr float CAMERA_PITCH = 0.0f;
constexpr float CAMERA_SPEED = 10.0f;
constexpr float CAMERA_SENSITIVITY = 0.1f;

class Camera final
{
public:
	enum MovementDir
	{
		Forward,
		Backward,
		Left,
		Right
	};

	void Set(
		glm::vec3 position = glm::vec3(0.0f),
		glm::vec3 up = CAMERA_UP,
		float yaw = CAMERA_YAW,
		float pitch = CAMERA_PITCH);

	void Move(MovementDir direction, float deltaTime);
	void Rotate(float xOffset, float yOffset); // TODO: дельтатайм нужна?

	[[nodiscard]] const glm::mat4& GetViewMatrix() const;

	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 front = CAMERA_FRONT;
	glm::vec3 up = CAMERA_UP;
	glm::vec3 right = CAMERA_RIGHT;
	glm::vec3 worldUp = CAMERA_UP;

	float yaw = CAMERA_YAW;
	float pitch = CAMERA_PITCH;

	float movementSpeed = CAMERA_SPEED;
	float mouseSensitivity = CAMERA_SENSITIVITY;

private:
	void update();
	glm::mat4 m_view;
};

#pragma endregion