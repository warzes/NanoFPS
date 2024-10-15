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
	float typicalSpeed = 10.0f; // Typical speed of an object in the scene.

	bool enable = false;
};

class PhysicsSystem final
{
	friend EngineApplication;
	friend class PhysicsScene;

	// Type of contacts reported by PhysX simulation.
	enum class ContactEventType
	{
		ContactBegin,
		ContactStay,
		ContactEnd
	};



public:
	PhysicsSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const PhysicsCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();

	[[nodiscard]] MaterialPtr CreateMaterial(const MaterialCreateInfo& createInfo);

	// Pauses or resumes the physics simulation.
	void SetPaused(bool paused);

	// Enables or disables collision between two layers. Each physics object can be assigned a specific layer, and here you can determine which layers can interact with each other.
	void ToggleCollision(uint64_t groupA, uint64_t groupB, bool enabled);

	// Checks if two collision layers are allowed to interact.
	bool IsCollisionEnabled(uint64_t groupA, uint64_t groupB) const;

	[[nodiscard]] bool IsEnable() const { return m_enable; }

	[[nodiscard]] PhysicsScene& GetScene() { return m_scene; }
	[[nodiscard]] auto GetPxPhysics() { return m_physics; }

private:
	static const uint64_t CollisionMapSize = 64;

	EngineApplication&       m_engine;
	physx::PxTolerancesScale m_scale{};
	physx::PxFoundation*     m_foundation{ nullptr };
	physx::PxPhysics*        m_physics{ nullptr };
	ph::PhysicsScene         m_scene; // TODO: в будущем создавать из physics system
	mutable std::mutex       m_mutex;
	bool                     m_collisionMap[CollisionMapSize][CollisionMapSize]{};
	bool                     m_enable{ false };
};

#pragma endregion

} // namespace ph