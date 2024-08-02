#include "PhysicsSimulationEventCallback.h"
#include "GameScene.h"

void PhysicsSimulationEventCallback::onConstraintBreak([[maybe_unused]] physx::PxConstraintInfo* constraints, [[maybe_unused]] physx::PxU32 count)
{
}

void PhysicsSimulationEventCallback::onWake([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count)
{
}

void PhysicsSimulationEventCallback::onSleep([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count)
{
}

void PhysicsSimulationEventCallback::onContact(
	[[maybe_unused]] const physx::PxContactPairHeader& pairHeader,
	[[maybe_unused]] const physx::PxContactPair* pairs,
	[[maybe_unused]] physx::PxU32 nbPairs
)
{
}

void PhysicsSimulationEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	for (int i = 0; i < count; i++)
	{
		const physx::PxTriggerPair& pair = pairs[i];
		if (pair.flags == physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER || pair.flags == physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER)
		{
			// ignore events caused by shape removal
			continue;
		}

		auto triggerActor = static_cast<Actor*>(pair.triggerActor->userData);
		auto otherActor = static_cast<Actor*>(pair.otherActor->userData);

		switch (pair.status)
		{
		case physx::PxPairFlag::eNOTIFY_TOUCH_FOUND:
			triggerActor->OnTriggerEnter(otherActor);
			break;
		case physx::PxPairFlag::eNOTIFY_TOUCH_LOST:
			triggerActor->OnTriggerExit(otherActor);
			break;
		default:
			Error("Invalid trigger pair status " + std::to_string(pair.status));
			break;
		}
	}
}

void PhysicsSimulationEventCallback::onAdvance(
	[[maybe_unused]] const physx::PxRigidBody* const* bodyBuffer,
	[[maybe_unused]] const physx::PxTransform* poseBuffer,
	[[maybe_unused]] physx::PxU32 count
)
{
}