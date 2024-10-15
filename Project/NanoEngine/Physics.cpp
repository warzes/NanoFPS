#include "stdafx.h"
#include "Core.h"
#include "Physics.h"
#include "Physics2.h"
#include "PhysicsSystem.h"
#include "Application.h"

namespace ph {

using namespace physx;

//=============================================================================
#pragma region [ Simulation Event Callback ]

void PhysicsSimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
	for (uint32_t i = 0; i < count; i++)
	{
		const PxTriggerPair& pair = pairs[i];
		if (pair.flags == PxTriggerPairFlag::eREMOVED_SHAPE_OTHER || pair.flags == PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER)
		{
			// ignore events caused by shape removal
			continue;
		}

		auto triggerActor = static_cast<PhysicsActor*>(pair.triggerActor->userData);
		auto otherActor = static_cast<PhysicsActor*>(pair.otherActor->userData);

		switch (pair.status)
		{
		case PxPairFlag::eNOTIFY_TOUCH_FOUND:
			triggerActor->OnTriggerEnter(otherActor);
			break;
		case PxPairFlag::eNOTIFY_TOUCH_LOST:
			triggerActor->OnTriggerExit(otherActor);
			break;
		default:
			Error("Invalid trigger pair status " + std::to_string(pair.status));
			break;
		}
	}
}

#pragma endregion

//=============================================================================
#pragma region [ Layers ]

void PhysicsSetQueryLayer(PxRigidActor* actor, PhysicsLayer layer)
{
	const PxFilterData filterData = PhysicsFilterDataFromLayer(layer);
	PhysicsForEachActorShape(actor, [&filterData](PxShape* shape) { shape->setQueryFilterData(filterData); });
}

#pragma endregion

//=============================================================================
#pragma region Physics Material

Material::Material(EngineApplication& engine, const MaterialCreateInfo& createInfo)
	: m_engine(engine)
{
	m_material = m_engine.GetPhysicsSystem().GetPxPhysics()->createMaterial(createInfo.staticFriction, createInfo.dynamicFriction, createInfo.restitution);
}

Material::~Material()
{
	PX_RELEASE(m_material);
}

void Material::SetStaticFriction(float value)
{
	assert(m_material);
	m_material->setStaticFriction(value);
}

float Material::GetStaticFriction() const
{
	assert(m_material);
	return m_material->getStaticFriction();
}

void Material::SetDynamicFriction(float value)
{
	assert(m_material);
	m_material->setDynamicFriction(value);
}

float Material::GetDynamicFriction() const
{
	assert(m_material);
	return m_material->getDynamicFriction();
}

void Material::SetRestitution(float value)
{
	assert(m_material);
	m_material->setRestitution(value);
}

float Material::GetRestitution() const
{
	assert(m_material);
	return m_material->getRestitution();
}

void Material::SetFrictionCombineMode(PhysicsCombineMode mode)
{
	assert(m_material);
	m_material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(static_cast<std::underlying_type<PhysicsCombineMode>::type>(mode)));
}

void Material::SetRestitutionCombineMode(PhysicsCombineMode mode)
{
	assert(m_material);
	m_material->setRestitutionCombineMode(static_cast<PxCombineMode::Enum>(static_cast<std::underlying_type<PhysicsCombineMode>::type>(mode)));
}

PhysicsCombineMode Material::GetFrictionCombineMode() const
{
	assert(m_material);
	return PhysicsCombineMode(m_material->getFrictionCombineMode());
}

PhysicsCombineMode Material::GetRestitutionCombineMode() const
{
	assert(m_material);
	return PhysicsCombineMode(m_material->getRestitutionCombineMode());
}

#pragma endregion

//=============================================================================
#pragma region [ Physics Body ]

PhysicsBody::PhysicsBody(EngineApplication& engine)
	: m_engine(engine)
{
}

PhysicsBody::~PhysicsBody()
{
	m_colliders.clear();
	if (m_rigidActor != nullptr)
	{
		auto scene = m_rigidActor->getScene();
		scene->lockWrite();
		scene->removeActor(*(m_rigidActor));
		scene->unlockWrite();
		PX_RELEASE(m_rigidActor);

		for (auto& receiver : m_receivers) // TODO:
		{
			//receiver->OnUnregisterBody(otherwayHandle);
		}
	}
}

void PhysicsBody::AttachCollider(const BoxColliderCreateInfo& createInfo)
{
	m_colliders.emplace_back(std::make_shared<BoxCollider>(m_engine, this, createInfo));
	PhysicsSetQueryLayer(m_rigidActor, m_queryLayer);
}

void PhysicsBody::AttachCollider(const SphereColliderCreateInfo& createInfo)
{
	m_colliders.emplace_back(std::make_shared<SphereCollider>(m_engine, this, createInfo));
	PhysicsSetQueryLayer(m_rigidActor, m_queryLayer);
}

void PhysicsBody::AttachCollider(const CapsuleColliderCreateInfo& createInfo)
{
	m_colliders.emplace_back(std::make_shared<CapsuleCollider>(m_engine, this, createInfo));
	PhysicsSetQueryLayer(m_rigidActor, m_queryLayer);
}

void PhysicsBody::AttachCollider(const MeshColliderCreateInfo& createInfo)
{
	m_colliders.emplace_back(std::make_shared<MeshCollider>(m_engine, this, createInfo));
	PhysicsSetQueryLayer(m_rigidActor, m_queryLayer);
}

void PhysicsBody::AttachCollider(const ConvexMeshColliderCreateInfo& createInfo)
{
	m_colliders.emplace_back(std::make_shared<ConvexMeshCollider>(m_engine, this, createInfo));
	PhysicsSetQueryLayer(m_rigidActor, m_queryLayer);
}

std::pair<glm::vec3, glm::quat> PhysicsBody::GetDynamicsWorldPose() const
{
	PxTransform t;
	lockRead([&] { t = m_rigidActor->getGlobalPose(); });
	return std::make_pair(glm::vec3{ t.p.x, t.p.y, t.p.z }, glm::quat{ t.q.w, t.q.x,t.q.y,t.q.z });
}

void PhysicsBody::SetDynamicsWorldPose(const glm::vec3& pos, const glm::quat& worldrot) const
{
	m_rigidActor->getScene()->lockWrite();
	m_rigidActor->setGlobalPose(PxTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(worldrot.x, worldrot.y, worldrot.z, worldrot.w)));
	m_rigidActor->getScene()->unlockWrite();
}

void PhysicsBody::SetGravityEnabled(bool state)
{
	lockWrite([&] { m_rigidActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !state); });
}

bool PhysicsBody::GetGravityEnabled() const
{
	return m_rigidActor->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY;
}

void PhysicsBody::SetSleepNotificationsEnabled(bool state)
{
	m_rigidActor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, state);
}

bool PhysicsBody::GetSleepNotificationsEnabled() const
{
	return m_rigidActor->getActorFlags() & PxActorFlag::eSEND_SLEEP_NOTIFIES;
}

void PhysicsBody::SetSimulationEnabled(bool state)
{
	m_rigidActor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, state);
}

bool PhysicsBody::GetSimulationEnabled() const
{
	return m_rigidActor->getActorFlags() & PxActorFlag::eDISABLE_SIMULATION;
}

void PhysicsBody::OnColliderEnter(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderEnter)
		{
			receiver->OnColliderEnter(other, contactPoints, numContactPoints);
		}
	}
}

void PhysicsBody::OnColliderPersist(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderPersist)
		{
			receiver->OnColliderPersist(other, contactPoints, numContactPoints);
		}
	}
}

void PhysicsBody::OnColliderExit(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderExit)
		{
			receiver->OnColliderExit(other, contactPoints, numContactPoints);
		}
	}
}

void PhysicsBody::OnTriggerEnter(PhysicsBody* other)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnTriggerEnter)
		{
			receiver->OnTriggerEnter(other);
		}
	}
}

void PhysicsBody::OnTriggerExit(PhysicsBody* other)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnTriggerExit)
		{
			receiver->OnTriggerExit(other);
		}
	}
}

#pragma endregion

//=============================================================================
#pragma region [ RigidBody ]

RigidBody::RigidBody(EngineApplication& engine, const RigidBodyCreateInfo& createInfo)
	: PhysicsBody(engine)
{
	m_filterGroup = createInfo.filterGroup;
	m_filterMask = createInfo.filterMask;
	m_queryLayer = createInfo.queryLayer;

	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();

	auto actor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidDynamic(PxTransform(PxVec3(0, 0, 0)));
	PxRigidBodyExt::updateMassAndInertia(*actor, createInfo.density);

	m_rigidActor = actor;
	scene->lockWrite();
	scene->addActor(*m_rigidActor);
	scene->unlockWrite();
	SetDynamicsWorldPose(createInfo.worldPosition, createInfo.worldRotation);
}

glm::vec3 RigidBody::GetLinearVelocity() const
{
	m_rigidActor->getScene()->lockRead();
	auto vel = static_cast<PxRigidBody*>(m_rigidActor)->getLinearVelocity();
	auto ret = glm::vec3(vel.x, vel.y, vel.z);
	m_rigidActor->getScene()->unlockRead();
	return ret;
}

glm::vec3 RigidBody::GetAngularVelocity() const
{
	m_rigidActor->getScene()->lockRead();
	auto vel = static_cast<PxRigidBody*>(m_rigidActor)->getAngularVelocity();
	auto ret = glm::vec3(vel.x, vel.y, vel.z);
	m_rigidActor->getScene()->unlockRead();
	return ret;
}

void RigidBody::SetLinearVelocity(const glm::vec3& newvel, bool autowake)
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->setLinearVelocity(PxVec3(newvel.x, newvel.y, newvel.z), autowake); });
}

void RigidBody::SetAngularVelocity(const glm::vec3& newvel, bool autowake)
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->setAngularVelocity(PxVec3(newvel.x, newvel.y, newvel.z), autowake); });
}

void RigidBody::SetAngularDamping(float angDamp)
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->setAngularDamping(angDamp); });
}

void RigidBody::SetKinematicTarget(const glm::vec3& targetPos, const glm::quat& targetRot)
{
	PxTransform transform(PxVec3(targetPos.x, targetPos.y, targetPos.z), PxQuat(targetRot.x, targetRot.y, targetRot.z, targetRot.w));
	m_rigidActor->getScene()->lockWrite();
	static_cast<PxRigidDynamic*>(m_rigidActor)->setKinematicTarget(transform);
	m_rigidActor->getScene()->unlockWrite();
}

std::pair<glm::vec3, glm::quat> ph::RigidBody::GetKinematicTarget() const
{
	PxTransform trns;
	m_rigidActor->getScene()->lockRead();
	static_cast<PxRigidDynamic*>(m_rigidActor)->getKinematicTarget(trns);
	m_rigidActor->getScene()->unlockRead();
	return std::make_pair(glm::vec3(trns.p.x, trns.p.y, trns.p.z), glm::quat(trns.q.w, trns.q.x, trns.q.y, trns.q.z));
}

void RigidBody::Wake()
{
	static_cast<PxRigidDynamic*>(m_rigidActor)->wakeUp();
}

void RigidBody::Sleep()
{
	static_cast<PxRigidDynamic*>(m_rigidActor)->putToSleep();
}

bool RigidBody::IsSleeping() const
{
	return static_cast<PxRigidDynamic*>(m_rigidActor)->isSleeping();
}

void RigidBody::SetAxisLock(uint16_t LockFlags)
{
	static_cast<PxRigidDynamic*>(m_rigidActor)->setRigidDynamicLockFlags(static_cast<physx::PxRigidDynamicLockFlag::Enum>(LockFlags));
}

uint16_t RigidBody::GetAxisLock() const
{
	return static_cast<PxRigidDynamic*>(m_rigidActor)->getRigidDynamicLockFlags();
}

void RigidBody::SetMass(float mass)
{
	static_cast<PxRigidDynamic*>(m_rigidActor)->setMass(mass);
}

float RigidBody::GetMass() const
{
	float mass;
	lockRead([&] { mass = static_cast<PxRigidDynamic*>(m_rigidActor)->getMass(); });
	return mass;
}

float RigidBody::GetMassInverse() const
{
	float rigidMass;
	lockRead([&] { rigidMass = static_cast<PxRigidDynamic*>(m_rigidActor)->getInvMass(); });
	return rigidMass;
}

void RigidBody::AddForce(const glm::vec3& force)
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->addForce(PxVec3(force.x, force.y, force.z)); });
}

void RigidBody::AddTorque(const glm::vec3& torque)
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->addTorque(PxVec3(torque.x, torque.y, torque.z)); });
}

void RigidBody::ClearAllForces()
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->clearForce(); });
}

void RigidBody::ClearAllTorques()
{
	lockWrite([&] { static_cast<PxRigidDynamic*>(m_rigidActor)->clearTorque(); });
}

#pragma endregion

//=============================================================================
#pragma region [ StaticBody ]

StaticBody::StaticBody(EngineApplication& engine, const StaticBodyCreateInfo& createInfo)
	: PhysicsBody(engine)
{
	m_filterGroup = createInfo.filterGroup;
	m_filterMask = createInfo.filterMask;
	m_queryLayer = createInfo.queryLayer;

	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();

	PxTransform transform{ PxVec3(createInfo.worldPosition.x, createInfo.worldPosition.y, createInfo.worldPosition.z), PxQuat(createInfo.worldRotation.x, createInfo.worldRotation.y, createInfo.worldRotation.z, createInfo.worldRotation.w)};

	m_rigidActor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidStatic(transform);
	scene->lockWrite();
	scene->addActor(*(m_rigidActor));
	scene->unlockWrite();
}

#pragma endregion

//=============================================================================
#pragma region [ Character Controller ]

CharacterController::CharacterController(EngineApplication& engine, const CharacterControllerCreateInfo& createInfo)
	: m_engine(engine)
{
	m_material = createInfo.material;
	if (!m_material)
	{
		m_material = m_engine.GetPhysicsScene().GetDefaultMaterial();
	}

	physx::PxCapsuleControllerDesc desc;
	desc.position = { createInfo.position.x, createInfo.position.y, createInfo.position.z };
	desc.stepOffset = 0.0f;
	desc.material = m_material->GetPxMaterial();
	// https://nvidia-omniverse.github.io/PhysX/physx/5.1.0/docs/CharacterControllers.html#character-volume
	desc.radius = createInfo.radius;
	desc.height = createInfo.height;

	m_controller = m_engine.GetPhysicsScene().GetControllerManager()->createController(desc);

	physx::PxRigidDynamic* rigidbody = m_controller->getActor();
	PhysicsSetQueryLayer(rigidbody, createInfo.queryLayer);

	rigidbody->userData = createInfo.userData;
}

CharacterController::~CharacterController()
{
	PX_RELEASE(m_controller);
}

void CharacterController::Move(const glm::vec3& disp, float minDist, float elapsedTime)
{
	m_controller->move({ disp.x, disp.y, disp.z }, minDist, elapsedTime, physx::PxControllerFilters());
}

glm::vec3 CharacterController::GetPosition() const
{
	auto pos = m_controller->getPosition();
	return { pos.x, pos.y, pos.z };
}

float CharacterController::GetSlopeLimit() const
{
	return m_controller->getSlopeLimit();
}

#pragma endregion

//=============================================================================
#pragma region [ Physics Callback ]


#pragma endregion

//=============================================================================
#pragma region [ Physics Collider ]

ColliderOLD::ColliderOLD(EngineApplication& engine, MaterialPtr material)
	: m_engine(engine)
	, m_material(material)
{
}

ColliderOLD::~ColliderOLD()
{
	PX_RELEASE(m_collider);
}

void ColliderOLD::SetType(CollisionType type)
{
	switch (type)
	{
	case CollisionType::Collider:
		m_collider->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
		m_collider->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
		break;
	case CollisionType::Trigger:
		m_collider->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
		m_collider->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
		break;
	}
}

CollisionType ColliderOLD::GetType() const
{
	return m_collider->getFlags() & PxShapeFlag::eTRIGGER_SHAPE ? CollisionType::Trigger : CollisionType::Collider;
}

void ColliderOLD::SetQueryable(bool state)
{
	m_collider->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, state);
}

bool ColliderOLD::GetQueryable() const
{
	return m_collider->getFlags() & PxShapeFlag::eSCENE_QUERY_SHAPE;
}

void ColliderOLD::SetRelativeTransform(const glm::vec3& position, const glm::quat& rotation)
{
	m_collider->setLocalPose(PxTransform(PxVec3(position.x, position.y, position.z), PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
}

std::pair<glm::vec3, glm::quat> ColliderOLD::GetRelativeTransform() const
{
	auto pose = m_collider->getLocalPose();
	return { glm::vec3(pose.p.x,pose.p.y,pose.p.z),glm::quat(pose.q.w, pose.q.x,pose.q.y,pose.q.z) };
}

void ColliderOLD::UpdateFilterData(PhysicsBody* owner)
{
	PxFilterData filterData;
	filterData.word0 = owner->m_filterGroup; // word0 = own ID
	filterData.word1 = owner->m_filterMask;
	m_collider->setSimulationFilterData(filterData);
}

MaterialPtr ph::ColliderOLD::getMaterial()
{
	if (m_material == nullptr || !m_material->IsValid())
		return m_engine.GetPhysicsScene().GetDefaultMaterial();

	return m_material;
}

#pragma endregion

//=============================================================================
#pragma region [ Box Collider ]

BoxCollider::BoxCollider(EngineApplication& engine, PhysicsBody* owner, const BoxColliderCreateInfo& createInfo)
	: ColliderOLD(engine, createInfo.material)
{
	m_extent = createInfo.extent;

	const PxBoxGeometry geom = { m_extent.x, m_extent.y, m_extent.z };
	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), geom, *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Sphere Collider ]

SphereCollider::SphereCollider(EngineApplication& engine, PhysicsBody* owner, const SphereColliderCreateInfo& createInfo)
	: ColliderOLD(engine, createInfo.material)
{
	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxSphereGeometry(createInfo.radius), *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

float SphereCollider::GetRadius() const
{
	return static_cast<const PxSphereGeometry&>(m_collider->getGeometry()).radius;
}

#pragma endregion

//=============================================================================
#pragma region [ Capsule Collider ]

CapsuleCollider::CapsuleCollider(EngineApplication& engine, PhysicsBody* owner, const CapsuleColliderCreateInfo& createInfo)
	: ColliderOLD(engine, createInfo.material)
{
	m_radius = createInfo.radius;
	m_halfHeight = createInfo.halfHeight;

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxCapsuleGeometry(m_radius, m_halfHeight), *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Mesh Collider ]

MeshCollider::MeshCollider(EngineApplication& engine, PhysicsBody* owner, const MeshColliderCreateInfo& createInfo)
	: ColliderOLD(engine, createInfo.material)
{
	physx::PxTriangleMeshDesc desc{};
	desc.setToDefault();
	// vertex data
	desc.points.data = &createInfo.vertices[0];
	desc.points.stride = sizeof(createInfo.vertices[0]);
	assert(createInfo.vertices.size() < std::numeric_limits<physx::PxU32>::max());
	desc.points.count = static_cast<physx::PxU32>(createInfo.vertices.size());

	// index data
	assert(createInfo.indices.size() / 3 < std::numeric_limits<physx::PxU32>::max());
	desc.triangles.count = static_cast<physx::PxU32>(createInfo.indices.size() / 3);
	desc.triangles.stride = 3 * sizeof(createInfo.indices[0]);
	desc.triangles.data = &createInfo.indices[0];

	if (sizeof(createInfo.indices[0]) == sizeof(uint16_t))
	{
		desc.flags = PxMeshFlag::e16_BIT_INDICES; // TODO:
	}  //otherwise assume 32 bit

	physx::PxDefaultMemoryOutputStream buffer;
	const physx::PxCookingParams cookingParams(engine.GetPhysicsSystem().GetPxPhysics()->getTolerancesScale()); // TODO: save init
	if (!PxCookTriangleMesh(cookingParams, desc, buffer))
	{
		Fatal("Failed to create triangle PhysX mesh.");
		return;
	}

	physx::PxDefaultMemoryInputData input(buffer.getData(), buffer.getSize());
	physx::PxTriangleMesh* triMesh = engine.GetPhysicsSystem().GetPxPhysics()->createTriangleMesh(input);

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxTriangleMeshGeometry(triMesh), *getMaterial()->GetPxMaterial());
	triMesh->release();
	UpdateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Convex Mesh Collider ]

ConvexMeshCollider::ConvexMeshCollider(EngineApplication& engine, PhysicsBody* owner, const ConvexMeshColliderCreateInfo& createInfo)
	: ColliderOLD(engine, createInfo.material)
{
	physx::PxBoundedData pointdata;
	pointdata.data = &createInfo.vertices[0];
	pointdata.stride = sizeof(createInfo.vertices[0]);
	assert(createInfo.vertices.size() < std::numeric_limits<physx::PxU32>::max());
	pointdata.count = static_cast<physx::PxU32>(createInfo.vertices.size());

	PxConvexMeshDesc desc;
	desc.setToDefault();
	desc.points = pointdata;
	desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

	physx::PxDefaultMemoryOutputStream buffer;
	const physx::PxCookingParams cookingParams(engine.GetPhysicsSystem().GetPxPhysics()->getTolerancesScale()); // TODO: save init
	if (!PxCookConvexMesh(cookingParams, desc, buffer))
	{
		Fatal("Failed to create triangle PhysX mesh.");
		return;
	}

	physx::PxDefaultMemoryInputData input(buffer.getData(), buffer.getSize());
	physx::PxConvexMesh* convMesh = engine.GetPhysicsSystem().GetPxPhysics()->createConvexMesh(input);

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxConvexMeshGeometry(convMesh), *getMaterial()->GetPxMaterial());
	convMesh->release();
	UpdateFilterData(owner);
}

#pragma endregion

} // namespace ph