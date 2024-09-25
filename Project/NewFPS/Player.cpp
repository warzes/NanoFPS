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
}

void Player::controlMovingAndFiring(float deltaTime)
{
}
