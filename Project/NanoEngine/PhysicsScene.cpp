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
	void onConstraintBreak(physx::PxConstraintInfo* /*constraints*/, physx::PxU32 /*count*/) final {}
	void onWake(physx::PxActor** /*actors*/, physx::PxU32 /*count*/) final {}
	void onSleep(physx::PxActor** /*actors*/, physx::PxU32 /*count*/) final {}
	void onContact(const physx::PxContactPairHeader& /*pairHeader*/, const physx::PxContactPair* /*pairs*/, physx::PxU32 /*nbPairs*/) final {}
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) final {}
	void onAdvance(const physx::PxRigidBody* const* /*bodyBuffer*/, const physx::PxTransform* /*poseBuffer*/, const physx::PxU32 /*count*/) final {}
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Scene ]

extern PxDefaultCpuDispatcher* gCpuDispatcher;

PhysicsScene::PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine)
	: m_engine(engine)
	, m_system(physicsEngine)
{
}

bool PhysicsScene::Setup(const PhysicsSceneCreateInfo& createInfo)
{
	m_physics = m_system.m_physics;

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = { createInfo.gravity.x, createInfo.gravity.y, createInfo.gravity.z };
	sceneDesc.cpuDispatcher = gCpuDispatcher;
	sceneDesc.filterShader = PxDefaultSimulationFilterShader;
	sceneDesc.simulationEventCallback = &m_physicsCallback;
	m_scene = m_physics->createScene(sceneDesc);
	if (!m_scene)
	{
		Fatal("Failed to create PhysX scene.");
		return false;
	}

	if (PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient())
	{
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	m_defaultMaterial = m_system.CreateMaterial(createInfo.defaultMaterial);
	if (!m_defaultMaterial->IsValid())
	{
		Fatal("Failed to create default PhysX material.");
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
	m_defaultMaterial.reset();
	PX_RELEASE(m_scene);
}

void PhysicsScene::FixedUpdate()
{
	m_scene->simulate(m_engine.GetFixedTimestep());
	m_scene->fetchResults(true);
}

void PhysicsScene::SetSimulationEventCallback(physx::PxSimulationEventCallback* callback)
{
	m_scene->setSimulationEventCallback(callback);
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