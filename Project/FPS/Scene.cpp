#include "Base.h"
#include "Core.h"
#include "EngineWindow.h"
#include "Renderer.h"
#include "Scene.h"

extern std::unique_ptr<PbrRenderer> renderer;

#pragma region APlayerNoClip

APlayerNoClip::APlayerNoClip(const glm::vec3& position)
{
	GetTransform().SetPosition(position);
	Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
}

APlayerNoClip::~APlayerNoClip()
{
	Mouse::SetCursorMode(Mouse::CursorMode::Normal);
}

void APlayerNoClip::Update(float deltaTime)
{
	const glm::vec3 inputDirection = 
		GetTransform().GetHorizontalRightVector()   * Keyboard::GetKeyAxis(GLFW_KEY_D, GLFW_KEY_A) +
		glm::vec3(0.0f, 1.0f, 0.0f)                 * Keyboard::GetKeyAxis(GLFW_KEY_E, GLFW_KEY_Q) +
		GetTransform().GetHorizontalForwardVector() * Keyboard::GetKeyAxis(GLFW_KEY_W, GLFW_KEY_S);

	if (inputDirection.x != 0.0f || inputDirection.y != 0.0f || inputDirection.z != 0.0f)
	{
		GetTransform().Translate(glm::normalize(inputDirection) * (5.0f * deltaTime));
	}

	const glm::vec2& deltaMousePos = Mouse::GetDeltaPosition();
	GetTransform()
		.RotateX(0.001f * deltaMousePos.y)
		.RotateY(0.001f * deltaMousePos.x)
		.ClampPitch();
}

void APlayerNoClip::Draw()
{
	renderer->SetCameraData(GetTransform().GetPosition(), GetTransform().GetInverseMatrix(), glm::radians(60.0f), 0.01f, 1000.0f);
}

#pragma endregion