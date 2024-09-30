#include "stdafx.h"
#include "Player.h"
#include "GameApp.h"

bool Player::Setup(GameApplication* game, const PlayerCreateInfo& createInfo)
{
	m_game = game;

	m_position = createInfo.position;

	setupCamera();
		
	return true;
}

void Player::Shutdown()
{
}

void Player::ProcessInput(const std::set<KeyCode>& pressedKeys)
{
	if (pressedKeys.count(KEY_LEFT) > 0) Turn(-GetRateOfTurn(), 0);
	if (pressedKeys.count(KEY_RIGHT) > 0) Turn(GetRateOfTurn(), 0);
	if (pressedKeys.count(KEY_UP) > 0) Turn(0, -GetRateOfTurn());
	if (pressedKeys.count(KEY_DOWN) > 0) Turn(0, GetRateOfTurn());

	if (pressedKeys.count(KEY_W) > 0) move(MovementDirection::Forward, GetRateOfMove());
	if (pressedKeys.count(KEY_A) > 0) move(MovementDirection::Left, GetRateOfMove());
	if (pressedKeys.count(KEY_S) > 0) move(MovementDirection::Backward, GetRateOfMove());
	if (pressedKeys.count(KEY_D) > 0) move(MovementDirection::Right, GetRateOfMove());

	updateCamera();
}

void Player::Update(float deltaTime)
{
}

void Player::move(MovementDirection dir, float distance)
{
	if (dir == MovementDirection::Forward)
	{
		m_position += float3(distance * std::cos(m_azimuth), 0, distance * std::sin(m_azimuth));
	}
	else if (dir == MovementDirection::Left)
	{
		float perpendicularDir = m_azimuth - pi<float>() / 2.0f;
		m_position += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
	}
	else if (dir == MovementDirection::Right)
	{
		float perpendicularDir = m_azimuth + pi<float>() / 2.0f;
		m_position += float3(distance * std::cos(perpendicularDir), 0, distance * std::sin(perpendicularDir));
	}
	else if (dir == MovementDirection::Backward)
	{
		m_position += float3(-distance * std::cos(m_azimuth), 0, -distance * std::sin(m_azimuth));
	}
}

void Player::Turn(float deltaAzimuth, float deltaAltitude)
{
	m_azimuth += deltaAzimuth;
	m_altitude += deltaAltitude;

	// Saturate azimuth values by making wrap around.
	if (m_azimuth < 0.0f) m_azimuth = 2.0f * pi<float>();
	else if (m_azimuth > 2.0f * pi<float>()) m_azimuth = 0.0f;

	// Altitude is saturated by making it stop, so the world doesn't turn upside down.
	if (m_altitude < 0.0f) m_altitude = 0.0f;
	else if (m_altitude > pi<float>()) m_altitude = pi<float>();

	updateCamera();
}

void Player::setupCamera()
{
	m_perspCamera = PerspCamera(60.0f, m_game->GetWindowAspect());
	updateCamera();
}

void Player::updateCamera()
{
	float3 cameraPosition = GetPosition();
	m_perspCamera.LookAt(cameraPosition, GetLookAt(), CAMERA_DEFAULT_WORLD_UP);
	m_perspCamera.SetPerspective(60.f, m_game->GetWindowAspect());
}