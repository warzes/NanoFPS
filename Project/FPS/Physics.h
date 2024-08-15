#pragma once

#pragma region Physics Utilities

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

#pragma region Physics Error Callback

class PhysicsErrorCallback : public physx::PxErrorCallback
{
public:
	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

#pragma endregion

#pragma region Physics Layers

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

#pragma region Physics Scene

class PhysicsSystem;

class PhysicsScene final
{
public:
	explicit PhysicsScene(PhysicsSystem* physicsSystem);
	PhysicsScene(const PhysicsScene&) = delete;
	PhysicsScene(PhysicsScene&&) = delete;
	~PhysicsScene();

	PhysicsScene& operator=(const PhysicsScene&) = delete;
	PhysicsScene& operator=(PhysicsScene&&) = delete;

	bool Update(float deltaTime, float timeScale);

	void SetSimulationEventCallback(physx::PxSimulationEventCallback* callback);

	[[nodiscard]] float GetFixedTimestep() const { return m_fixedTimestep; }

	[[nodiscard]] float GetFixedUpdateTimeError() const { return m_timeSinceLastTick; }

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
	physx::PxPhysics* m_physics = nullptr;

	physx::PxDefaultCpuDispatcher* m_defaultCpuDispatcher = nullptr;
	physx::PxMaterial*             m_defaultMaterial = nullptr;
	physx::PxScene*                m_scene = nullptr;
	physx::PxControllerManager*    m_controllerManager = nullptr;

	float m_fixedTimestep = 0.02f;
	float m_timeSinceLastTick = 0.0f;
};

#pragma endregion

#pragma region Physics System

class PhysicsSystem final
{
public:
	PhysicsSystem();
	PhysicsSystem(const PhysicsSystem&) = delete;
	PhysicsSystem(PhysicsSystem&&) = delete;
	~PhysicsSystem();

	PhysicsSystem& operator=(const PhysicsSystem&) = delete;
	PhysicsSystem& operator=(PhysicsSystem&&) = delete;

	physx::PxMaterial* CreateMaterial(physx::PxReal staticFriction, physx::PxReal dynamicFriction, physx::PxReal restitution);

	physx::PxConvexMesh* CreateConvexMesh(
		physx::PxU32         count,
		const physx::PxVec3* vertices,
		// The number of vertices and faces of a convex mesh in PhysX is limited to 255.
		physx::PxU16         vertexLimit = 255
	);

	physx::PxTriangleMesh* CreateTriangleMesh(physx::PxU32 count, const physx::PxVec3* vertices);

private:
	friend class PhysicsScene;

	physx::PxFoundation*   m_foundation = nullptr;
	physx::PxPvdTransport* m_pvdTransport = nullptr;
	physx::PxPvd*          m_pvd = nullptr;
	physx::PxPhysics*      m_physics = nullptr;
};

#pragma endregion

#pragma region PhysicsSimulationEventCallback

class PhysicsSimulationEventCallback final : public physx::PxSimulationEventCallback
{
public:
	void onConstraintBreak([[maybe_unused]] physx::PxConstraintInfo* constraints, [[maybe_unused]] physx::PxU32 count) final {}
	void onWake([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count) final {}
	void onSleep([[maybe_unused]] physx::PxActor** actors, [[maybe_unused]] physx::PxU32 count) final {}
	void onContact([[maybe_unused]] const physx::PxContactPairHeader& pairHeader, [[maybe_unused]] const physx::PxContactPair* pairs, [[maybe_unused]] physx::PxU32 nbPairs) final {}
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) final;
	void onAdvance([[maybe_unused]] const physx::PxRigidBody* const* bodyBuffer, [[maybe_unused]] const physx::PxTransform* poseBuffer, [[maybe_unused]] physx::PxU32 count) final {}
};

#pragma endregion