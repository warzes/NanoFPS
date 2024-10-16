#pragma once

#include "PhysicsScene.h"

class EngineApplication;

namespace ph {

//=============================================================================
#pragma region [ Physics System ]

struct PhysicsCreateInfo final
{
	PhysicsSceneCreateInfo scene;

	uint8_t cpuDispatcherNum = 2;

	float typicalLength = 1.0f; // Typical length of an object in the scene.
	float typicalSpeed = 9.81f; // Typical speed of an object in the scene.
	MaterialCreateInfo defaultMaterial{ 0.8f, 0.8f, 0.25f };

	bool enable = false;
};

class PhysicsSystem final
{
	friend EngineApplication;
	friend class PhysicsScene;
public:
	PhysicsSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const PhysicsCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();

	[[nodiscard]] MaterialPtr CreateMaterial(const MaterialCreateInfo& createInfo);

	// Pauses or resumes the physics simulation.
	void SetPaused(bool paused);
	[[nodiscard]] bool IsEnable() const { return m_enable; }

	[[nodiscard]] PhysicsScene& GetScene() { return m_scene; }
	[[nodiscard]] auto GetDefaultMaterial() { return m_defaultMaterial; }
	[[nodiscard]] auto GetPxPhysics() { return m_physics; }

private:
	EngineApplication&       m_engine;
	physx::PxTolerancesScale m_scale{};
	physx::PxFoundation*     m_foundation{ nullptr };
	physx::PxPhysics*        m_physics{ nullptr };
	PhysicsScene             m_scene; // TODO: в будущем создавать из physics system
	MaterialPtr              m_defaultMaterial{ nullptr };
	bool                     m_enable{ false };
};

#pragma endregion

} // namespace ph