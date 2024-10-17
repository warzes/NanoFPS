#include "stdafx.h"
#include "Core.h"
#include "PhysicsScene.h"
#include "PhysicsSystem.h"
#include "Application.h"

namespace ph {

using namespace physx;

//=============================================================================
#pragma region [ FilterShader ]

//see https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Manual/RigidBodyCollision.html#broad-phase-callback
PxFilterFlags FilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}
	// generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;

	// trigger the contact callback for pairs (A,B) where
	// the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eNOTIFY_CONTACT_POINTS;

	return PxFilterFlag::eDEFAULT;
}

#pragma endregion

//=============================================================================
#pragma region [ Simulation Event Callback ]

class PhysXEventCallback final : public PxSimulationEventCallback
{
public:
	void onWake(PxActor** /*actors*/, PxU32 /*count*/) final {}
	void onSleep(PxActor** /*actors*/, PxU32 /*count*/) final {}

	void onTrigger(PxTriggerPair* pairs, PxU32 count) final;
	void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) final;
	void onConstraintBreak(PxConstraintInfo* /*constraints*/, PxU32 /*count*/) final {}

	void onAdvance(const PxRigidBody* const* /*bodyBuffer*/, const PxTransform* /*poseBuffer*/, const PxU32 /*count*/) final {}
};

void PhysXEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	for (PxU32 i = 0; i < count; ++i)
	{
		// ignore pairs when shapes have been deleted
		const PxTriggerPair& cp = pairs[i];
		if (cp.flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
		{
			continue;
		}

		//if these actors do not exist in the scene anymore due to deallocation, do not process
		if (cp.otherActor->userData == nullptr || cp.triggerActor->userData == nullptr)
		{
			continue;
		}

		UserData* data1 = (UserData*)cp.otherActor->userData;
		UserData* data2 = (UserData*)cp.triggerActor->userData;

		if (data1->type == UserDataType::CharacterController || data2->type == UserDataType::CharacterController)
		{
			// TODO: пока не реализовано
			continue;
		}

		BaseActor* other = (BaseActor*)data1->ptr;
		BaseActor* trigger = (BaseActor*)data2->ptr;

		//process events
		if (cp.status & (PxPairFlag::eNOTIFY_TOUCH_FOUND))
		{
			other->OnTriggerEnter(trigger);
			trigger->OnTriggerEnter(other);
		}

		if (cp.status & (PxPairFlag::eNOTIFY_TOUCH_LOST))
		{
			other->OnTriggerExit(trigger);
			trigger->OnTriggerExit(other);
		}
	}
}

void PhysXEventCallback::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
	for (PxU32 i = 0; i < nbPairs; ++i) 
	{
		const PxContactPair& contactpair = pairs[i];

		//if these actors do not exist in the scene anymore due to deallocation, do not process
		if (pairHeader.actors[0]->userData == nullptr || pairHeader.actors[1]->userData == nullptr) {
			continue;
		}

		UserData* data1 = (UserData*)pairHeader.actors[0]->userData;
		UserData* data2 = (UserData*)pairHeader.actors[1]->userData;

		if (data1->type == UserDataType::CharacterController || data2->type == UserDataType::CharacterController)
		{
			// TODO: пока не реализовано
			continue;
		}

		BaseActor* actor1 = (BaseActor*)data1->ptr;
		BaseActor* actor2 = (BaseActor*)data2->ptr;

		size_t numContacts = 0;
		ContactPairPoint* contactPoints = (ContactPairPoint*)alloca(sizeof(ContactPairPoint) * contactpair.contactCount);
		{
			// do we need contact data?
			if (actor1->GetWantsContactData() || actor2->GetWantsContactData()) 
			{
				PxContactPairPoint* points = (PxContactPairPoint*)alloca(sizeof(ContactPairPoint) * contactpair.contactCount);

				auto count = contactpair.extractContacts(points, contactpair.contactCount);
				for (int i = 0; i < contactpair.contactCount; i++) 
				{
					contactPoints[i] = points[i];
				}
				numContacts = count;
			}
		}

		//invoke events
		if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
		{

			actor1->OnColliderEnter(actor2, contactPoints, numContacts);
			actor2->OnColliderEnter(actor1, contactPoints, numContacts);
		}

		if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
		{
			actor1->OnColliderExit(actor2, contactPoints, numContacts);
			actor2->OnColliderExit(actor1, contactPoints, numContacts);
		}

		if (contactpair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
		{
			actor1->OnColliderPersist(actor2, contactPoints, numContacts);
			actor2->OnColliderPersist(actor1, contactPoints, numContacts);
		}
	}
}

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
	//sceneDesc.filterShader            = PxDefaultSimulationFilterShader;
	sceneDesc.filterShader            = FilterShader;
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