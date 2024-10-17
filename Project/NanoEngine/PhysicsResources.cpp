#include "stdafx.h"
#include "Core.h"
#include "PhysicsResources.h"
#include "Application.h"

namespace ph {

using namespace physx;

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
	m_material->setStaticFriction(value);
}

float Material::GetStaticFriction() const
{
	return m_material->getStaticFriction();
}

void Material::SetDynamicFriction(float value)
{
	m_material->setDynamicFriction(value);
}

float Material::GetDynamicFriction() const
{
	return m_material->getDynamicFriction();
}

void Material::SetRestitution(float value)
{
	m_material->setRestitution(value);
}

float Material::GetRestitution() const
{
	return m_material->getRestitution();
}

void Material::SetFrictionCombineMode(PhysicsCombineMode mode)
{
	m_material->setFrictionCombineMode(static_cast<PxCombineMode::Enum>(static_cast<std::underlying_type<PhysicsCombineMode>::type>(mode)));
}

void Material::SetRestitutionCombineMode(PhysicsCombineMode mode)
{
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
#pragma region [ Collider ]

Collider::Collider(EngineApplication& engine, MaterialPtr material)
	: m_engine(engine)
	, m_material(material)
{
}

Collider::~Collider()
{
	PX_RELEASE(m_collider);
}

void Collider::SetRelativeTransform(const glm::vec3& position, const glm::quat& rotation)
{
	m_collider->setLocalPose(PxTransform(
		PxVec3(position.x, position.y, position.z), 
		PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
}

std::pair<glm::vec3, glm::quat> Collider::GetRelativeTransform() const
{
	auto pose = m_collider->getLocalPose();
	return { glm::vec3(pose.p.x,pose.p.y,pose.p.z), glm::quat(pose.q.w, pose.q.x,pose.q.y,pose.q.z) };
}

void Collider::SetType(CollisionType type)
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

CollisionType Collider::GetType() const
{
	return m_collider->getFlags() & PxShapeFlag::eTRIGGER_SHAPE ? CollisionType::Trigger : CollisionType::Collider;
}

void Collider::SetQueryable(bool state)
{
	m_collider->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, state);
}

bool Collider::GetQueryable() const
{
	return m_collider->getFlags() & PxShapeFlag::eSCENE_QUERY_SHAPE;
}

MaterialPtr Collider::getMaterial() const
{
	if (m_material == nullptr || !m_material->IsValid())
		return m_engine.GetPhysicsSystem().GetDefaultMaterial();

	return m_material;
}

void Collider::updateFilterData(BaseActor* owner)
{
	PxFilterData filterData;
	filterData.word0 = owner->m_filterGroup; // word0 = own ID
	filterData.word1 = owner->m_filterMask;
	m_collider->setSimulationFilterData(filterData);
}

#pragma endregion

//=============================================================================
#pragma region [ Box Collider ]

BoxCollider::BoxCollider(EngineApplication& engine, BaseActor* owner, const BoxColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_extent = createInfo.extent;

	const PxBoxGeometry geom = { m_extent.x, m_extent.y, m_extent.z };
	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxActor(), geom, *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	updateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Sphere Collider ]

SphereCollider::SphereCollider(EngineApplication& engine, BaseActor* owner, const SphereColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxActor(), PxSphereGeometry(createInfo.radius), *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	updateFilterData(owner);
}

float SphereCollider::GetRadius() const
{
	return static_cast<const PxSphereGeometry&>(m_collider->getGeometry()).radius;
}

#pragma endregion

//=============================================================================
#pragma region [ Capsule Collider ]

CapsuleCollider::CapsuleCollider(EngineApplication& engine, BaseActor* owner, const CapsuleColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_radius = createInfo.radius;
	m_halfHeight = createInfo.halfHeight;

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxActor(), PxCapsuleGeometry(m_radius, m_halfHeight), *getMaterial()->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	updateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Mesh Collider ]

MeshCollider::MeshCollider(EngineApplication& engine, BaseActor* owner, const MeshColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
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

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxActor(), PxTriangleMeshGeometry(triMesh), *getMaterial()->GetPxMaterial());
	triMesh->release();
	updateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Convex Mesh Collider ]

ConvexMeshCollider::ConvexMeshCollider(EngineApplication& engine, BaseActor* owner, const ConvexMeshColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
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

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxActor(), PxConvexMeshGeometry(convMesh), *getMaterial()->GetPxMaterial());
	convMesh->release();
	updateFilterData(owner);
}

#pragma endregion

//=============================================================================
#pragma region [ Physics Actor ]

BaseActor::BaseActor(EngineApplication& engine)
	: m_engine(engine)
{
}

BaseActor::~BaseActor()
{
	m_colliders.clear();
	m_receivers.clear();
	if (m_actor != nullptr)
	{
		auto scene = m_actor->getScene();
		delete m_actor->userData;
		scene->removeActor(*m_actor);
		PX_RELEASE(m_actor);
	}
}

void BaseActor::AttachCollider(const BoxColliderCreateInfo& createInfo)
{
	AttachCollider(std::make_shared<BoxCollider>(m_engine, this, createInfo));
}

void BaseActor::AttachCollider(const SphereColliderCreateInfo& createInfo)
{
	AttachCollider(std::make_shared<SphereCollider>(m_engine, this, createInfo));
}

void BaseActor::AttachCollider(const CapsuleColliderCreateInfo& createInfo)
{
	AttachCollider(std::make_shared<CapsuleCollider>(m_engine, this, createInfo));
}

void BaseActor::AttachCollider(const MeshColliderCreateInfo& createInfo)
{
	AttachCollider(std::make_shared<MeshCollider>(m_engine, this, createInfo));
}

void BaseActor::AttachCollider(const ConvexMeshColliderCreateInfo& createInfo)
{
	AttachCollider(std::make_shared<ConvexMeshCollider>(m_engine, this, createInfo));
}

void BaseActor::AttachCollider(ColliderPtr collider)
{
	m_colliders.emplace_back(collider);
	physicsSetQueryLayer();
}

void BaseActor::SetSimulationEnabled(bool state)
{
	m_actor->setActorFlag(PxActorFlag::eDISABLE_SIMULATION, state);
}

bool BaseActor::GetSimulationEnabled() const
{
	return m_actor->getActorFlags() & PxActorFlag::eDISABLE_SIMULATION;
}

void BaseActor::AddReceiver(PhysicsCallbackPtr obj)
{
	m_receivers.insert(obj);
}

void BaseActor::OnColliderEnter(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderEnter)
		{
			receiver->OnColliderEnter(other, contactPoints, numContactPoints);
		}
	}
}

void BaseActor::OnColliderPersist(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderPersist)
		{
			receiver->OnColliderPersist(other, contactPoints, numContactPoints);
		}
	}
}

void BaseActor::OnColliderExit(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnColliderExit)
		{
			receiver->OnColliderExit(other, contactPoints, numContactPoints);
		}
	}
}

void BaseActor::OnTriggerEnter(BaseActor* other)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnTriggerEnter)
		{
			receiver->OnTriggerEnter(other);
		}
	}
}

void BaseActor::OnTriggerExit(BaseActor* other)
{
	for (auto& receiver : m_receivers)
	{
		if (receiver->OnTriggerExit)
		{
			receiver->OnTriggerExit(other);
		}
	}
}

void BaseActor::physicsSetQueryLayer()
{
	PxFilterData filterData = PhysicsFilterDataFromLayer(m_queryLayer);
	PhysicsForEachActorShape(m_actor, [&filterData](PxShape* shape) { shape->setQueryFilterData(filterData); });

	filterData.word0 = m_filterGroup; // word0 = own ID
	filterData.word1 = m_filterMask;

	collider->setSimulationFilterData(filterData);
}


#pragma endregion

//=============================================================================
#pragma region [ StaticBody ]

StaticBody::StaticBody(EngineApplication& engine, const StaticActorCreateInfo& createInfo)
	: BaseActor(engine)
{
	m_filterGroup = createInfo.filterGroup;
	m_filterMask = createInfo.filterMask;
	m_queryLayer = createInfo.queryLayer;
	
	PxTransform transform{ 
		PxVec3(createInfo.worldPosition.x, createInfo.worldPosition.y, createInfo.worldPosition.z), 
		PxQuat(createInfo.worldRotation.x, createInfo.worldRotation.y, createInfo.worldRotation.z, createInfo.worldRotation.w) };

	m_actor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidStatic(transform);
	m_actor->userData = new UserData{ .type = UserDataType::StaticBody, .ptr = this };
	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();
	scene->addActor(*m_actor);
}

#pragma endregion

//=============================================================================
#pragma region [ RigidBody ]

PxForceMode::Enum toPxForceMode(ForceMode mode)
{
	switch (mode)
	{
	case ForceMode::Force:        return PxForceMode::eFORCE;
	case ForceMode::Impulse:      return PxForceMode::eIMPULSE;
	case ForceMode::Velocity:     return PxForceMode::eVELOCITY_CHANGE;
	case ForceMode::Acceleration: return PxForceMode::eACCELERATION;
	}

	return PxForceMode::eFORCE;
}

PxForceMode::Enum toPxForceMode(PointForceMode mode)
{
	switch (mode)
	{
	case PointForceMode::Force:   return PxForceMode::eFORCE;
	case PointForceMode::Impulse: return PxForceMode::eIMPULSE;
	}

	return PxForceMode::eFORCE;
}

RigidBody::RigidBody(EngineApplication& engine, const RigidBodyCreateInfo& createInfo)
	: BaseActor(engine)
{
	m_filterGroup = createInfo.filterGroup;
	m_filterMask = createInfo.filterMask;
	m_queryLayer = createInfo.queryLayer;

	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();

	auto actor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidDynamic(PxTransform(PxVec3(0, 0, 0)));
	PxRigidBodyExt::updateMassAndInertia(*actor, createInfo.density);
	m_actor = actor;
	m_actor->userData = new UserData{ .type = UserDataType::RigidBody, .ptr = this };
	scene->addActor(*m_actor);
}

void RigidBody::SetFlags(RigidbodyFlag flags)
{
	m_flags = flags;
}

void RigidBody::SetWorldPose(const glm::vec3& pos, const glm::quat& rot)
{
	m_actor->setGlobalPose({ {pos.x, pos.y, pos.z}, PxQuat(rot.x, rot.y, rot.z, rot.w) });
}

void RigidBody::SetPosition(const glm::vec3& position)
{
	if (GetIsKinematic())
	{
		PxTransform target;
		if (!getInternal()->getKinematicTarget(target))
			target = PxTransform(PxIdentity);

		target.p = { position.x, position.y, position.z };
		getInternal()->setKinematicTarget(target);
	}
	else
	{
		SetWorldPose(position, GetRotation());
	}
}

void RigidBody::Rotate(const glm::quat& rotation)
{
	if (GetIsKinematic())
	{
		PxTransform target;
		if (!getInternal()->getKinematicTarget(target))
			target = PxTransform(PxIdentity);

		target.q.x = rotation.x;
		target.q.y = rotation.y;
		target.q.z = rotation.z;
		target.q.w = rotation.w;

		getInternal()->setKinematicTarget(target);
	}
	else
	{
		SetWorldPose(GetPosition(), rotation);
	}
}

std::pair<glm::vec3, glm::quat> RigidBody::GetWorldPose() const
{
	PxTransform t = m_actor->getGlobalPose();
	return std::make_pair(glm::vec3{ t.p.x, t.p.y, t.p.z }, glm::quat{ t.q.w, t.q.x,t.q.y,t.q.z });
}

glm::vec3 RigidBody::GetPosition() const
{
	PxTransform t = m_actor->getGlobalPose();
	return { t.p.x, t.p.y, t.p.z };
}

glm::quat RigidBody::GetRotation() const
{
	PxTransform t = m_actor->getGlobalPose();
	return glm::quat{ t.q.w, t.q.x,t.q.y,t.q.z };
}

void RigidBody::SetGravityEnabled(bool state)
{
	m_actor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !state);
}

bool RigidBody::GetGravityEnabled() const
{
	return m_actor->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY;
}

void RigidBody::SetSleepNotificationsEnabled(bool state)
{
	m_actor->setActorFlag(PxActorFlag::eSEND_SLEEP_NOTIFIES, state);
}

bool RigidBody::GetSleepNotificationsEnabled() const
{
	return m_actor->getActorFlags() & PxActorFlag::eSEND_SLEEP_NOTIFIES;
}

void RigidBody::SetLinearVelocity(const glm::vec3& newvel, bool autowake)
{
	getInternal()->setLinearVelocity(PxVec3(newvel.x, newvel.y, newvel.z), autowake);
}

glm::vec3 RigidBody::GetLinearVelocity() const
{
	auto vel = getInternal()->getLinearVelocity();
	auto ret = glm::vec3(vel.x, vel.y, vel.z);
	return ret;
}

void RigidBody::SetAngularVelocity(const glm::vec3& newvel, bool autowake)
{
	getInternal()->setAngularVelocity(PxVec3(newvel.x, newvel.y, newvel.z), autowake);
}

glm::vec3 RigidBody::GetAngularVelocity() const
{
	auto vel = getInternal()->getAngularVelocity();
	auto ret = glm::vec3(vel.x, vel.y, vel.z);
	return ret;
}

void RigidBody::SetLinearDamping(float drag)
{
	getInternal()->setLinearDamping(drag);
}

float RigidBody::GetLinearDamping() const
{
	return getInternal()->getLinearDamping();
}

void RigidBody::SetAngularDamping(float angDamp)
{
	getInternal()->setAngularDamping(angDamp);
}

float RigidBody::GetAngularDrag() const
{
	return getInternal()->getAngularDamping();
}

void RigidBody::SetInertiaTensor(const glm::vec3& tensor)
{
	if (((uint32_t)m_flags & (uint32_t)RigidbodyFlag::AutoTensors) != 0)
	{
		Warning("Attempting to set Rigidbody inertia tensor, but it has automatic tensor calculation turned on.");
		return;
	}

	getInternal()->setMassSpaceInertiaTensor({ tensor.x, tensor.y, tensor.z });
}

glm::vec3 RigidBody::GetInertiaTensor() const
{
	PxVec3 v = getInternal()->getMassSpaceInertiaTensor();
	return { v.x, v.y, v.z };
}

void RigidBody::SetMaxAngularVelocity(float maxVelocity)
{
	getInternal()->setMaxAngularVelocity(maxVelocity);
}

float RigidBody::GetMaxAngularVelocity() const
{
	return getInternal()->getMaxAngularVelocity();
}

void RigidBody::SetIsKinematic(bool kinematic)
{
	getInternal()->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, kinematic);
}

bool RigidBody::GetIsKinematic() const
{
	return ((uint32_t)getInternal()->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC) != 0;
}

void RigidBody::SetKinematicTarget(const glm::vec3& targetPos, const glm::quat& targetRot)
{
	PxTransform transform(PxVec3(targetPos.x, targetPos.y, targetPos.z), PxQuat(targetRot.x, targetRot.y, targetRot.z, targetRot.w));
	static_cast<PxRigidDynamic*>(m_actor)->setKinematicTarget(transform);
}

std::pair<glm::vec3, glm::quat> ph::RigidBody::GetKinematicTarget() const
{
	PxTransform trns;
	static_cast<PxRigidDynamic*>(m_actor)->getKinematicTarget(trns);
	return std::make_pair(glm::vec3(trns.p.x, trns.p.y, trns.p.z), glm::quat(trns.q.w, trns.q.x, trns.q.y, trns.q.z));
}

void RigidBody::Wake()
{
	getInternal()->wakeUp();
}

void RigidBody::Sleep()
{
	getInternal()->putToSleep();
}

bool RigidBody::IsSleeping() const
{
	return getInternal()->isSleeping();
}

void RigidBody::SetSleepThreshold(float threshold)
{
	getInternal()->setSleepThreshold(threshold);
}

float RigidBody::GetSleepThreshold() const
{
	return getInternal()->getSleepThreshold();
}

void RigidBody::SetAxisLock(uint16_t LockFlags)
{
	static_cast<PxRigidDynamic*>(m_actor)->setRigidDynamicLockFlags(static_cast<physx::PxRigidDynamicLockFlag::Enum>(LockFlags));
}

uint16_t RigidBody::GetAxisLock() const
{
	return static_cast<PxRigidDynamic*>(m_actor)->getRigidDynamicLockFlags();
}

void RigidBody::SetMass(float mass)
{
	if (((uint32_t)m_flags & (uint32_t)RigidbodyFlag::AutoMass) != 0)
	{
		Warning("Attempting to set Rigidbody mass, but it has automatic mass calculation turned on.");
		return;
	}
	getInternal()->setMass(mass);
}

float RigidBody::GetMass() const
{
	return getInternal()->getMass();
}

float RigidBody::GetMassInverse() const
{
	float rigidMass;
	rigidMass = getInternal()->getInvMass();
	return rigidMass;
}

void RigidBody::SetCenterOfMass(const glm::vec3& pos, const glm::quat& rot)
{
	if (((uint32_t)m_flags & (uint32_t)RigidbodyFlag::AutoTensors) != 0)
	{
		Warning("Attempting to set Rigidbody center of mass, but it has automatic tensor calculation turned on.");
		return;
	}

	getInternal()->setCMassLocalPose(PxTransform({ pos.x, pos.y, pos.z }, PxQuat( rot.x, rot.y, rot.z, rot.w )));
}

glm::vec3 RigidBody::GetCenterOfMassPosition() const
{
	PxTransform cMassTfrm = getInternal()->getCMassLocalPose();
	return { cMassTfrm.p.x,  cMassTfrm.p.y,  cMassTfrm.p.z };
}

glm::quat RigidBody::GetCenterOfMassRotation() const
{
	PxTransform cMassTfrm = getInternal()->getCMassLocalPose();
	return { cMassTfrm.q.w, cMassTfrm.q.x, cMassTfrm.q.y, cMassTfrm.q.z };
}

void RigidBody::SetPositionSolverCount(uint32_t count)
{
	getInternal()->setSolverIterationCounts(std::max(1u, count), GetVelocitySolverCount());
}

uint32_t RigidBody::GetPositionSolverCount() const
{
	UINT32 posCount = 1;
	UINT32 velCount = 1;
	getInternal()->getSolverIterationCounts(posCount, velCount);
	return posCount;
}

void RigidBody::SetVelocitySolverCount(uint32_t count)
{
	getInternal()->setSolverIterationCounts(GetPositionSolverCount(), std::max(1u, count));
}

uint32_t RigidBody::GetVelocitySolverCount() const
{
	UINT32 posCount = 1;
	UINT32 velCount = 1;

	getInternal()->getSolverIterationCounts(posCount, velCount);
	return velCount;
}

void RigidBody::AddForce(const glm::vec3& force, ForceMode mode)
{
	getInternal()->addForce(PxVec3(force.x, force.y, force.z), toPxForceMode(mode));
}

void RigidBody::AddTorque(const glm::vec3& torque, ForceMode mode)
{
	getInternal()->addTorque(PxVec3(torque.x, torque.y, torque.z), toPxForceMode(mode));
}

void RigidBody::AddForceAtPoint(const glm::vec3& force, const glm::vec3& position, PointForceMode mode)
{
	const PxVec3& pxForce = { force.x, force.y, force.z };
	const PxVec3& pxPos = { position.x, position.y, position.z };

	const PxTransform globalPose = getInternal()->getGlobalPose();
	PxVec3 centerOfMass = globalPose.transform(getInternal()->getCMassLocalPose().p);

	PxForceMode::Enum pxMode = toPxForceMode(mode);

	PxVec3 torque = (pxPos - centerOfMass).cross(pxForce);
	getInternal()->addForce(pxForce, pxMode);
	getInternal()->addTorque(torque, pxMode);
}

glm::vec3 RigidBody::GetVelocityAtPoint(const glm::vec3& point) const
{
	const PxVec3& pxPoint = { point.x, point.y, point.z };

	const PxTransform globalPose = getInternal()->getGlobalPose();
	const PxVec3 centerOfMass = globalPose.transform(getInternal()->getCMassLocalPose().p);
	const PxVec3 rpoint = pxPoint - centerOfMass;

	PxVec3 velocity = getInternal()->getLinearVelocity();
	velocity += getInternal()->getAngularVelocity().cross(rpoint);

	return { velocity.x, velocity.y, velocity.z };
}

void RigidBody::ClearAllForces()
{
	getInternal()->clearForce();
}

void RigidBody::ClearAllTorques()
{
	getInternal()->clearTorque();
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
		m_material = m_engine.GetPhysicsSystem().GetDefaultMaterial();
	}

	physx::PxCapsuleControllerDesc desc;
	desc.position = { createInfo.position.x, createInfo.position.y, createInfo.position.z };
	desc.stepOffset = 0.0f;
	desc.material = m_material->GetPxMaterial();
	// https://nvidia-omniverse.github.io/PhysX/physx/5.1.0/docs/CharacterControllers.html#character-volume
	desc.radius = createInfo.radius;
	desc.height = createInfo.height;

	m_controller = m_engine.GetPhysicsScene().GetPxControllerManager()->createController(desc);

	const PxFilterData filterData = PhysicsFilterDataFromLayer(createInfo.queryLayer);
	m_rigidbody = m_controller->getActor();
	PhysicsForEachActorShape(m_rigidbody, [&filterData](PxShape* shape) { shape->setQueryFilterData(filterData); });

	m_rigidbody->userData = new UserData{ .type = UserDataType::CharacterController, .ptr = this };
}

CharacterController::~CharacterController()
{
	delete m_rigidbody->userData;
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

} // namespace ph