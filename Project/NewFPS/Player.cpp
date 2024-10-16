#include "stdafx.h"
#include "Player.h"
#include "GameApp.h"

bool Player::Setup(GameApplication* game, const PlayerCreateInfo& createInfo)
{
	m_game = game;

	m_transform.SetRotationOrder(Transform::RotationOrder::YXZ);
	m_transform.RotateY(createInfo.yaw);


	m_movement.Setup(game, &m_transform, createInfo.position);
		
	return true;
}

void Player::Shutdown()
{
	m_movement.Shutdown();
}

void Player::Update(float deltaTime)
{
	// turn
	{
		const float mouseSpeed = /*slowMotion ? m_mouseSpeed * 0.4f :*/ m_mouseSpeed;
		const glm::vec2& deltaMousePos = m_game->GetInput().GetDeltaPosition();
		m_transform
			.RotateX(mouseSpeed * deltaMousePos.y)
			.RotateY(-mouseSpeed * deltaMousePos.x)
			.ClampPitch();
	}

	// move
	{
		glm::vec3 movementInput = 
			m_transform.GetHorizontalRightVector()
			* m_game->GetInput().GetKeyAxis(GLFW_KEY_A, GLFW_KEY_D) 
			+ m_transform.GetHorizontalForwardVector()
			* m_game->GetInput().GetKeyAxis(GLFW_KEY_W, GLFW_KEY_S);
		if (movementInput.x != 0 || movementInput.y != 0 || movementInput.z != 0)
		{
			movementInput = glm::normalize(movementInput);
		}
		m_movement.SetMovementInput(movementInput);
	}

	// jump
	{
		bool currSpace = m_game->GetInput().IsPressed(GLFW_KEY_SPACE);
		if (/*!m_prevSpace &&*/ currSpace)
		{
			m_movement.Jump();
		}
		m_prevSpace = currSpace;
	}

	m_movement.Update(deltaTime);
}

void Player::FixedUpdate(float fixedDeltaTime)
{
	m_movement.FixedUpdate(fixedDeltaTime);
}