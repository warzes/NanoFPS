#pragma once

class EngineApplication;

namespace ph {

//=============================================================================
#pragma region [ Core Utilities ]

template<class Func>
inline std::vector<physx::PxShape*> PhysicsForEachActorShape(physx::PxRigidActor* actor, Func&& func)
{
	std::vector<physx::PxShape*> shapes(actor->getNbShapes());
	actor->getShapes(shapes.data(), static_cast<physx::PxU32>(shapes.size()));
	for (physx::PxShape* shape : shapes)
	{
		func(shape);
	}
	return shapes;
}

inline void PhysicsSetActorMaterial(physx::PxRigidActor* actor, physx::PxMaterial* material)
{
	PhysicsForEachActorShape(actor, [material](physx::PxShape* shape) { shape->setMaterials(&material, 1); });
}

#pragma endregion

//=============================================================================
#pragma region [ Error Callback ]

class PhysicsErrorCallback final : public physx::PxErrorCallback
{
public:
	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) final;
};

#pragma endregion

//=============================================================================
#pragma region [ Simulation Event Callback ]

class PhysicsSimulationEventCallback final : public physx::PxSimulationEventCallback
{
public:
	void onConstraintBreak(physx::PxConstraintInfo* /*constraints*/, physx::PxU32 /*count*/) final {}
	void onWake(physx::PxActor** /*actors*/, physx::PxU32 /*count*/) final {}
	void onSleep(physx::PxActor** /*actors*/, physx::PxU32 /*count*/) final {}
	void onContact(const physx::PxContactPairHeader& /*pairHeader*/, const physx::PxContactPair* /*pairs*/, physx::PxU32 /*nbPairs*/) final {}
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) final;
	void onAdvance(const physx::PxRigidBody* const* /*bodyBuffer*/, const physx::PxTransform* /*poseBuffer*/, const physx::PxU32 /*count*/) final {}
};

#pragma endregion

//=============================================================================
#pragma region [ Layers ]

enum PhysicsLayer : physx::PxU32
{
	PHYSICS_LAYER_0 = (1u << 0), // world default
	PHYSICS_LAYER_1 = (1u << 1), // character controller default
	PHYSICS_LAYER_2 = (1u << 2),
	PHYSICS_LAYER_3 = (1u << 3),
	PHYSICS_LAYER_4 = (1u << 4),
	PHYSICS_LAYER_5 = (1u << 5),
	PHYSICS_LAYER_6 = (1u << 6),
	PHYSICS_LAYER_7 = (1u << 7),
	PHYSICS_LAYER_8 = (1u << 8),
	PHYSICS_LAYER_9 = (1u << 9),
	PHYSICS_LAYER_10 = (1u << 10),
	PHYSICS_LAYER_11 = (1u << 11),
	PHYSICS_LAYER_12 = (1u << 12),
	PHYSICS_LAYER_13 = (1u << 13),
	PHYSICS_LAYER_14 = (1u << 14),
	PHYSICS_LAYER_15 = (1u << 15),
	PHYSICS_LAYER_16 = (1u << 16),
	PHYSICS_LAYER_17 = (1u << 17),
	PHYSICS_LAYER_18 = (1u << 18),
	PHYSICS_LAYER_19 = (1u << 19),
	PHYSICS_LAYER_20 = (1u << 20),
	PHYSICS_LAYER_21 = (1u << 21),
	PHYSICS_LAYER_22 = (1u << 22),
	PHYSICS_LAYER_23 = (1u << 23),
	PHYSICS_LAYER_24 = (1u << 24),
	PHYSICS_LAYER_25 = (1u << 25),
	PHYSICS_LAYER_26 = (1u << 26),
	PHYSICS_LAYER_27 = (1u << 27),
	PHYSICS_LAYER_28 = (1u << 28),
	PHYSICS_LAYER_29 = (1u << 29),
	PHYSICS_LAYER_30 = (1u << 30),
	PHYSICS_LAYER_31 = (1u << 31)
};

inline physx::PxFilterData PhysicsFilterDataFromLayer(PhysicsLayer layer)
{
	return { layer, 0, 0, 0 };
}

void PhysicsSetQueryLayer(physx::PxRigidActor* actor, PhysicsLayer layer);

#pragma endregion

//=============================================================================
#pragma region [ Physics Actor ]

class PhysicsActor
{
public:
	PhysicsActor() = default;
	virtual ~PhysicsActor() = default;

	virtual void OnTriggerEnter([[maybe_unused]] PhysicsActor* other) {}
	virtual void OnTriggerExit([[maybe_unused]] PhysicsActor* other) {}
};

#pragma endregion

//=============================================================================
#pragma region Physics Material

enum class PhysicsMaterialFlag 
{
	DisableFriction = 1 << 0,
	DisableStrongFriction = 1 << 1
};

enum class PhysicsCombineMode
{
	Average = 0,
	Min = 1,
	Multiply = 2,
	Max = 3,
	NValues = 4,
	Pad32 = 0x7fffffff
};

class Material final
{
public:
	Material(EngineApplication& engine, float staticFriction = 1.0f, float dynamicFriction = 1.0f, float restitution = 1.0f);
	~Material();

	void SetStaticFriction(float staticFriction);
	void SetDynamicFriction(float dynamicFriction);
	void SetRestitution(float restitution);
	void SetFrictionCombineMode(PhysicsCombineMode mode);
	void SetRestitutionCombineMode(PhysicsCombineMode mode);

	float GetStaticFriction() const;
	float GetDynamicFriction() const;
	float GetRestitution() const;
	PhysicsCombineMode GetFrictionCombineMode() const;
	PhysicsCombineMode GetRestitutionCombineMode() const;

	const physx::PxMaterial* GetPxMaterial() const { return m_material; }
private:
	EngineApplication& m_engine;
	physx::PxMaterial* m_material{ nullptr };
};
using MaterialPtr = std::shared_ptr<Material>;

#pragma endregion


//=============================================================================
#pragma region [ Physics Scene ]

class PhysicsSystem;

struct PhysicsSceneCreateInfo final
{
	uint8_t   cpuDispatcherNum = 2;
	glm::vec3 gravity{ 0.0f, -9.81f, 0.0f };

	struct DefaultMaterial
	{
		float staticFriction = 0.8f;
		float dynamicFriction = 0.8f;
		float restitution = 0.25f;
	} defaultMaterial;
};

class PhysicsScene final
{
	friend EngineApplication;
public:
	PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine);

	[[nodiscard]] bool Setup(const PhysicsSceneCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();

	void SetSimulationEventCallback(physx::PxSimulationEventCallback* callback);

	physx::PxScene* GetPxScene() { return m_scene; }


	physx::PxController* CreateController(const physx::PxVec3& position, float radius, float height, PhysicsLayer queryLayer = PHYSICS_LAYER_1);

	physx::PxShape* CreateShape(const physx::PxGeometry& geometry, bool isExclusive, bool isTrigger);

	physx::PxRigidStatic* CreateStatic(const physx::PxTransform& transform);

	physx::PxRigidStatic* CreateStatic(
		const physx::PxTransform& transform,
		const physx::PxGeometry& geometry,
		PhysicsLayer              queryLayer = PHYSICS_LAYER_0
	);

	physx::PxRigidDynamic* CreateDynamic(const physx::PxTransform& transform);

	physx::PxRigidDynamic* CreateDynamic(
		const physx::PxTransform& transform,
		const physx::PxGeometry& geometry,
		PhysicsLayer              queryLayer = PHYSICS_LAYER_0,
		float                     density = 1.0f
	);

	[[nodiscard]] physx::PxRaycastBuffer Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] physx::PxSweepBuffer Sweep(
		const physx::PxGeometry& geometry,
		const physx::PxTransform& pose,
		const physx::PxVec3& unitDir,
		float                     distance,
		PhysicsLayer              layer
	) const;



private:
	EngineApplication&             m_engine;
	PhysicsSystem&                 m_system;
	PhysicsSimulationEventCallback m_physicsCallback;
	physx::PxPhysics*              m_physics{ nullptr };
	physx::PxDefaultCpuDispatcher* m_cpuDispatcher{ nullptr };
	physx::PxScene*                m_scene{ nullptr };
	physx::PxMaterial*             m_defaultMaterial{ nullptr };
	physx::PxControllerManager*    m_controllerManager{ nullptr };
};

#pragma endregion

//=============================================================================
#pragma region [ Physics System ]

struct PhysicsCreateInfo final
{
	PhysicsSceneCreateInfo scene;
	bool enable = false;
};

class PhysicsSystem final
{
	friend EngineApplication;
	friend class PhysicsScene;
public:
	PhysicsSystem(EngineApplication& engine);

	[[nodiscard]] bool Setup(const PhysicsCreateInfo& createInfo);
	void Shutdown();

	[[nodiscard]] bool IsEnable() const { return m_enable; }
	[[nodiscard]] PhysicsScene& GetScene() { return m_scene; }

	void FixedUpdate();








	[[nodiscard]] physx::PxMaterial* CreateMaterial(physx::PxReal staticFriction, physx::PxReal dynamicFriction, physx::PxReal restitution);

	[[nodiscard]] physx::PxConvexMesh* CreateConvexMesh(
		physx::PxU32         count,
		const physx::PxVec3* vertices,
		// The number of vertices and faces of a convex mesh in PhysX is limited to 255.
		physx::PxU16         vertexLimit = 255
	);

	[[nodiscard]] physx::PxTriangleMesh* CreateTriangleMesh(physx::PxU32 count, const physx::PxVec3* vertices);

	[[nodiscard]] physx::PxPhysics* GetPhysics() { return m_physics; }

private:
	EngineApplication&     m_engine;
	physx::PxFoundation*   m_foundation{ nullptr };
	physx::PxPvdTransport* m_pvdTransport{ nullptr };
	physx::PxPvd*          m_pvd{ nullptr };
	physx::PxPhysics*      m_physics{ nullptr };

	ph::PhysicsScene m_scene; // TODO: в будущем создавать из physics system

	bool                   m_enable{ false };
};

#pragma endregion

} // namespace ph