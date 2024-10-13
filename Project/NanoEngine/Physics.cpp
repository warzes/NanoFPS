#include "stdafx.h"
#include "Core.h"
#include "Physics.h"
#include "Application.h"

namespace ph {

using namespace physx;

#pragma comment( lib, "PhysX_64.lib" )
#pragma comment( lib, "PhysXFoundation_64.lib" )
#pragma comment( lib, "PhysXCooking_64.lib" )
#pragma comment( lib, "PhysXCommon_64.lib" )

//#pragma comment( lib, "LowLevel_static_64.lib" )
//#pragma comment( lib, "LowLevelAABB_static_64.lib" )
//#pragma comment( lib, "LowLevelDynamics_static_64.lib" )
#pragma comment( lib, "PhysXCharacterKinematic_static_64.lib" )
#pragma comment( lib, "PhysXExtensions_static_64.lib" )
#pragma comment( lib, "PhysXPvdSDK_static_64.lib" )

//=============================================================================
#pragma region [ Error Callback ]

void info(const std::string& error, const std::string& message) noexcept
{
	Print("PhysX " + error + ": " + message);
}

void warning(const std::string& error, const std::string& message) noexcept
{
	Warning("PhysX " + error + ": " + message);
}

void error(const std::string& error, const std::string& message) noexcept
{
	Error("PhysX " + error + ": " + message);
}

void PhysicsErrorCallback::reportError(PxErrorCode::Enum code, const char* message, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	const char* msgType = "Unknown Error";
	void (*loggingCallback)(const std::string&, const std::string&) = error;
	switch (code)
	{
	case physx::PxErrorCode::eNO_ERROR:          msgType = "No Error";            loggingCallback = info; break;
	case physx::PxErrorCode::eDEBUG_INFO:        msgType = "Debug Info";          loggingCallback = info; break;
	case physx::PxErrorCode::eDEBUG_WARNING:     msgType = "Debug Warning";       loggingCallback = warning; break;
	case physx::PxErrorCode::eINVALID_PARAMETER: msgType = "Invalid Parameter";   loggingCallback = error; break;
	case physx::PxErrorCode::eINVALID_OPERATION: msgType = "Invalid Operation";   loggingCallback = error; break;
	case physx::PxErrorCode::eOUT_OF_MEMORY:     msgType = "Out of Memory";       loggingCallback = error; break;
	case physx::PxErrorCode::eINTERNAL_ERROR:    msgType = "Internal Error";      loggingCallback = error; break;
	case physx::PxErrorCode::eABORT:             msgType = "Abort";               loggingCallback = error; break;
	case physx::PxErrorCode::ePERF_WARNING:      msgType = "Performance Warning"; loggingCallback = warning; break;
	default: break;
	}
	loggingCallback(msgType, message);
}

#pragma endregion

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

Material::Material(EngineApplication& engine, float staticFriction, float dynamicFriction, float restitution)
	: m_engine(engine)
{
	m_material = m_engine.GetPhysicsSystem().GetPxPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);
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
	if (m_rigidActor != nullptr)
	{
		auto scene = m_rigidActor->getScene();
		scene->lockWrite();
		scene->removeActor(*(m_rigidActor));
		scene->unlockWrite();
		m_rigidActor->release();
		m_rigidActor = nullptr;

		for (auto& receiver : m_receivers)
		{
			//receiver->OnUnregisterBody(otherwayHandle);
		}
	}
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

	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();

	m_rigidActor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidDynamic(PxTransform(PxVec3(0, 0, 0)));
	scene->lockWrite();
	scene->addActor(*(m_rigidActor));
	scene->unlockWrite();
	SetDynamicsWorldPose(createInfo.worldPosition, createInfo.worldRotation);
}

RigidBody::~RigidBody()
{
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

	auto scene = m_engine.GetPhysicsSystem().GetScene().GetPxScene();

	PxTransform transform{ PxVec3(createInfo.worldPosition.x, createInfo.worldPosition.y, createInfo.worldPosition.z), PxQuat(createInfo.worldRotation.x, createInfo.worldRotation.y, createInfo.worldRotation.z, createInfo.worldRotation.w)};

	m_rigidActor = m_engine.GetPhysicsSystem().GetPxPhysics()->createRigidStatic(transform);
	scene->lockWrite();
	scene->addActor(*(m_rigidActor));
	scene->unlockWrite();
}

StaticBody::~StaticBody()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Character Body ]

CharacterControllers::CharacterControllers(EngineApplication& engine, const CharacterControllersCreateInfo& createInfo)
{
	тут
		возможно не нужно на
}

CharacterControllers::~CharacterControllers()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Physics Callback ]


#pragma endregion

//=============================================================================
#pragma region [ Physics Collider ]

Collider::Collider(EngineApplication& engine, MaterialPtr material)
	: m_engine(engine)
	, m_material(material)
{
	if (m_material == nullptr)
		m_material = engine.GetPhysicsScene().GetDefaultMaterial();
	else if (!m_material->IsValid())
	{
		Warning("Physics Material not valid!!! Set default materials");
		m_material = engine.GetPhysicsScene().GetDefaultMaterial();
	}
}

Collider::~Collider()
{
	//PX_RELEASE(m_collider);
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

void Collider::SetRelativeTransform(const glm::vec3& position, const glm::quat& rotation)
{
	m_collider->setLocalPose(PxTransform(PxVec3(position.x, position.y, position.z), PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
}

Transformation Collider::GetRelativeTransform() const
{
	auto pose = m_collider->getLocalPose();
	return Transformation{ glm::vec3(pose.p.x,pose.p.y,pose.p.z),glm::quat(pose.q.w, pose.q.x,pose.q.y,pose.q.z) };
}

void Collider::UpdateFilterData(PhysicsBody* owner)
{
	PxFilterData filterData;
	filterData.word0 = owner->m_filterGroup; // word0 = own ID
	filterData.word1 = owner->m_filterMask;
	m_collider->setSimulationFilterData(filterData);
}

#pragma endregion

//=============================================================================
#pragma region [ Box Collider ]

BoxCollider::BoxCollider(EngineApplication& engine, PhysicsBody* owner, const BoxColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_extent = createInfo.extent;

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxBoxGeometry(m_extent.x, m_extent.y, m_extent.z), *m_material->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

BoxCollider::~BoxCollider()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Sphere Collider ]

SphereCollider::SphereCollider(EngineApplication& engine, PhysicsBody* owner, const SphereColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxSphereGeometry(createInfo.radius), *m_material->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

SphereCollider::~SphereCollider()
{
}

float SphereCollider::GetRadius() const
{
	return static_cast<const PxSphereGeometry&>(m_collider->getGeometry()).radius;
}

#pragma endregion

//=============================================================================
#pragma region [ Capsule Collider ]

CapsuleCollider::CapsuleCollider(EngineApplication& engine, PhysicsBody* owner, const CapsuleColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
	m_radius = createInfo.radius;
	m_halfHeight = createInfo.halfHeight;

	m_collider = PxRigidActorExt::createExclusiveShape(*owner->GetPxRigidActor(), PxCapsuleGeometry(m_radius, m_halfHeight), *m_material->GetPxMaterial());

	SetRelativeTransform(createInfo.position, createInfo.rotation);
	UpdateFilterData(owner);
}

CapsuleCollider::~CapsuleCollider()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Mesh Collider ]

MeshCollider::MeshCollider(EngineApplication& engine, PhysicsBody* owner, const MeshColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
}

MeshCollider::~MeshCollider()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Convex Mesh Collider ]

ConvexMeshCollider::ConvexMeshCollider(EngineApplication& engine, PhysicsBody* owner, const ConvexMeshColliderCreateInfo& createInfo)
	: Collider(engine, createInfo.material)
{
}

ConvexMeshCollider::~ConvexMeshCollider()
{
}

#pragma endregion

//=============================================================================
#pragma region [ Physics Scene ]

PhysicsScene::PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine)
	: m_engine(engine)
	, m_system(physicsEngine)
{
}

bool PhysicsScene::Setup(const PhysicsSceneCreateInfo& createInfo)
{
	m_physics = m_system.m_physics;

	m_cpuDispatcher = PxDefaultCpuDispatcherCreate(createInfo.cpuDispatcherNum);
	if (!m_cpuDispatcher)
	{
		Fatal("Failed to create default PhysX CPU dispatcher.");
		return false;
	}

	PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity                 = { createInfo.gravity.x, createInfo.gravity.y, createInfo.gravity.z };
	sceneDesc.cpuDispatcher           = m_cpuDispatcher;
	sceneDesc.filterShader            = PxDefaultSimulationFilterShader;
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

	m_defaultMaterial = m_system.CreateMaterial(createInfo.defaultMaterial.staticFriction, createInfo.defaultMaterial.dynamicFriction, createInfo.defaultMaterial.restitution);
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
	PX_RELEASE(m_cpuDispatcher);
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

physx::PxController* PhysicsScene::CreateController(const physx::PxVec3& position, float radius, float height, PhysicsLayer queryLayer)
{
	physx::PxCapsuleControllerDesc desc;
	desc.position = { position.x, position.y, position.z };
	desc.stepOffset = 0.0f;
	desc.material = m_defaultMaterial->GetPxMaterial();
	// https://nvidia-omniverse.github.io/PhysX/physx/5.1.0/docs/CharacterControllers.html#character-volume
	desc.radius = radius;
	desc.height = height;

	physx::PxController* controller = m_controllerManager->createController(desc);

	physx::PxRigidDynamic* rigidbody = controller->getActor();
	PhysicsSetQueryLayer(rigidbody, queryLayer);

	return controller;
}

physx::PxShape* PhysicsScene::CreateShape(const physx::PxGeometry& geometry, bool isExclusive, bool isTrigger)
{
	return m_physics->createShape(
		geometry,
		*m_defaultMaterial->GetPxMaterial(),
		isExclusive,
		isTrigger ? physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eTRIGGER_SHAPE :
		physx::PxShapeFlag::eVISUALIZATION | physx::PxShapeFlag::eSCENE_QUERY_SHAPE | physx::PxShapeFlag::eSIMULATION_SHAPE
	);
}

physx::PxRigidStatic* PhysicsScene::CreateStatic(const physx::PxTransform& transform)
{
	physx::PxRigidStatic* actor = m_physics->createRigidStatic(transform);
	m_scene->addActor(*actor);
	return actor;
}

physx::PxRigidStatic* PhysicsScene::CreateStatic(const physx::PxTransform& transform, const physx::PxGeometry& geometry, PhysicsLayer queryLayer)
{
	physx::PxRigidStatic* actor = PxCreateStatic(*m_physics, transform, geometry, *m_defaultMaterial->GetPxMaterial());
	PhysicsSetQueryLayer(actor, queryLayer);
	m_scene->addActor(*actor);
	return actor;
}

physx::PxRigidDynamic* PhysicsScene::CreateDynamic(const physx::PxTransform& transform)
{
	physx::PxRigidDynamic* actor = m_physics->createRigidDynamic(transform);
	m_scene->addActor(*actor);
	return actor;
}

physx::PxRigidDynamic* PhysicsScene::CreateDynamic(
	const physx::PxTransform& transform,
	const physx::PxGeometry& geometry,
	PhysicsLayer              queryLayer,
	float                     density
)
{
	physx::PxRigidDynamic* actor = PxCreateDynamic(*m_physics, transform, geometry, *m_defaultMaterial->GetPxMaterial(), density);
	PhysicsSetQueryLayer(actor, queryLayer);
	m_scene->addActor(*actor);
	return actor;
}

physx::PxRaycastBuffer PhysicsScene::Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, const float distance, PhysicsLayer layer) const
{
	physx::PxQueryFilterData queryFilterData;
	queryFilterData.data = PhysicsFilterDataFromLayer(layer);

	physx::PxRaycastBuffer buffer;
	m_scene->raycast(origin, unitDir, distance, buffer, physx::PxHitFlag::eDEFAULT, queryFilterData);
	return buffer;
}

physx::PxSweepBuffer PhysicsScene::Sweep(
	const physx::PxGeometry& geometry,
	const physx::PxTransform& pose,
	const physx::PxVec3& unitDir,
	float                     distance,
	PhysicsLayer              layer
) const
{
	physx::PxQueryFilterData queryFilterData;
	queryFilterData.data = PhysicsFilterDataFromLayer(layer);

	physx::PxSweepBuffer buffer;
	m_scene->sweep(geometry, pose, unitDir, distance, buffer, physx::PxHitFlag::eDEFAULT, queryFilterData);
	return buffer;
}

#pragma endregion

//=============================================================================
#pragma region [ Physics System ]

PxDefaultAllocator   gDefaultAllocatorCallback;
PhysicsErrorCallback gErrorCallback;

PhysicsSystem::PhysicsSystem(EngineApplication& engine)
	: m_engine(engine)
	, m_scene(engine, *this)
{
}

bool PhysicsSystem::Setup(const PhysicsCreateInfo& createInfo)
{
	m_enable = createInfo.enable;

	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gErrorCallback);
	if (!m_foundation)
	{
		Fatal("PhysX foundation failed to create.");
		return false;
	}

	m_pvd = PxCreatePvd(*m_foundation);
	if (!m_pvd)
	{
		Fatal("Failed to create PhysX PVD.");
		return false;
	}
	
	m_pvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	if (!m_pvdTransport)
	{
		Fatal("Failed to create PhysX PVD transport.");
		return false;
	}
	
	if (!m_pvd->connect(*m_pvdTransport, PxPvdInstrumentationFlag::eALL))
		Warning("Failed to connect to PVD.");

	bool recordMemoryAllocations = true;
	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, PxTolerancesScale(), recordMemoryAllocations, m_pvd);
	if (!m_physics)
	{
		Fatal("Failed to create PhysX physics.");
		return false;
	}

	Print("PhysX Init " + std::to_string(PX_PHYSICS_VERSION_MAJOR) + "." + std::to_string(PX_PHYSICS_VERSION_MINOR) + "." + std::to_string(PX_PHYSICS_VERSION_BUGFIX));

	// initialize extensions (can be omitted, these are optional components)
	if (!PxInitExtensions(*m_physics, m_pvd))
	{
		Fatal("Unable to initialize PhysX");
		return false;
	}

	if (!m_scene.Setup(createInfo.scene)) return false;

	return true;
}

void PhysicsSystem::Shutdown()
{
	m_scene.Shutdown();
	PX_RELEASE(m_physics);
	PX_RELEASE(m_pvd);
	PX_RELEASE(m_pvdTransport);
	PX_RELEASE(m_foundation);
}

void PhysicsSystem::FixedUpdate()
{
	if (IsEnable())
		m_scene.FixedUpdate();
}

MaterialPtr ph::PhysicsSystem::CreateMaterial(float staticFriction, float dynamicFriction, float restitution)
{
	return std::make_shared<Material>(m_engine, staticFriction, dynamicFriction, restitution);
}

physx::PxConvexMesh* PhysicsSystem::CreateConvexMesh(physx::PxU32 count, const physx::PxVec3* vertices, physx::PxU16 vertexLimit)
{
	physx::PxConvexMeshDesc desc;
	desc.points.count = count;
	desc.points.stride = sizeof(physx::PxVec3);
	desc.points.data = vertices;
	desc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX | physx::PxConvexFlag::eDISABLE_MESH_VALIDATION | physx::PxConvexFlag::eFAST_INERTIA_COMPUTATION;
	desc.vertexLimit = vertexLimit;

	physx::PxDefaultMemoryOutputStream buffer;
	const physx::PxCookingParams cookingParams(m_physics->getTolerancesScale()); // TODO: save init
	if (!PxCookConvexMesh(cookingParams, desc, buffer))
	{
		Fatal("Failed to create convex PhysX mesh.");
		return nullptr;
	}

	physx::PxDefaultMemoryInputData input(buffer.getData(), buffer.getSize());
	return m_physics->createConvexMesh(input);
}

physx::PxTriangleMesh* PhysicsSystem::CreateTriangleMesh(physx::PxU32 count, const physx::PxVec3* vertices)
{
	physx::PxTriangleMeshDesc desc;
	desc.points.count = count;
	desc.points.stride = sizeof(physx::PxVec3);
	desc.points.data = vertices;
	desc.triangles.count = count / 3;
	desc.triangles.stride = 3 * sizeof(physx::PxU32);
	std::vector<physx::PxU32> trianglesData(count);
	for (physx::PxU32 i = 0; i < count; i++)
	{
		trianglesData[i] = i;
	}
	desc.triangles.data = trianglesData.data();

	physx::PxDefaultMemoryOutputStream buffer;
	const physx::PxCookingParams cookingParams(m_physics->getTolerancesScale()); // TODO: save init
	if (!PxCookTriangleMesh(cookingParams, desc, buffer))
	{
		Fatal("Failed to create triangle PhysX mesh.");
		return nullptr;
	}

	physx::PxDefaultMemoryInputData input(buffer.getData(), buffer.getSize());
	return m_physics->createTriangleMesh(input);
}

#pragma endregion


} // namespace ph