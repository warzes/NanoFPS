#pragma once

class EngineApplication;

namespace ph {

//=============================================================================
#pragma region [ Decl Class ]

class Material;
class Collider;
class PhysicsBody;
class RigidBody;
class StaticBody;
class CharacterController;
class PhysicsCallback;

struct BoxColliderCreateInfo;
struct SphereColliderCreateInfo;
struct CapsuleColliderCreateInfo;
struct MeshColliderCreateInfo;
struct ConvexMeshColliderCreateInfo;

using MaterialPtr = std::shared_ptr<Material>;
using ColliderPtr = std::shared_ptr<Collider>;
using RigidBodyPtr = std::shared_ptr<RigidBody>;
using StaticBodyPtr = std::shared_ptr<StaticBody>;
using CharacterControllerPtr = std::shared_ptr<CharacterController>;
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
#pragma region [ Physics Material ]

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

// Material that controls how two physical objects interact with each other. Materials of both objects are used during their interaction and their combined values are used.
class Material final
{
public:
	Material(EngineApplication& engine, float staticFriction = 1.0f, float dynamicFriction = 1.0f, float restitution = 1.0f);
	~Material();

	// Controls friction when two in-contact objects are not moving lateral to each other (for example how difficult it is to get an object moving from a static state while it is in contact with other object(s)).
	void SetStaticFriction(float value);
	float GetStaticFriction() const;

	// Controls friction when two in-contact objects are moving lateral to each other (for example how quickly does an object slow down when sliding along another object).
	void SetDynamicFriction(float value);
	float GetDynamicFriction() const;

	// Controls "bounciness" of an object during a collision. Value of 1 means the collision is elastic, and value of 0 means the value is inelastic. Must be in [0, 1] range.
	void SetRestitution(float value);
	float GetRestitution() const;

	void SetFrictionCombineMode(PhysicsCombineMode mode);
	void SetRestitutionCombineMode(PhysicsCombineMode mode);

	PhysicsCombineMode GetFrictionCombineMode() const;
	PhysicsCombineMode GetRestitutionCombineMode() const;

	physx::PxMaterial* GetPxMaterial() { return m_material; }

	bool IsValid() const { return m_material != nullptr; }
private:
	EngineApplication& m_engine;
	physx::PxMaterial* m_material{ nullptr };
};

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

	void AttachCollider(const BoxColliderCreateInfo& createInfo);
	void AttachCollider(const SphereColliderCreateInfo& createInfo);
	void AttachCollider(const CapsuleColliderCreateInfo& createInfo);
	void AttachCollider(const MeshColliderCreateInfo& createInfo);
	void AttachCollider(const ConvexMeshColliderCreateInfo& createInfo);

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
	PhysicsLayer                           m_queryLayer = PHYSICS_LAYER_0;
	std::unordered_set<PhysicsCallbackPtr> m_receivers;
	std::vector<ColliderPtr>               m_colliders;

};

#pragma endregion

//=============================================================================
#pragma region [ RigidBody ]

struct RigidBodyCreateInfo final
{
	uint32_t filterGroup = 0; // TODO: ?
	uint32_t filterMask = 0; // TODO: ?

	PhysicsLayer queryLayer = PHYSICS_LAYER_0;

	float density = 1.0f;

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

	glm::vec3 GetLinearVelocity() const;
	glm::vec3 GetAngularVelocity() const;

	void SetLinearVelocity(const glm::vec3& newvel, bool autowake);
	void SetAngularVelocity(const glm::vec3& newvel, bool autowake);

	void SetAngularDamping(float angDamp);

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
	uint32_t filterGroup = 0; // TODO: ?
	uint32_t filterMask = 0; // TODO: ?

	PhysicsLayer queryLayer = PHYSICS_LAYER_0;

	glm::vec3 worldPosition{ glm::vec3(0.0f) };
	glm::quat worldRotation{ glm::quat{1.0f, 0.0f, 0.0f, 0.0f} };
};

class StaticBody final : public PhysicsBody
{
public:
	StaticBody(EngineApplication& engine, const StaticBodyCreateInfo& createInfo);
};

#pragma endregion

//=============================================================================
#pragma region [ Character Controller ]

struct CharacterControllerCreateInfo final
{
	glm::vec3    position{ 0.0f };
	float        radius = 0.4f;
	float        height = 0.8f;
	PhysicsLayer queryLayer = PHYSICS_LAYER_1;
	MaterialPtr  material;
	void*        userData{ nullptr };
};

class CharacterController final
{
public:
	CharacterController(EngineApplication& engine, const CharacterControllerCreateInfo& createInfo);
	~CharacterController();

	void Move(const glm::vec3& disp, float minDist, float elapsedTime);

	[[nodiscard]] glm::vec3 GetPosition() const;
	[[nodiscard]] float GetSlopeLimit() const;

private:
	EngineApplication&   m_engine;
	physx::PxController* m_controller = nullptr;
	MaterialPtr          m_material;
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
#pragma region [ Collider ]

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
	std::pair<glm::vec3, glm::quat> GetRelativeTransform() const;

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
	std::vector<glm::vec3> vertices;
	std::vector<uint32_t> indices;
};

class MeshCollider final : public Collider
{
public:
	MeshCollider(EngineApplication& engine, PhysicsBody* owner, const MeshColliderCreateInfo& createInfo);
};

#pragma endregion

//=============================================================================
#pragma region [ Convex Mesh Collider ]

struct ConvexMeshColliderCreateInfo final
{
	MaterialPtr material{ nullptr };
	std::vector<glm::vec3> vertices;
	std::vector<uint32_t> indices;
};

class ConvexMeshCollider final : public Collider
{
public:
	ConvexMeshCollider(EngineApplication& engine, PhysicsBody* owner, const ConvexMeshColliderCreateInfo& createInfo);
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

	[[nodiscard]] physx::PxRaycastBuffer Raycast(const physx::PxVec3& origin, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] physx::PxSweepBuffer Sweep(const physx::PxGeometry& geometry, const physx::PxTransform& pose, const physx::PxVec3& unitDir, float distance, PhysicsLayer layer) const;

	[[nodiscard]] auto GetDefaultMaterial() { return m_defaultMaterial; }
	[[nodiscard]] auto GetControllerManager() { return m_controllerManager; }

	[[nodiscard]] auto GetPxScene() { return m_scene; }

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