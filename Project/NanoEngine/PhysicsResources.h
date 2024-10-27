#pragma once

#include "PhysicsCore.h"

namespace ph {

//=============================================================================
#pragma region [ Physics Material ]

struct MaterialCreateInfo final
{
	float staticFriction = 0.8f;
	float dynamicFriction = 0.8f;
	float restitution = 0.25f;
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
	Material(EngineApplication& engine, const MaterialCreateInfo& createInfo);
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
#pragma region [ Collider ]

enum class CollisionType : uint8_t
{
	Trigger,
	Collider
};

class Collider
{
public:
	Collider(EngineApplication& engine, MaterialPtr material);
	virtual ~Collider();

	virtual void DebugDraw() const {} // TODO: в будущем сделать

	void SetRelativeTransform(const glm::vec3& position, const glm::quat& rotation);
	std::pair<glm::vec3, glm::quat> GetRelativeTransform() const;

	void SetType(CollisionType type);
	CollisionType GetType() const;

	// 	Set whether the collider participates in scene queries (raycasts, overlaps, etc)
	void SetQueryable(bool state);
	bool GetQueryable() const;
	
	bool IsValid() const { return m_collider != nullptr; }

	physx::PxShape* GetPxShape() { return m_collider; }
	
protected:
	void updateFilterData(BaseActor* owner);
	MaterialPtr getMaterial() const;

	EngineApplication& m_engine;
	physx::PxShape*    m_collider{ nullptr };
private:
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
	BoxCollider(EngineApplication& engine, BaseActor* owner, const BoxColliderCreateInfo& createInfo);

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
	SphereCollider(EngineApplication& engine, BaseActor* owner, const SphereColliderCreateInfo& createInfo);

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
	CapsuleCollider(EngineApplication& engine, BaseActor* owner, const CapsuleColliderCreateInfo& createInfo);

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
	MeshCollider(EngineApplication& engine, BaseActor* owner, const MeshColliderCreateInfo& createInfo);
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
	ConvexMeshCollider(EngineApplication& engine, BaseActor* owner, const ConvexMeshColliderCreateInfo& createInfo);
};

#pragma endregion

//=============================================================================
#pragma region [ Physics Callback ]

struct ContactPairPoint final
{
	ContactPairPoint(const physx::PxContactPairPoint& pxcpp)
		: position(pxcpp.position.x, pxcpp.position.y, pxcpp.position.z)
		, normal(pxcpp.normal.x, pxcpp.normal.y, pxcpp.normal.z)
		, impulse(pxcpp.impulse.x, pxcpp.impulse.y, pxcpp.impulse.z)
		, separation(pxcpp.separation)
	{
	}

	glm::vec3 position, normal, impulse;
	float separation;
};

class PhysicsCallback final
{
public:
	std::function<void(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderEnter;
	std::function<void(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderExit;
	// Called by a BaseActor when it has collided with another and the collision has persisted. 
	std::function<void(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints)> OnColliderPersist;

	std::function<void(BaseActor*)> OnTriggerEnter;
	std::function<void(BaseActor*)> OnTriggerExit;
};

#pragma endregion

//=============================================================================
#pragma region [ Physics BaseActor ]

class BaseActor
{
	friend Collider;
public:
	BaseActor(EngineApplication& engine);
	~BaseActor();

	void AttachCollider(const BoxColliderCreateInfo& createInfo);
	void AttachCollider(const SphereColliderCreateInfo& createInfo);
	void AttachCollider(const CapsuleColliderCreateInfo& createInfo);
	void AttachCollider(const MeshColliderCreateInfo& createInfo);
	void AttachCollider(const ConvexMeshColliderCreateInfo& createInfo);

	void AttachCollider(ColliderPtr collider);

	void SetSimulationEnabled(bool state);
	bool GetSimulationEnabled() const;

	// wantsContactData controls if the simulation calculates and extracts contact point information on collisions. Set to true to get contact point data. If set to false, OnCollider functions will have a dangling contactPoints pointer and numContactPoints will be 0.
	void SetWantsContactData(bool state) { m_wantsContactData = state; }
	bool GetWantsContactData() const { return m_wantsContactData; }

	void AddReceiver(PhysicsCallbackPtr obj);

	void OnColliderEnter(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints);
	void OnColliderPersist(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints);
	void OnColliderExit(BaseActor* other, const ContactPairPoint* contactPoints, size_t numContactPoints);
	void OnTriggerEnter(BaseActor* other);
	void OnTriggerExit(BaseActor* other);

	[[nodiscard]] auto GetPxActor() const { return m_actor; }
	[[nodiscard]] auto GetPxScene() const { return m_actor->getScene(); }

protected:
	void physicsSetQueryLayer(ColliderPtr collider);
	EngineApplication&                     m_engine;
	physx::PxRigidActor*                   m_actor{ nullptr };
	physx::PxU32                           m_filterGroup = -1; // TODO: set max uint
	physx::PxU32                           m_filterMask = -1; // TODO: set max uint
	PhysicsLayer                           m_queryLayer = PHYSICS_LAYER_0;
	std::vector<ColliderPtr>               m_colliders;
	std::unordered_set<PhysicsCallbackPtr> m_receivers;
	bool                                   m_wantsContactData = true;
};

#pragma endregion

//=============================================================================
#pragma region [ StaticBody ]

// примечание: статические объекты не рекомендуется двигать, поэтому Transform у него задается при создании и нет методов движения

struct StaticActorCreateInfo final
{
	uint32_t filterGroup = 0; // TODO: ?
	uint32_t filterMask = 0; // TODO: ?
	PhysicsLayer queryLayer = PHYSICS_LAYER_0;

	glm::vec3 worldPosition{ glm::vec3(0.0f) };
	glm::quat worldRotation{ glm::quat{1.0f, 0.0f, 0.0f, 0.0f} };
};

class StaticBody final : public BaseActor
{
public:
	StaticBody(EngineApplication& engine, const StaticActorCreateInfo& createInfo);
};

#pragma endregion

//=============================================================================
#pragma region [ RigidBody ]

// Flags that control options of a Rigidbody object. 
enum class RigidbodyFlag : uint8_t
{
	// No options.
	None = 0x00,
	// Automatically calculate center of mass transform and inertia tensors from child shapes (colliders).
	AutoTensors = 0x01,
	// Calculate mass distribution from child shapes (colliders). Only relevant when auto-tensors is on.
	AutoMass = 0x02,
	// Enables continous collision detection. This can prevent fast moving bodies from tunneling through each other. This must also be enabled globally in Physics otherwise the flag will be ignored.
	//CCD = 0x04
};

// Type of force or torque that can be applied to a rigidbody.
enum class ForceMode : uint8_t
{
	Force,       // Value applied is a force.
	Impulse,     // Value applied is an impulse (a direct change in its linear or angular momentum).
	Velocity,    // Value applied is velocity.
	Acceleration // Value applied is accelearation.
};

// Type of force that can be applied to a rigidbody at an arbitrary point.
enum PointForceMode : uint8_t
{
	Force,   // Value applied is a force.
	Impulse, // Value applied is an impulse (a direct change in its linear or angular momentum).
};

struct RigidBodyCreateInfo final
{
	uint32_t     filterGroup = 0; // TODO: ?
	uint32_t     filterMask = 0; // TODO: ?
	PhysicsLayer queryLayer = PHYSICS_LAYER_0;

	float        density = 1.0f;
};

class RigidBody final : public BaseActor
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

	void SetFlags(RigidbodyFlag flags);

	void SetWorldPose(const glm::vec3& worldPosition, const glm::quat& worldRotation);
	void SetPosition(const glm::vec3& position);
	void Rotate(const glm::quat& rotation);
	std::pair<glm::vec3, glm::quat> GetWorldPose() const;
	glm::vec3 GetPosition() const;
	glm::quat GetRotation() const;

	void SetGravityEnabled(bool state);
	bool GetGravityEnabled() const;

	void SetSleepNotificationsEnabled(bool state);
	bool GetSleepNotificationsEnabled() const;

	void SetLinearVelocity(const glm::vec3& newvel, bool autowake = true);
	glm::vec3 GetLinearVelocity() const;

	void SetAngularVelocity(const glm::vec3& newvel, bool autowake = true);
	glm::vec3 GetAngularVelocity() const;

	void SetLinearDamping(float drag);
	float GetLinearDamping() const;

	void SetAngularDamping(float angDamp);
	float GetAngularDrag() const;

	void SetInertiaTensor(const glm::vec3& tensor);
	glm::vec3 GetInertiaTensor() const;

	void SetMaxAngularVelocity(float maxVelocity);
	float GetMaxAngularVelocity() const;

	void SetIsKinematic(bool kinematic);
	bool GetIsKinematic() const;

	void SetKinematicTarget(const glm::vec3& targetPos, const glm::quat& targetRot);
	std::pair<glm::vec3, glm::quat> GetKinematicTarget() const;

	void Wake();
	void Sleep();
	bool IsSleeping() const;
	void SetSleepThreshold(float threshold);
	float GetSleepThreshold() const;

	void SetAxisLock(uint16_t LockFlags);
	uint16_t GetAxisLock() const;

	void SetMass(float mass);
	float GetMass() const;
	float GetMassInverse() const;

	void SetCenterOfMass(const glm::vec3& position, const glm::quat& rotation);
	glm::vec3 GetCenterOfMassPosition() const;
	glm::quat GetCenterOfMassRotation() const;

	void SetPositionSolverCount(uint32_t count);
	uint32_t GetPositionSolverCount() const;

	void SetVelocitySolverCount(uint32_t count);
	uint32_t GetVelocitySolverCount() const;

	void AddForce(const glm::vec3& force, ForceMode mode = ForceMode::Force);
	void AddTorque(const glm::vec3& torque, ForceMode mode = ForceMode::Force);

	void AddForceAtPoint(const glm::vec3& force, const glm::vec3& position, PointForceMode mode = PointForceMode::Force);

	glm::vec3 GetVelocityAtPoint(const glm::vec3& point) const;

	void ClearAllForces();
	void ClearAllTorques();

private:
	physx::PxRigidDynamic* getInternal() const { return m_actor->is<physx::PxRigidDynamic>(); }
	RigidbodyFlag m_flags = RigidbodyFlag::None;
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
	EngineApplication&                     m_engine;
	physx::PxController*                   m_controller{ nullptr };
	physx::PxRigidDynamic*                 m_rigidbody{ nullptr };
	MaterialPtr                            m_material;
};

#pragma endregion

//=============================================================================
#pragma region [ Body User Data Link ]

enum class UserDataType : uint8_t
{
	RigidBody,
	StaticBody,
	CharacterController
};

struct UserData final
{
	UserDataType type;
	void* ptr;
};

#pragma endregion

} // namespace ph