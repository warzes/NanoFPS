#include "stdafx.h"
#include "Core.h"
#include "PhysicsScene.h"
#include "PhysicsSystem.h"
#include "Application.h"

namespace ph {

using namespace physx;

//=============================================================================
#pragma region [ Simulation Event Callback ]

class PhysXEventCallback final : public PxSimulationEventCallback
{
public:
	void onWake(PxActor** /*actors*/, PxU32 /*count*/) final {}
	void onSleep(PxActor** /*actors*/, PxU32 /*count*/) final {}

	void onTrigger(PxTriggerPair* pairs, PxU32 count) final {}
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) final {}
	void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) final {}

	void onAdvance(const PxRigidBody* const* /*bodyBuffer*/, const PxTransform* /*poseBuffer*/, const PxU32 /*count*/) final {}
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Scene ]

extern PxDefaultCpuDispatcher* gCpuDispatcher;
PhysXEventCallback             gPhysXEventCallback;

PhysicsScene::PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine)
	: m_engine(engine)
	, m_system(physicsEngine)
{
}

bool PhysicsScene::Setup(const PhysicsSceneCreateInfo& createInfo)
{
	m_physics = m_system.m_physics;

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity                 = { createInfo.gravity.x, createInfo.gravity.y, createInfo.gravity.z };
	sceneDesc.cpuDispatcher           = gCpuDispatcher;
	sceneDesc.filterShader            = PxDefaultSimulationFilterShader;
	sceneDesc.simulationEventCallback = &gPhysXEventCallback;

	m_scene = m_physics->createScene(sceneDesc);
	if (!m_scene)
	{
		Fatal("Failed to create PhysX scene.");
		return false;
	}

	m_controllerManager = PxCreateControllerManager(*m_scene);
	if (!m_controllerManager)
	{
		Fatal("Failed to create PhysX controller manager.");
		return false;
	}

	return true;
}

void PhysicsScene::Shutdown()
{
	PX_RELEASE(m_controllerManager);
	PX_RELEASE(m_scene);
}

void PhysicsScene::FixedUpdate()
{
	m_scene->simulate(m_engine.GetFixedTimestep());
	
	uint32_t errorState;
	if (!m_scene->fetchResults(true, &errorState))
		Warning("Physics simulation failed. Error code: " + std::to_string(errorState));
}

physx::PxRaycastBuffer PhysicsScene::Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, const float distance, PhysicsLayer layer) const
{
	physx::PxQueryFilterData queryFilterData;
	queryFilterData.data = PhysicsFilterDataFromLayer(layer);

	physx::PxRaycastBuffer buffer;
	m_scene->raycast(origin, unitDir, distance, buffer, physx::PxHitFlag::eDEFAULT, queryFilterData);
	return buffer;
}

physx::PxSweepBuffer PhysicsScene::Sweep(const physx::PxGeometry& geometry, const physx::PxTransform& pose, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const
{
	physx::PxQueryFilterData queryFilterData;
	queryFilterData.data = PhysicsFilterDataFromLayer(layer);

	physx::PxSweepBuffer buffer;
	m_scene->sweep(geometry, pose, unitDir, distance, buffer, physx::PxHitFlag::eDEFAULT, queryFilterData);
	return buffer;
}

#pragma endregion


} // namespace ph