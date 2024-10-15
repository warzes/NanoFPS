#pragma once

#include "Physics.h"

namespace ph {

// Flags for controlling physics behaviour globally.
enum PhysicsFlag
{
	// Automatically recovers character controllers that are interpenetrating geometry. This can happen if a controller is spawned or teleported into geometry, its size/rotation is changed so it penetrates geometry, or simply because of numerical imprecision.
	CCT_OverlapRecovery = 1 << 0,
	// Performs more accurate sweeps when moving the character controller, making it less likely to interpenetrate geometry. When overlap recovery is turned on you can consider turning this off as it can compensate for the less precise sweeps.
	CCT_PreciseSweeps = 1 << 1,
	// Large triangles can cause problems with character controller collision. If this option is enabled the triangles larger than a certain size will be automatically tesselated into smaller triangles, in order to help with precision.
	CCT_Tesselation = 1 << 2,
	// Enables continous collision detection. This will prevent fast-moving objects from tunneling through each other. You must also enable CCD for individual Rigidbodies. This option can have a significant performance impact.
	CCD_Enable = 1 << 3
};
typedef Flags<PhysicsFlag> PhysicsFlags;
FLAGS_OPERATORS(PhysicsFlag);

//=============================================================================
#pragma region [ Physics Scene ]

class PhysicsSystem;

struct PhysicsSceneCreateInfo final
{
	glm::vec3          gravity{ 0.0f, -9.81f, 0.0f };
	MaterialCreateInfo defaultMaterial{ 0.8f, 0.8f, 0.25f };
	// Flags that control global physics option.
	PhysicsFlags       flags = PhysicsFlag::CCT_OverlapRecovery | PhysicsFlag::CCT_PreciseSweeps | PhysicsFlag::CCD_Enable;
};

class PhysicsScene final
{
	friend EngineApplication;
public:
	PhysicsScene(EngineApplication& engine, PhysicsSystem& physicsEngine);

	[[nodiscard]] bool Setup(const PhysicsSceneCreateInfo& createInfo);
	void Shutdown();

	void FixedUpdate();

	// Checks is a specific physics option enabled.
	[[nodiscard]] bool HasFlag(PhysicsFlags flag) const { return m_flags & flag; }
	// Enables or disabled a specific physics option.
	void SetFlag(PhysicsFlags flag, bool enabled) { if (enabled) m_flags |= flag; else m_flags &= ~flag; }
	// Returns a maximum edge length before a triangle is tesselated.
	[[nodiscard]] float GetMaxTesselationEdgeLength() const;
	//  Sets a maximum edge length before a triangle is tesselated.
	void SetMaxTesselationEdgeLength(float length);

	[[nodiscard]] glm::vec3 GetGravity() const;
	// Determines the global gravity value for all objects in the scene.
	void SetGravity(const glm::vec3& gravity);

	// Adds a new physics region. Certain physics options require you to set up regions in which physics objects are allowed to be in, and objects outside of these regions will not be handled by physics. You do not need to set up these regions by default.
	uint32_t AddBroadPhaseRegion(const AABB& region);
	[[nodiscard]] void RemoveBroadPhaseRegion(uint32_t handle);
	// Removes all physics regions.
	void ClearBroadPhaseRegions();





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
	PhysicsFlags                   m_flags{};
	physx::PxPhysics*              m_physics{ nullptr };
	physx::PxScene*                m_scene{ nullptr };
	MaterialPtr                    m_defaultMaterial{ nullptr };
	physx::PxControllerManager*    m_controllerManager{ nullptr };
};

#pragma endregion

} // namespace ph