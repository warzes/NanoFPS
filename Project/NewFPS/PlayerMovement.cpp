#include "stdafx.h"
#include "PlayerMovement.h"
#include "GameApp.h"

// based on Kalman filter from: https://github.com/Garima13a/Kalman-Filters/blob/master/2.%201D%20Kalman%20Filter%2C%20solution.ipynb

void kalmanUpdate(glm::vec3& posPredicted, glm::vec3& varPredicted, const glm::vec3& posObserved, const glm::vec3& varObserved)
{
	const glm::vec3 pos = (varObserved * posPredicted + varPredicted * posObserved) / (varPredicted + varObserved);
	const glm::vec3 var = varPredicted * varObserved / (varPredicted + varObserved);

	posPredicted = pos;
	varPredicted = var;
}

void kalmanPredict(glm::vec3& posPredicted, glm::vec3& varPredicted, const glm::vec3& deltaObserved, const glm::vec3& varObserved)
{
	const glm::vec3 mean = posPredicted + deltaObserved;
	const glm::vec3 var = varPredicted + varObserved;

	posPredicted = mean;
	varPredicted = var;
}

bool PlayerMovement::Setup(GameApplication* game, Transform* playerTransform, const glm::vec3& position)
{
	m_app = game;
	m_scene = &game->GetPhysicsScene();
	m_transform = playerTransform;

	m_transform->SetTranslation(position);

	ph::CharacterControllerCreateInfo ccci{};
	ccci.position = position;
	ccci.radius = CAPSULE_RADIUS;
	ccci.height = CAPSULE_HEIGHT;
	ccci.userData = this;

	m_controller = std::make_shared<ph::CharacterController>(*game, ccci);

	m_position = position;
	m_predictedPosition = position;

	return true;
}

void PlayerMovement::Shutdown()
{
	m_controller.reset();
}

void PlayerMovement::Update(float deltaTime)
{
	const glm::vec3 lastEyePosition = m_transform->GetTranslation();
	const glm::vec3 predictedPosition = glm::mix(
		m_position,
		m_predictedPosition,
		m_app->GetFixedUpdateTimeError() / m_app->GetFixedTimestep()
	);
	const glm::vec3 targetEyePosition = predictedPosition + glm::vec3{ 0.0f, CAPSULE_HALF_HEIGHT, 0.0f };
	m_transform->SetTranslation(glm::mix(lastEyePosition, targetEyePosition, glm::min(1.0f, 30.0f * deltaTime)));
}

void PlayerMovement::FixedUpdate(float fixedDeltaTime)
{
	checkGround();
	updateAcceleration();

	// move character controller
	m_velocity += m_acceleration * fixedDeltaTime;

	const glm::vec3 displacement = m_velocity * fixedDeltaTime;
	m_controller->Move(displacement, 0.0001f, fixedDeltaTime);

	// calc projected velocity
	const glm::vec3 currentPosition = m_controller->GetPosition();
	const glm::vec3 deltaPosition = currentPosition - m_position;

	m_velocity = deltaPosition / fixedDeltaTime;
	m_position = currentPosition;

	// clamp vertical speed (this is a hack)
	m_velocity.y = glm::min(m_velocity.y, JUMP_VELOCITY);

	kalmanUpdate(m_predictedPosition, m_predictedVariance, m_position, { 8.0f, 8.0f, 8.0f });
	kalmanPredict(m_predictedPosition, m_predictedVariance, deltaPosition, { 4.0f, 4.0f, 4.0f });
}

void PlayerMovement::checkGround()
{
	// https://nvidia-omniverse.github.io/PhysX/physx/5.1.1/docs/Geometry.html#capsules
	static const physx::PxCapsuleGeometry QUERY_GEOMETRY(CAPSULE_RADIUS, CAPSULE_HALF_HEIGHT);
	static const physx::PxQuat            QUERY_ROTATION(physx::PxHalfPi, physx::PxVec3(0.0f, 0.0f, 1.0f));
	static const physx::PxVec3            QUERY_DIRECTION(0.0f, -1.0f, 0.0f);

	const glm::vec3 pos = m_controller->GetPosition();

	const physx::PxSweepBuffer buffer = m_scene->Sweep(
		QUERY_GEOMETRY,
		physx::PxTransform({ pos.x, pos.y, pos.z }, QUERY_ROTATION),
		QUERY_DIRECTION,
		GROUND_CHECK_DISTANCE,
		ph::PHYSICS_LAYER_0
	);

	// check is touching ground and ground is not too steep
	m_isOnGround = buffer.hasBlock && buffer.block.normal.y >= m_controller->GetSlopeLimit();
}

void PlayerMovement::updateAcceleration()
{
	float acceleration = m_isOnGround ? GROUND_ACCELERATION : AIR_ACCELERATION;
	float drag = m_isOnGround ? GROUND_DRAG : AIR_DRAG;
	m_acceleration.x = m_movementInput.x * acceleration - m_velocity.x * drag;
	m_acceleration.y = -GRAVITY;
	m_acceleration.z = m_movementInput.z * acceleration - m_velocity.z * drag;
}