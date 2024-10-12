#pragma once

class EngineApplication;

namespace ph {

//=============================================================================
#pragma region [ Decl Class ]

class Collider;
class PhysicsBody;
class RigidBody;
class PhysicsCallback;

using ColliderPtr = std::shared_ptr<Collider>;
using RigidBodyPtr = std::shared_ptr<RigidBody>;
using PhysicsCallbackPtr = std::shared_ptr<PhysicsCallback>;

#pragma endregion

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
// TODO: переделать
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

	physx::PxMaterial* GetPxMaterial() { return m_material; }
	const physx::PxMaterial* GetPxMaterial() const { return m_material; }

	bool IsValid() const { return m_material != nullptr; }
private:
	EngineApplication& m_engine;
	physx::PxMaterial* m_material{ nullptr };
};
using MaterialPtr = std::shared_ptr<Material>;

#pragma endregion

//=============================================================================
#pragma region [ Transformation ]

// TODO: удалить или переместить в коре

struct Transformation final
{
	operator glm::mat4() const
	{
		return glm::translate(glm::mat4(1), (glm::vec3)position) * glm::toMat4((glm::quat)rotation) * glm::scale(glm::mat4(1), (glm::vec3)scale);
	}

	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::quat rotation = glm::quat(1.0, 0.0, 0.0, 0.0);
	glm::vec3 scale = glm::vec3(1, 1, 1);
};

#pragma endregion

//=============================================================================
#pragma region [ Contact Pair Point ]

struct ContactPairPoint final
{
	ContactPairPoint(const physx::PxContactPairPoint& pxcpp) :
		position(pxcpp.position.x, pxcpp.position.y, pxcpp.position.z),
		normal(pxcpp.normal.x, pxcpp.normal.y, pxcpp.normal.z),
		impulse(pxcpp.impulse.x, pxcpp.impulse.y, pxcpp.impulse.z),
		separation(pxcpp.separation) {}

	glm::vec3 position, normal, impulse;
	float separation;
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Body ]

class PhysicsBody
{
	friend class Collider;
public:
	PhysicsBody(EngineApplication& engine);
	virtual ~PhysicsBody();

	template<typename T, typename ... A>
	auto EmplaceCollider(A&& ... args)
	{
		auto it = m_colliders.emplace_back(std::make_shared<T>(m_engine, this, args...));
		return it;
	}

	std::pair<glm::vec3, glm::quat> GetDynamicsWorldPose() const;
	void SetDynamicsWorldPose(const glm::vec3& worldpos, const glm::quat& worldrot) const;

	void SetGravityEnabled(bool state);
	bool GetGravityEnabled() const;

	void SetSleepNotificationsEnabled(bool state);
	bool GetSleepNotificationsEnabled() const;

	void SetSimulationEnabled(bool state);
	bool GetSimulationEnabled() const;

	void OnColliderEnter(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints);
	void OnColliderPersist(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints);
	void OnColliderExit(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints);

	void OnTriggerEnter(PhysicsBody* other);
	void OnTriggerExit(PhysicsBody* other);


	[[nodiscard]] auto GetPxRigidActor() const { return m_rigidActor; }
	[[nodiscard]] auto GetPxScene() const { return m_rigidActor->getScene(); }

protected:
	void construction();

	template<typename T>
	void lockWrite(const T& func)
	{
		auto scene = m_rigidActor->getScene();
		if (scene)
		{
			scene->lockWrite();
			func();
			scene->unlockWrite();
		}
		else
		{
			func();
		}
	}

	template<typename T>
	void lockRead(const T& func) const
	{
		auto scene = m_rigidActor->getScene();
		if (scene)
		{
			scene->lockRead();
			func();
			scene->unlockRead();
		}
		else
		{
			func();
		}
	}

	EngineApplication&                     m_engine;
	physx::PxRigidActor*                   m_rigidActor{ nullptr };
	physx::PxU32                           m_filterGroup = -1; // TODO: set max uint
	physx::PxU32                           m_filterMask = -1; // TODO: set max uint
	std::unordered_set<PhysicsCallbackPtr> m_receivers;
	std::vector<ColliderPtr>               m_colliders;
};

#pragma endregion

//=============================================================================
#pragma region [ RigidBody ]

struct RigidBodyCreateInfo final
{
	uint32_t filterGroup = 0;
	uint32_t filterMask = 0;

	glm::vec3 worldPosition{ glm::vec3(0.0f) };
	glm::quat worldRotation{ glm::quat{1.0f, 0.0f, 0.0f, 0.0f} };
};

class RigidBody final : public PhysicsBody
{
public:
	enum AxisLock
	{
		Linear_X = (1 << 0),
		Linear_Y = (1 << 1),
		Linear_Z = (1 << 2),
		Angular_X = (1 << 3),
		Angular_Y = (1 << 4),
		Angular_Z = (1 << 5)
	};

	RigidBody(EngineApplication& engine, const RigidBodyCreateInfo& createInfo);
	~RigidBody();

	glm::vec3 GetLinearVelocity() const;
	glm::vec3 GetAngularVelocity() const;

	void SetLinearVelocity(const glm::vec3& newvel, bool autowake);
	void SetAngularVelocity(const glm::vec3& newvel, bool autowake);

	void SetKinematicTarget(const glm::vec3& targetPos, const glm::quat& targetRot);
	std::pair<glm::vec3, glm::quat> GetKinematicTarget() const;

	void Wake();
	void Sleep();
	bool IsSleeping() const;

	void SetAxisLock(uint16_t LockFlags);
	uint16_t GetAxisLock() const;

	void SetMass(float mass);
	float GetMass() const;
	float GetMassInverse() const;

	void AddForce(const glm::vec3& force);
	void AddTorque(const glm::vec3& torque);

	void ClearAllForces();
	void ClearAllTorques();
};

#pragma endregion

//=============================================================================
#pragma region [ StaticBody ]

struct StaticBodyCreateInfo final
{
	uint32_t filterGroup = 0;
	uint32_t filterMask = 0;
};

class StaticBody final : public PhysicsBody
{
public:
	StaticBody(EngineApplication& engine, const StaticBodyCreateInfo& createInfo);
	~StaticBody();
	
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Callback ]

class PhysicsCallback final
{
public:
	std::function<void(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderEnter;
	std::function<void(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderExit;
	// Called by a PhysicsBody when it has collided with another and the collision has persisted. 
	std::function<void(PhysicsBody* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderPersist;

	std::function<void(PhysicsBody*)> OnTriggerEnter;
	std::function<void(PhysicsBody*)> OnTriggerExit;
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Collider ]

enum class CollisionType 
{ 
	Trigger,
	Collider
};

class Collider
{
	friend class PhysicsBody;
public:
	Collider(EngineApplication& engine, MaterialPtr material);
	virtual ~Collider();

	virtual void DebugDraw(Color debugColor, const Transformation&) const {} // TODO: в будущем сделать

	void SetType(CollisionType type);
	CollisionType GetType() const;

	// 	Set whether the collider participates in scene queries (raycasts, overlaps, etc)
	void SetQueryable(bool state);
	bool GetQueryable() const;

	void SetRelativeTransform(const glm::vec3& position, const glm::quat& rotation);

	Transformation GetRelativeTransform() const;

	bool IsValid() const { return m_collider != nullptr; }

	void UpdateFilterData(PhysicsBody* owner);

protected:
	EngineApplication& m_engine;
	physx::PxShape*    m_collider{ nullptr };
	MaterialPtr        m_material{ nullptr };
};

#pragma endregion

//=============================================================================
#pragma region [ Box Collider ]

struct BoxColliderCreateInfo final
{
	glm::vec3   extent{ glm::vec3(1.0f) };
	MaterialPtr material{ nullptr };
	glm::vec3   position{ glm::vec3(0.0f) };
	glm::quat   rotation{ glm::quat(1.0, 0.0, 0.0, 0.0) };
};

class BoxCollider final : public Collider
{
public:
	BoxCollider(EngineApplication& engine, PhysicsBody* owner, const BoxColliderCreateInfo& createInfo);
	~BoxCollider();

private:
	glm::vec3 m_extent;
};

#pragma endregion

//=============================================================================
#pragma region [ Sphere Collider ]

struct SphereColliderCreateInfo final
{
	float       radius{ 1.0f };
	MaterialPtr material{ nullptr };
	glm::vec3   position{ glm::vec3(0.0f) };
	glm::quat   rotation{ glm::quat(1.0, 0.0, 0.0, 0.0) };
};

class SphereCollider final : public Collider
{
public:
	SphereCollider(EngineApplication& engine, PhysicsBody* owner, const SphereColliderCreateInfo& createInfo);
	~SphereCollider();

	float GetRadius() const;
};

#pragma endregion

//=============================================================================
#pragma region [ Capsule Collider ]

struct CapsuleColliderCreateInfo final
{
	float       radius{ 1.0f };
	float       halfHeight{ 1.0f };
	MaterialPtr material{ nullptr };
	glm::vec3   position{ glm::vec3(0.0f) };
	glm::quat   rotation{ glm::quat(1.0, 0.0, 0.0, 0.0) };
};

class CapsuleCollider final : public Collider
{
public:
	CapsuleCollider(EngineApplication& engine, PhysicsBody* owner, const CapsuleColliderCreateInfo& createInfo);
	~CapsuleCollider();

private:
	float m_radius;
	float m_halfHeight;
};

#pragma endregion

//=============================================================================
#pragma region [ Mesh Collider ]

struct MeshColliderCreateInfo final
{
	MaterialPtr material{ nullptr };
	glm::vec3   position{ glm::vec3(0.0f) };
	glm::quat   rotation{ glm::quat(1.0, 0.0, 0.0, 0.0) };
};

class MeshCollider final : public Collider
{
public:
	MeshCollider(EngineApplication& engine, PhysicsBody* owner, const MeshColliderCreateInfo& createInfo);
	~MeshCollider();
};

#pragma endregion

//=============================================================================
#pragma region [ Convex Mesh Collider ]

struct ConvexMeshColliderCreateInfo final
{
	MaterialPtr material{ nullptr };
	glm::vec3   position{ glm::vec3(0.0f) };
	glm::quat   rotation{ glm::quat(1.0, 0.0, 0.0, 0.0) };
};

class ConvexMeshCollider final : public Collider
{
public:
	ConvexMeshCollider(EngineApplication& engine, PhysicsBody* owner, const ConvexMeshColliderCreateInfo& createInfo);
	~ConvexMeshCollider();
};

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

	[[nodiscard]] auto GetDefaultMaterial() { return m_defaultMaterial; }

	[[nodiscard]] auto GetPxScene() { return m_scene; }







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
	MaterialPtr                    m_defaultMaterial{ nullptr };
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

	[[nodiscard]] MaterialPtr CreateMaterial(float staticFriction, float dynamicFriction, float restitution);






	[[nodiscard]] physx::PxConvexMesh* CreateConvexMesh(
		physx::PxU32         count,
		const physx::PxVec3* vertices,
		// The number of vertices and faces of a convex mesh in PhysX is limited to 255.
		physx::PxU16         vertexLimit = 255
	);

	[[nodiscard]] physx::PxTriangleMesh* CreateTriangleMesh(physx::PxU32 count, const physx::PxVec3* vertices);

	[[nodiscard]] auto GetPxPhysics() { return m_physics; }

private:
	EngineApplication&     m_engine;
	physx::PxFoundation*   m_foundation{ nullptr };
	physx::PxPvdTransport* m_pvdTransport{ nullptr };
	physx::PxPvd*          m_pvd{ nullptr };
	physx::PxPhysics*      m_physics{ nullptr };
	ph::PhysicsScene       m_scene; // TODO: в будущем создавать из physics system
	bool                   m_enable{ false };
};

#pragma endregion

} // namespace ph