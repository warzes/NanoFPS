#pragma once

#include "Physics.h"

namespace ph {

//=============================================================================
#pragma region [ Physics Scene ]

class PhysicsSystem;

struct PhysicsSceneCreateInfo final
{
	uint8_t   cpuDispatcherNum = 2;

	glm::vec3 gravity{ 0.0f, -9.81f, 0.0f };
	MaterialCreateInfo defaultMaterial{ 0.8f, 0.8f, 0.25f };
};

class PhysicsScene final
{
	friend EngineApplication;
public:
	PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine);

	[[nodiscard]] bool Setup(const PhysicsSceneCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();

	void SetSimulationEventCallback(physx::PxSimulationEventCallback* callback);

	[[nodiscard]] physx::PxRaycastBuffer Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] physx::PxSweepBuffer Sweep(const physx::PxGeometry& geometry, const physx::PxTransform& pose, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] auto GetDefaultMaterial() { return m_defaultMaterial; }
	[[nodiscard]] auto GetControllerManager() { return m_controllerManager; }

	[[nodiscard]] auto GetPxScene() { return m_scene; }

private:
	EngineApplication& m_engine;
	PhysicsSystem& m_system;
	PhysicsSimulationEventCallback m_physicsCallback;
	physx::PxPhysics* m_physics{ nullptr };
	physx::PxDefaultCpuDispatcher* m_cpuDispatcher{ nullptr };
	physx::PxScene* m_scene{ nullptr };
	MaterialPtr                    m_defaultMaterial{ nullptr };
	physx::PxControllerManager* m_controllerManager{ nullptr };
};

#pragma endregion

} // namespace ph