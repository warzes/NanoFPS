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
	m_material = m_engine.GetPhysicsSystem().GetPhysics()->createMaterial(staticFriction, dynamicFriction, restitution);

	Rav
}

Material::~Material()
{
	PX_RELEASE(m_material);
}

void Material::SetStaticFriction(float staticFriction)
{
	assert(m_material);
	m_material->setStaticFriction(staticFriction);
}

void Material::SetDynamicFriction(float dynamicFriction)
{
	assert(m_material);
	m_material->setDynamicFriction(dynamicFriction);
}

void Material::SetRestitution(float restitution)
{
	assert(m_material);
	m_material->setRestitution(restitution);
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

float Material::GetStaticFriction() const
{
	assert(m_material);
	return m_material->getStaticFriction();
}

float Material::GetDynamicFriction() const
{
	assert(m_material);
	return m_material->getDynamicFriction();
}

float Material::GetRestitution() const
{
	assert(m_material);
	return m_material->getRestitution();
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

	m_defaultMaterial = m_physics->createMaterial(createInfo.defaultMaterial.staticFriction, createInfo.defaultMaterial.dynamicFriction, createInfo.defaultMaterial.restitution);
	if (!m_defaultMaterial)
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
	PX_RELEASE(m_defaultMaterial);
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
	desc.material = m_defaultMaterial;
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
		*m_defaultMaterial,
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
	physx::PxRigidStatic* actor = PxCreateStatic(*m_physics, transform, geometry, *m_defaultMaterial);
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
	physx::PxRigidDynamic* actor = PxCreateDynamic(*m_physics, transform, geometry, *m_defaultMaterial, density);
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










physx::PxMaterial* PhysicsSystem::CreateMaterial(physx::PxReal staticFriction, physx::PxReal dynamicFriction, physx::PxReal restitution)
{
	return m_physics->createMaterial(staticFriction, dynamicFriction, restitution);
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