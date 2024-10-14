#pragma once

#include "PlayerMovement.h"

class GameApplication;

enum class MovementDirection
{
	Forward,
	Left,
	Right,
	Backward,
	Up,
	Down,
};

struct PlayerCreateInfo final
{
	glm::vec3 position = glm::vec3(20.0f, 20.0f, 20.0f);
	float yaw = 0.0f;
};

class Player final
{
public:
	bool Setup(GameApplication* game, const PlayerCreateInfo& createInfo);
	void Shutdown();

	void Update(float deltaTime);
	void FixedUpdate(float fixedDeltaTime);

	const float3& GetPosition() const { return m_transform.GetTranslation(); }
	Transform& GetTransform() { return m_transform; }

private:
	GameApplication* m_game;
	PlayerMovement m_movement;
	Transform m_transform; // TODO: ������� ��� ��������, � �� ��-�� ���� ������� ������� ������ (��-�� ���� �� ������� ����������� ��������� ������� ������� �� World ���� - ��� ������ �� ���������
	float m_mouseSpeed = 0.001f;
	bool m_prevSpace = false;
};