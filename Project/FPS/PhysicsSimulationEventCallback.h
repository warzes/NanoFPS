#pragma once

#include "NanoEngineVK.h"

class PhysicsSimulationEventCallback final : public physx::PxSimulationEventCallback
{
public:
	void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) final;
	void onWake(physx::PxActor** actors, physx::PxU32 count) final;
	void onSleep(physx::PxActor** actors, physx::PxU32 count) final;
	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) final;
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) final;
	void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, physx::PxU32 count) final;
};