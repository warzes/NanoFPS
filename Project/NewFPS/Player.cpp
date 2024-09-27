#include "stdafx.h"
#include "Player.h"
#include "GameApp.h"

constexpr float MAX_LOOK_PITCH = M_PI_2 - 0.2; // min and max pitch angle for the camera in radians

bool Player::Setup(GameApplication* game, const PlayerCreateInfo& createInfo)
{
	m_game = game;

	m_pos = createInfo.position;

	// movement and jumping defaults---initially, the player is on the ground
	m_jumpTimer = 0.0;
	m_gravity = 0.0;
	m_touchingGround = false;
	m_isMoving = false;

	// default viewing parameters
	m_targetLookAngleX = 0.0;
	m_targetLookAngleY = 0.0;
	m_lookAngleX = 0.0;
	m_lookAngleY = 0.0;

	computeWalkingVectors();
	auto mpos = game->GetInput().GetPosition();
	m_oldMouseX = mpos.x;
	m_oldMouseY = mpos.y;

	m_alive = true;

	return true;
}

void Player::Shutdown()
{
}

void Player::Update(float deltaTime)
{
	// player can only do things if they're alive
	if (IsAlive())
	{
		controlMouseInput(deltaTime);
		computeWalkingVectors();
		controlMovingAndFiring(deltaTime);
	}
	controlLooking(deltaTime);
}

bool Player::IsAlive() const
{
	return m_alive;
}

void Player::ComputeCameraOrientation()
{
	glm::mat4 camera = glm::mat4(1.0);
	camera = glm::rotate(camera, m_lookAngleY, glm::vec3(0.0, 1.0, 0.0));
	camera = glm::rotate(camera, m_lookAngleX, glm::vec3(1.0, 0.0, 0.0));

	m_cameraForward = -glm::vec3(camera[2]);
	m_cameraSide = glm::vec3(camera[0]);
	m_cameraUp = glm::vec3(camera[1]);
	m_cameraLook = m_pos + m_cameraForward;
}

glm::vec3 Player::GetPos()
{
	return m_pos;
}

void Player::SetPos(const glm::vec3& pos)
{
	m_pos = pos;
}

glm::vec3 Player::GetCameraLook()
{
	return m_cameraLook;
}

glm::vec3 Player::GetCameraSide()
{
	return m_cameraSide;
}

glm::vec3 Player::GetCameraUp()
{
	return m_cameraUp;
}

void Player::controlMouseInput(float deltaTime)
{
	// always update our mouse position
	auto mpos = m_game->GetInput().GetPosition();

	// compute a smooth look direction based on the mouse motion
	m_targetLookAngleX -= (mpos.x - m_oldMouseY) * 0.002;
	m_targetLookAngleY -= (mpos.y - m_oldMouseX) * 0.002;
	if (m_targetLookAngleX > MAX_LOOK_PITCH) m_targetLookAngleX = MAX_LOOK_PITCH;
	if (m_targetLookAngleX < -MAX_LOOK_PITCH) m_targetLookAngleX = -MAX_LOOK_PITCH;

	// track our old mouse position for mouse movement calculations next frame
	m_oldMouseX = mpos.x;
	m_oldMouseY = mpos.y;
}

void Player::computeWalkingVectors()
{
	m_up = glm::vec3(0.0, 1.0, 0.0);
	m_forward = glm::rotate(glm::vec3(0.0, 0.0, -1.0), m_lookAngleY, m_up);
	m_side = glm::normalize(glm::cross(m_forward, m_up));
}

void Player::controlLooking(float deltaTime)
{
	const float DEATH_IMPACT_STENGTH_FACTOR = 8.0;

	// compute our pitch angle, taking into account any impact animations
	m_lookAngleX = m_lookAngleX + (m_targetLookAngleX - m_lookAngleX) * 0.8;
	if (m_lookAngleX > MAX_LOOK_PITCH) m_lookAngleX = MAX_LOOK_PITCH;
	if (m_lookAngleX < -MAX_LOOK_PITCH) m_lookAngleX = -MAX_LOOK_PITCH;

	// y-look direction is simpler
	m_lookAngleY = m_lookAngleY + (m_targetLookAngleY - m_lookAngleY) * 0.8;
}

void Player::controlMovingAndFiring(float deltaTime)
{
	const float JUMP_ACCEL_TIME = 0.10;				// player jump acceleration control
	const float JUMP_STRENGTH = 60.0;				// maximum jump acceleration experienced by player
	const float GRAVITY_STRENGTH = 9.81;			// lighter than regular gravity, for a smoother jump
	const float GUN_INACCURACY = 0.0025;			// how much to vary the direction of the bullet when firing

	const float MOVE_SPEED = 6.7;					// fast sprint is 15mph, or 6.7 m/s

	glm::vec3 targetVelocity = glm::vec3(0.0);						// how fast we want to go
	glm::vec3 bulletDir;							// the direction we fire bullets in

	// inform everyone else that we're not doing anything yet
	m_isMoving = false;

	if (m_game->GetInput().IsPressed(KEY_W))
	{
		targetVelocity += glm::vec3(0.0, 0.0, MOVE_SPEED);
		m_isMoving = true;
	}
	if (m_game->GetInput().IsPressed(KEY_A))
	{
		targetVelocity += glm::vec3(-MOVE_SPEED, 0.0, 0.0);
		m_isMoving = true;
	}
	if (m_game->GetInput().IsPressed(KEY_D))
	{
		targetVelocity += glm::vec3(MOVE_SPEED, 0.0, 0.0);
		m_isMoving = true;
	}
	if (m_game->GetInput().IsPressed(KEY_S))
	{
		targetVelocity += glm::vec3(0.0, 0.0, -MOVE_SPEED);
		m_isMoving = true;
	}
	// space bar will start the player's jump acceleration
	if (m_game->GetInput().IsPressed(KEY_SPACE) && m_touchingGround)
	{
		m_jumpTimer = JUMP_ACCEL_TIME;
	}
	
	// if we're in the middle of a jump, then keep accelerating upwards
	if (m_jumpTimer > 0.0)
	{
		m_gravity += (JUMP_STRENGTH * (m_jumpTimer / JUMP_ACCEL_TIME)) * deltaTime;
	}
	m_jumpTimer -= deltaTime;

	// add gravity
	m_gravity -= GRAVITY_STRENGTH * deltaTime;
	m_pos.y += m_gravity * deltaTime;

	// move forward, too
	m_pos = m_pos + (m_forward * targetVelocity.z * deltaTime) + (m_side * targetVelocity.x * deltaTime);

	// detect collision with ground
	if (m_pos.y < 2.0f/*world->getTerrainHeight(m_pos) + PLAYER_HEIGHT*/)
	{
		// restore normal ground-level height and set gravity speed to zero
		m_pos.y = 2.0f;// world->getTerrainHeight(m_pos) + PLAYER_HEIGHT;
		m_gravity = 0.0;

		// we can jump
		m_touchingGround = true;
	}
	else
	{
		m_touchingGround = false;
	}
}
