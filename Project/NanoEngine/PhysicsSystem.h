#pragma once

#include "PhysicsScene.h"

class EngineApplication;

namespace ph {

//=============================================================================
#pragma region [ Physics System ]

// Flags for controlling physics behaviour globally.
enum PhysicsFlag
{
	/**
	* Automatically recovers character controllers that are interpenetrating geometry. This can happen if a controller
	* is spawned or teleported into geometry, its size/rotation is changed so it penetrates geometry, or simply
	* because of numerical imprecision.
	*/
	CCT_OverlapRecovery = 1 << 0,
	/**
	* Performs more accurate sweeps when moving the character controller, making it less likely to interpenetrate
	* geometry. When overlap recovery is turned on you can consider turning this off as it can compensate for the
	* less precise sweeps.
	*/
	CCT_PreciseSweeps = 1 << 1,
	/**
	* Large triangles can cause problems with character controller collision. If this option is enabled the triangles
	* larger than a certain size will be automatically tesselated into smaller triangles, in order to help with
	* precision.
	*
	* @see Physics::getMaxTesselationEdgeLength
	*/
	CCT_Tesselation = 1 << 2,
	/**
	* Enables continous collision detection. This will prevent fast-moving objects from tunneling through each other.
	* You must also enable CCD for individual Rigidbodies. This option can have a significant performance impact.
	*/
	CCD_Enable = 1 << 3
};
typedef Flags<PhysicsFlag> PhysicsFlags;
FLAGS_OPERATORS(PhysicsFlag);

struct PhysicsCreateInfo final
{
	PhysicsSceneCreateInfo scene;

	float typicalLength = 1.0f; // Typical length of an object in the scene.
	float typicalSpeed = 10.0f; // Typical speed of an object in the scene.

	// Flags that control global physics option.
	PhysicsFlags flags = PhysicsFlag::CCT_OverlapRecovery | PhysicsFlag::CCT_PreciseSweeps | PhysicsFlag::CCD_Enable;
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

	[[nodiscard]] MaterialPtr CreateMaterial(const MaterialCreateInfo& createInfo);

	[[nodiscard]] auto GetPxPhysics() { return m_physics; }

private:
	EngineApplication&       m_engine;
	physx::PxTolerancesScale m_scale{};
	physx::PxFoundation*     m_foundation{ nullptr };
	physx::PxPhysics*        m_physics{ nullptr };
	ph::PhysicsScene         m_scene; // TODO: в будущем создавать из physics system
	bool                     m_enable{ false };
};

#pragma endregion

} // namespace ph