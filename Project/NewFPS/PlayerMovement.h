#pragma once

class GameApplication;

class PlayerMovement final
{
public:
	bool Setup(GameApplication* game, Transform* playerTransform, const glm::vec3& position);
	void Shutdown();

	void Update(float deltaTime);
	void FixedUpdate(float fixedDeltaTime);

	[[nodiscard]] bool CanJump() const { return m_isOnGround; }
	[[nodiscard]] const glm::vec3& GetVelocity() const { return m_velocity; }
	[[nodiscard]] const glm::vec3& GetPosition() const { return m_position; }

	void SetMovementInput(const glm::vec3& movementInput) { m_movementInput = movementInput; }

	void Launch(float yVelocity) { m_velocity.y = yVelocity; }

	void Jump()
	{
		if (CanJump()) Launch(JUMP_VELOCITY);
	}

private:
	void checkGround();
	void updateAcceleration();

	GameApplication* m_app;
	ph::PhysicsScene* m_scene;

	static constexpr float CAPSULE_RADIUS = 0.4f;
	static constexpr float CAPSULE_HALF_HEIGHT = 0.4f;
	static constexpr float CAPSULE_HEIGHT = CAPSULE_HALF_HEIGHT * 2.0f;

	static constexpr float GROUND_CHECK_DISTANCE = 0.3f;

	static constexpr float GRAVITY = 9.8f;
	static constexpr float GROUND_ACCELERATION = 150.0f;
	static constexpr float GROUND_DRAG = 10.0f;
	static constexpr float AIR_ACCELERATION = 10.0f;
	static constexpr float AIR_DRAG = 2.0f;
	static constexpr float JUMP_VELOCITY = 7.0f;

	ph::CharacterControllerPtr m_controller{ nullptr };

	glm::vec3 m_movementInput{};

	bool m_isOnGround = false;

	glm::vec3 m_velocity{};
	glm::vec3 m_acceleration{};

	glm::vec3 m_position{};
	Transform* m_transform;

	glm::vec3 m_predictedPosition{};
	glm::vec3 m_predictedVariance{ 10000.0f, 10000.0f, 10000.0f };
};