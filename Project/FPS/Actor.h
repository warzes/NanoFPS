#pragma once

#include "MapData.h"
#include "Player.h"

#pragma region AFuncBrush

class AFuncBrush : public Actor
{
public:
	DEFINE_ACTOR_CLASS(AFuncBrush);

	explicit AFuncBrush(const std::vector<MapData::Brush>& brushes, BrushType type = BrushType::Normal, PhysicsLayer layer = PHYSICS_LAYER_0);
	~AFuncBrush() override;

	void Update(float deltaTime) override;
	void FixedUpdate(float fixedDeltaTime) override;
	void Draw() override;
	void Move(const glm::vec3& deltaPosition);

private:
	Brushes               m_brushes;
	physx::PxRigidStatic* m_rigidbody;

	glm::mat4 m_translationMatrix{ 1.0f };
	glm::mat4 m_rotationMatrix{ 1.0f };

	glm::vec3 m_position{};
	glm::vec3 m_velocity{};
};

#pragma endregion

#pragma region AFuncMove

class AFuncMove : public AFuncBrush 
{
public:
	DEFINE_ACTOR_CLASS(AFuncMove);

	AFuncMove(const std::vector<MapData::Brush>& brushes, const glm::vec3& moveSpeed, float moveTime, const std::string& moveSound = "");

	void FixedUpdate(float fixedDeltaTime) override;

	void LuaSignal(lua_State* L) override;

	[[nodiscard]] bool IsMoving() const { return m_state == State::MovingOpen || m_state == State::MovingClose; }
	[[nodiscard]] bool IsOpen() const { return m_state == State::Open; }
	[[nodiscard]] bool IsClosed() const { return m_state == State::Close; }

	void Open();
	void Close();
	void Toggle();

private:
	enum class State
	{
		Close,
		MovingOpen,
		Open,
		MovingClose
	};

	State m_state = State::Close;
	float m_remainingTime = 0.0f;

	glm::vec3 m_moveSpeed{ 0.0f, 0.0f, 0.0f };
	float     m_moveTime = 0.0f;

	//AudioInstance m_audio;
};

#pragma endregion

#pragma region AFuncButton

class AFuncButton final : public AFuncMove
{
public:
	DEFINE_ACTOR_CLASS(AFuncButton)

	AFuncButton(const std::vector<MapData::Brush>& brushes, const glm::vec3& moveSpeed, float moveTime, std::string event)
		: AFuncMove(brushes, moveSpeed, moveTime)
		, m_event(std::move(event)) {}

	void StartUse(Actor* user, const physx::PxRaycastHit& hit) final;
	void StopUse(Actor* user) final;

private:
	std::string m_event;
};

#pragma endregion

#pragma region AFuncPhys

class AFuncPhys final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(AFuncPhys)

	explicit AFuncPhys(const std::vector<MapData::Brush>& brushes);
	~AFuncPhys() final;

	void FixedUpdate(float fixedDeltaTime) final;

	void Draw() final;

private:
	Brushes                m_brushes;
	physx::PxRigidDynamic* m_rigidbody;
	glm::mat4 m_modelMatrix{ 1.0f };
};

#pragma endregion

#pragma region ALightPoint

class ALightPoint final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(ALightPoint)

	explicit ALightPoint(const glm::vec3& position, const glm::vec3& color, float radius);

	void Draw() final;

private:
	glm::vec3 m_position;
	glm::vec3 m_color;
	float     m_radius;
};

#pragma endregion

#pragma region APlayer

class APlayer final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(APlayer)

	explicit APlayer(const glm::vec3& position, float yaw = 0.0f, float mouseSpeed = 0.001f);
	~APlayer() final;

	void Update(float deltaTime) final;
	void FixedUpdate(float fixedDeltaTime) final;
	void Draw() final;

private:
	void drawReticle();

	PlayerMovement m_movement;
	PlayerUse      m_use;

	float m_mouseSpeed;

	bool m_prevSpace = false;
};

#pragma endregion

#pragma region APropPowerSphere

class APropPowerSphere final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(APropPowerSphere)

	explicit APropPowerSphere(const glm::vec3& position);
	~APropPowerSphere() final;

	void Update(float deltaTime) final;
	void FixedUpdate(float fixedDeltaTime) final;
	void Draw() final;

	void StartUse(Actor* user, const physx::PxRaycastHit& hit) final;
	void ContinueUse(Actor* user, const physx::PxRaycastHit& hit) final;
	void StopUse(Actor* user) final;

private:
	void enableDamping();
	void disableDamping();

	physx::PxRigidDynamic* m_rigidbody;
	physx::PxMaterial* m_physicsMaterial;

	glm::mat4 m_translationMatrix{ 1.0f };

	glm::vec3 m_position{};
	glm::vec3 m_velocity{};

	glm::quat m_rotation{};
	glm::quat m_currentRotation{};

	VulkanMesh* m_mesh;
	PbrMaterial* m_material;

	bool          m_beingPushed = false;
	physx::PxVec3 m_pushPosition;
	physx::PxVec3 m_pushForce;
};

#pragma endregion

#pragma region APropTestModel

class APropTestModel final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(APropTestModel)

	APropTestModel(const std::string& meshName, const std::string& materialName, const glm::vec3& position);

	void Draw() final;

private:
	Transform m_transform;

	VulkanMesh* m_mesh;
	PbrMaterial* m_material;
};

#pragma endregion

#pragma region ATrigger

class ATrigger : public AFuncBrush
{
public:
	DEFINE_ACTOR_CLASS(ATrigger)

	enum TargetType
	{
		TARGET_TYPE_PLAYER = 0,
		TARGET_TYPE_POWER_SPHERE = 1,
	};

	ATrigger(const std::vector<MapData::Brush>& brushes, int targetType, std::string event)
		: AFuncBrush(brushes, BrushType::Trigger, PHYSICS_LAYER_1)
		, m_targetType(static_cast<TargetType>(targetType))
		, m_event(std::move(event)) {}

	void Draw() override;
	void OnTriggerEnter(Actor* other) override;
	void OnTriggerExit(Actor* other) override;

private:
	TargetType  m_targetType;
	std::string m_event;
};

#pragma endregion

#pragma region ATriggerOnce

class ATriggerOnce final : public ATrigger
{
public:
	ATriggerOnce(const std::vector<MapData::Brush>& brushes, int targetType, std::string event)
		: ATrigger(brushes, targetType, std::move(event)) {}

	void OnTriggerEnter(Actor* other) final
	{
		ATrigger::OnTriggerEnter(other);
		Destroy();
	}
};

#pragma endregion

#pragma region AWorldSpawn

class AWorldSpawn final : public AFuncBrush
{
public:
	DEFINE_ACTOR_CLASS(AWorldSpawn)

	explicit AWorldSpawn(
		const std::vector<MapData::Brush>& brushes,
		const glm::vec3& lightDirection,
		const glm::vec3& lightColor,
		const std::string& script
	);

	void Draw() final;

private:
	glm::vec3 m_geomMin{ std::numeric_limits<float>::max() };
	glm::vec3 m_geomMax{ std::numeric_limits<float>::min() };

	glm::vec3 m_lightDirection{};
	glm::vec3 m_lightColor{};
};

#pragma endregion