#include "Base.h"
#include "Core.h"
#include "Physics.h"

#pragma region Physics Error Callback

void Info(const char* error, const char* message) noexcept
{
	Print("PhysX " + std::string(error) + ": " + message);
}

void Warning(const char* error, const char* message) noexcept
{
	Warning("PhysX " + std::string(error) + ": " + message);
}

void Error(const char* error, const char* message) noexcept
{
	Error("PhysX " + std::string(error) + ": " + message);
}

void PhysicsErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* message, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	const char* error = nullptr;
	void (*loggingCallback)(const char*, const char*) = nullptr;
	switch (code)
	{
	case physx::PxErrorCode::eNO_ERROR:
		error = "No Error";
		loggingCallback = Info;
		break;
	case physx::PxErrorCode::eDEBUG_INFO:
		error = "Debug Info";
		loggingCallback = Info;
		break;
	case physx::PxErrorCode::eDEBUG_WARNING:
		error = "Debug Warning";
		loggingCallback = Warning;
		break;
	case physx::PxErrorCode::eINVALID_PARAMETER:
		error = "Invalid Parameter";
		loggingCallback = Error;
		break;
	case physx::PxErrorCode::eINVALID_OPERATION:
		error = "Invalid Operation";
		loggingCallback = Error;
		break;
	case physx::PxErrorCode::eOUT_OF_MEMORY:
		error = "Out of Memory";
		loggingCallback = Error;
		break;
	case physx::PxErrorCode::eINTERNAL_ERROR:
		error = "Internal Error";
		loggingCallback = Error;
		break;
	case physx::PxErrorCode::eABORT:
		error = "Abort";
		loggingCallback = Error;
		break;
	case physx::PxErrorCode::ePERF_WARNING:
		error = "Performance Warning";
		loggingCallback = Warning;
		break;
	case physx::PxErrorCode::eMASK_ALL:
		error = "Unknown Error";
		loggingCallback = Error;
		break;
	}
	loggingCallback(error, message);
}

#pragma endregion

#pragma region Physics Layers

void PhysicsSetQueryLayer(physx::PxRigidActor* actor, PhysicsLayer layer)
{
	const physx::PxFilterData filterData = PhysicsFilterDataFromLayer(layer);
	PhysicsForEachActorShape(actor, [&filterData](physx::PxShape* shape) { shape->setQueryFilterData(filterData); });
}

#pragma endregion

#pragma region Physics Scene

PhysicsScene::PhysicsScene(PhysicsSystem* physicsSystem)
	: m_physics(physicsSystem->m_physics)
{
	m_defaultCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	if(!m_defaultCpuDispatcher)
		Fatal("Failed to create default PhysX CPU dispatcher.");

	m_defaultMaterial = m_physics->createMaterial(0.8f, 0.8f, 0.25f);
	if(!m_defaultMaterial)
		Fatal("Failed to create default PhysX material.");

	physx::PxSceneDesc sceneDesc(m_physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = m_defaultCpuDispatcher;
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	m_scene = m_physics->createScene(sceneDesc);
	if (!m_scene)
		Fatal("Failed to create PhysX scene.");

	if (physx::PxPvdSceneClient* pvdClient = m_scene->getScenePvdClient())
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	m_controllerManager = PxCreateControllerManager(*m_scene);
	if(!m_controllerManager)
		Fatal("Failed to create PhysX controller manager.");
}

PhysicsScene::~PhysicsScene()
{
	PX_RELEASE(m_controllerManager)
	PX_RELEASE(m_scene)
	PX_RELEASE(m_defaultMaterial)
	PX_RELEASE(m_defaultCpuDispatcher)
}

bool PhysicsScene::Update(const float deltaTime, const float timeScale)
{
	m_timeSinceLastTick += deltaTime * timeScale;

	const float scaledTimestep = m_fixedTimestep * timeScale;
	if (m_timeSinceLastTick < scaledTimestep)
	{
		return false;
	}

	m_scene->simulate(scaledTimestep);
	m_scene->fetchResults(true);
	m_timeSinceLastTick -= scaledTimestep;
	return true;
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

#pragma region PhysicsSystem

static physx::PxDefaultAllocator s_Allocator;
static PhysicsErrorCallback      s_ErrorCallback;

PhysicsSystem::PhysicsSystem()
{
	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, s_Allocator, s_ErrorCallback);
	if(!m_foundation)
		Fatal("Failed to create PhysX foundation.");

	m_pvdTransport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	if(!m_pvdTransport)
		Fatal("Failed to create PhysX PVD transport.");

	m_pvd = PxCreatePvd(*m_foundation);
	if(!m_pvd)
		Fatal("Failed to create PhysX PVD.");

	if (!m_pvd->connect(*m_pvdTransport, physx::PxPvdInstrumentationFlag::eALL))
		Warning("Failed to connect to PVD.");

	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, physx::PxTolerancesScale(), true, m_pvd);
	if(!m_physics)
		Fatal("Failed to create PhysX physics.");

	Print("PhysX " + std::to_string(PX_PHYSICS_VERSION_MAJOR) + "." + std::to_string(PX_PHYSICS_VERSION_MINOR) + "." + std::to_string(PX_PHYSICS_VERSION_BUGFIX));
}

PhysicsSystem::~PhysicsSystem()
{
	PX_RELEASE(m_physics)
	PX_RELEASE(m_pvd)
	PX_RELEASE(m_pvdTransport)
	PX_RELEASE(m_foundation)
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