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

#pragma region ALightPoint

ALightPoint::ALightPoint(const glm::vec3& position, const glm::vec3& color, float radius)
	: m_position(position)
	, m_color(color)
	, m_radius(radius)
{
}

void ALightPoint::Draw()
{
	renderer->DrawPointLight(m_position, m_color, m_radius);
}

#pragma endregion

#pragma region Scene

void Scene::Update(const float deltaTime)
{
	// put pending destroy actors from last frame into a temp queue
	const std::vector<std::unique_ptr<Actor>> pendingDestroyActorsFromLastFrame = std::move(m_pendingDestroyActors);

	// update all alive actors
	std::vector<std::unique_ptr<Actor>> aliveActors;
	aliveActors.reserve(m_actors.size());
	for (auto& actor : m_actors)
	{
		actor->Update(deltaTime);
		if (actor->IsPendingDestroy())
		{
			// put pending destroy actors in this frame into m_pendingDestroyActors
			m_pendingDestroyActors.push_back(std::move(actor));
		}
		else
		{
			aliveActors.push_back(std::move(actor));
		}
	}
	m_actors = std::move(aliveActors);

	// pendingDestroyActorsFromLastFrame deconstructs and releases all pending destroy actors destroy pending destroy actors from last frame this strategy will give other actors one frame to cleanup their references
}

void Scene::FixedUpdate(float fixedDeltaTime)
{
	for (auto& actor : m_actors)
		actor->FixedUpdate(fixedDeltaTime);
}

void Scene::Draw()
{
	for (auto& actor : m_actors)
		actor->Draw();
}

void Scene::Register(const std::string& name, Actor* actor)
{
	auto pair = m_registeredActors.find(name);
	if (pair == m_registeredActors.end())
	{
		m_registeredActors.emplace(name, actor);
	}
	else
	{
		Error("Failed to register actor \"" + name + "\" because the name is already registered.");
	}
}

Actor* Scene::FindActorWithName(const std::string& name) const
{
	auto pair = m_registeredActors.find(name);
	if (pair == m_registeredActors.end())
	{
		Error("Failed to find actor \"" + name + "\"");
		return nullptr;
	}
	return pair->second;
}

Actor* Scene::findFirstActorOfClassImpl(const std::string& className) const
{
	for (auto& actor : m_actors)
	{
		if (actor->GetActorClassName() == className)
		{
			return actor.get();
		}
	}
	return nullptr;
}

#pragma endregion