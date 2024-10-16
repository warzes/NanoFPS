#pragma once

#include "PhysicsResources.h"

namespace ph {

//=============================================================================
#pragma region [ Physics Scene ]

struct PhysicsSceneCreateInfo final
{
	glm::vec3 gravity{ 0.0f, -9.81f, 0.0f };
};

class PhysicsScene final
{
	friend EngineApplication;
	friend PhysicsSystem;
public:
	PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine);

	[[nodiscard]] bool Setup(const PhysicsSceneCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();
	
	[[nodiscard]] physx::PxRaycastBuffer Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;
	[[nodiscard]] physx::PxSweepBuffer Sweep(const physx::PxGeometry& geometry, const physx::PxTransform& pose, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] auto GetPxControllerManager() { return m_controllerManager; }
	[[nodiscard]] auto GetPxScene() { return m_scene; }

private:
	EngineApplication&          m_engine;
	PhysicsSystem&              m_system;
	physx::PxPhysics*           m_physics{ nullptr };
	physx::PxScene*             m_scene{ nullptr };
	physx::PxControllerManager* m_controllerManager{ nullptr };
};

#pragma endregion

} // namespace ph