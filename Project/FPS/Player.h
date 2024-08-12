#pragma once

#include "NanoEngineVK.h"

#pragma region Player Movement

class Actor;

class PlayerMovement final
{
public:
	PlayerMovement(Actor* owner, const glm::vec3& position);
	PlayerMovement(const PlayerMovement&) = delete;
	PlayerMovement(PlayerMovement&&) = delete;
	~PlayerMovement();

	PlayerMovement& operator=(const PlayerMovement&) = delete;
	PlayerMovement& operator=(PlayerMovement&&) = delete;

	void Update(float deltaTime);
	void FixedUpdate(float fixedDeltaTime);

	[[nodiscard]] bool CanJump() const { return m_isOnGround; }
	[[nodiscard]] const glm::vec3& GetVelocity() const { return m_velocity; }

	void SetMovementInput(const glm::vec3& movementInput) { m_movementInput = movementInput; }

	void Launch(float yVelocity) { m_velocity.y = yVelocity; }

	void Jump()
	{
		if (CanJump()) Launch(JUMP_VELOCITY);
	}

private:
	void checkGround();

	void updateAcceleration();

	static constexpr float CAPSULE_RADIUS = 0.4f;
	static constexpr float CAPSULE_HALF_HEIGHT = 0.4f;
	static constexpr float CAPSULE_HEIGHT = CAPSULE_HALF_HEIGHT * 2.0f;

	static constexpr float GROUND_CHECK_DISTANCE = 0.3f;

	static constexpr float GRAVITY = 20.0f;
	static constexpr float GROUND_ACCELERATION = 50.0f;
	static constexpr float GROUND_DRAG = 10.0f;
	static constexpr float AIR_ACCELERATION = 10.0f;
	static constexpr float AIR_DRAG = 2.0f;
	static constexpr float JUMP_VELOCITY = 7.0f;

	Transform& m_transform;

	physx::PxController* m_controller = nullptr;

	glm::vec3 m_movementInput{};

	bool m_isOnGround = false;

	glm::vec3 m_velocity{};
	glm::vec3 m_acceleration{};

	glm::vec3 m_position{};

	glm::vec3 m_predictedPosition{};
	glm::vec3 m_predictedVariance{ 10000.0f, 10000.0f, 10000.0f };
};

#pragma endregion

#pragma region Player Use

class PlayerUse
{
public:
	explicit PlayerUse(Actor* owner);
	PlayerUse(const PlayerUse&) = delete;
	PlayerUse(PlayerUse&&) = delete;

	PlayerUse& operator=(const PlayerUse&) = delete;
	PlayerUse& operator=(PlayerUse&&) = delete;

	void Update();

	[[nodiscard]] Actor* GetTarget() const { return m_prevEyeTarget; }

	void SetInput(bool pressed) { m_currButton = pressed; }

private:
	physx::PxRaycastHit eyeRayCast();

	Actor* m_owner = nullptr;

	static constexpr float INTERACTION_DISTANCE = 2.0f;

	bool m_prevButton = false;
	bool m_currButton = false;

	Actor* m_prevEyeTarget = nullptr;
};

#pragma endregion
