#include "Actor.h"
#include "GameLua.h"

extern std::unique_ptr<PhysicsScene> physicsScene;
extern std::unique_ptr<PhysicsSystem> physicsSystem;
extern std::unique_ptr<GameLua> lua;
extern std::unique_ptr<PbrRenderer> renderer;
extern bool slowMotion;
extern bool showTriggers;

float GetKeyAxis(GLFWwindow* window, int posKey, int negKey)
{
	float value = 0.0f;
	if (glfwGetKey(window, posKey)) value += 1.0f;
	if (glfwGetKey(window, negKey)) value -= 1.0f;
	return value;
}

#pragma region AFuncBrush

AFuncBrush::AFuncBrush(const std::vector<MapData::Brush>& brushes, BrushType type, PhysicsLayer layer)
	: m_brushes(brushes, type, layer)
{
	const glm::vec3& center = m_brushes.GetCenter();

	GetTransform().SetPosition(center);
	m_position = center;

	m_rigidbody = physicsScene->CreateStatic(physx::PxTransform{ center.x, center.y, center.z });
	m_brushes.AttachToRigidActor(m_rigidbody);

	m_rigidbody->userData = this;
}

AFuncBrush::~AFuncBrush()
{
	PX_RELEASE(m_rigidbody)
}

void AFuncBrush::Update([[maybe_unused]] float deltaTime)
{
	const float timeError = glm::min(physicsScene->GetFixedTimestep(), physicsScene->GetFixedUpdateTimeError());
	m_translationMatrix = GetTransform().SetPosition(m_position + m_velocity * timeError).GetTranslationMatrix();
}

void AFuncBrush::FixedUpdate(float fixedDeltaTime)
{
	const physx::PxTransform transform = m_rigidbody->getGlobalPose();
	const glm::vec3 lastPosition = m_position;

	m_position = { transform.p.x, transform.p.y, transform.p.z };
	m_velocity = (m_position - lastPosition) / fixedDeltaTime;

	m_rotationMatrix = glm::mat4_cast(glm::quat{ transform.q.w, transform.q.x, transform.q.y, transform.q.z });
}

void AFuncBrush::Draw()
{
	m_brushes.Draw(m_translationMatrix * m_rotationMatrix);
}

void AFuncBrush::Move(const glm::vec3& deltaPosition)
{
	physx::PxTransform pose = m_rigidbody->getGlobalPose();
	physx::PxVec3& position = pose.p;
	position.x += deltaPosition.x;
	position.y += deltaPosition.y;
	position.z += deltaPosition.z;
	m_rigidbody->setGlobalPose(pose);
}

#pragma endregion

#pragma region AFuncMove

AFuncMove::AFuncMove(const std::vector<MapData::Brush>& brushes, const glm::vec3& moveSpeed, float moveTime, const std::string& moveSound)
	: AFuncBrush(brushes)
	, m_moveSpeed(moveSpeed)
	, m_moveTime(moveTime)
{
	if (!moveSound.empty())
	{
		//m_audio = g_Audio->CreateInstance(moveSound);
	}
}

void AFuncMove::FixedUpdate(float fixedDeltaTime)
{
	AFuncBrush::FixedUpdate(fixedDeltaTime);

	switch (m_state)
	{
	case State::MovingOpen:
	{
		float moveTime = glm::min(m_remainingTime, fixedDeltaTime);
		Move(m_moveSpeed * moveTime);
		m_remainingTime -= moveTime;
		if (m_remainingTime <= 0.0f)
		{
			//m_audio.Stop();
			m_state = State::Open;
		}
		break;
	}
	case State::MovingClose:
	{
		float moveTime = glm::min(m_remainingTime, fixedDeltaTime);
		Move(-m_moveSpeed * moveTime);
		m_remainingTime -= moveTime;
		if (m_remainingTime <= 0.0f)
		{
			//m_audio.Stop();
			m_state = State::Close;
		}
		break;
	}
	default:
		break;
	}

	//m_audio.Set3DAttributes(GetTransform().GetPosition());
}

void AFuncMove::LuaSignal(lua_State* L)
{
	const std::string event = luaL_checkstring(L, 2);
	if (event == "open")
	{
		Open();
	}
	else if (event == "close")
	{
		Close();
	}
	else if (event == "toggle")
	{
		Toggle();
	}
	else
	{
		luaL_error(L, "func_move doesn't recognize event %s", event.c_str());
	}
}

void AFuncMove::Open()
{
	switch (m_state)
	{
	case State::Close:
		//m_audio.Start();
		m_state = State::MovingOpen;
		m_remainingTime = m_moveTime;
		break;
	case State::MovingClose:
		m_state = State::MovingOpen;
		m_remainingTime = m_moveTime - m_remainingTime;
		break;
	default:
		break;
	}
}

void AFuncMove::Close() 
{
	switch (m_state)
	{
	case State::Open:
		//m_audio.Start();
		m_state = State::MovingClose;
		m_remainingTime = m_moveTime;
		break;
	case State::MovingOpen:
		m_state = State::MovingClose;
		m_remainingTime = m_moveTime - m_remainingTime;
		break;
	default:
		break;
	}
}

void AFuncMove::Toggle()
{
	switch (m_state)
	{
	case State::Close:
	case State::MovingClose:
		Open();
		break;
	case State::Open:
	case State::MovingOpen:
		Close();
		break;
	}
}

#pragma endregion

#pragma region AFuncButton

void AFuncButton::StartUse(Actor* user, const physx::PxRaycastHit& hit)
{
	Open();
	lua->CallGlobalFunction(m_event, "press");
}

void AFuncButton::StopUse(Actor* user)
{
	Close();
	lua->CallGlobalFunction(m_event, "release");
}

#pragma endregion

#pragma region AFuncPhys

AFuncPhys::AFuncPhys(const std::vector<MapData::Brush>& brushes)
	: m_brushes(brushes)
{
	const glm::vec3& center = m_brushes.GetCenter();

	GetTransform().SetPosition(center);

	m_rigidbody = physicsScene->CreateDynamic(physx::PxTransform{ center.x, center.y, center.z });
	m_brushes.AttachToRigidActor(m_rigidbody);
	physx::PxRigidBodyExt::updateMassAndInertia(*m_rigidbody, 1.0f);

	m_rigidbody->userData = this;
}

AFuncPhys::~AFuncPhys()
{
	PX_RELEASE(m_rigidbody)
}

void AFuncPhys::FixedUpdate(float fixedDeltaTime)
{
	const physx::PxTransform transform = m_rigidbody->getGlobalPose();
	const glm::vec3          position{ transform.p.x, transform.p.y, transform.p.z };
	const glm::quat          rotation{ transform.q.w, transform.q.x, transform.q.y, transform.q.z };
	m_modelMatrix = GetTransform().SetPosition(position).GetTranslationMatrix() * mat4_cast(rotation);
}

void AFuncPhys::Draw()
{
	m_brushes.Draw(m_modelMatrix);
}

#pragma endregion

#pragma region ALightPoint

ALightPoint::ALightPoint(const glm::vec3& position, const glm::vec3& color, float radius)
	: m_position(position)
	, m_color(color)
	, m_radius(radius)
{
}

void ALightPoint::Draw()
{
	renderer->DrawPointLight(m_position, m_color, m_radius);
}

#pragma endregion

#pragma region APlayer

APlayer::APlayer(const glm::vec3& position, float yaw, float mouseSpeed)
	: m_movement(this, position)
	, m_use(this)
{
	GetTransform().RotateY(yaw);

	m_mouseSpeed = mouseSpeed;

	Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
}

APlayer::~APlayer()
{
	Mouse::SetCursorMode(Mouse::CursorMode::Normal);
}

void APlayer::Update(const float deltaTime)
{
	Transform& transform = GetTransform();

	// turn
	{
		const float      mouseSpeed = slowMotion ? m_mouseSpeed * 0.4f : m_mouseSpeed;
		const glm::vec2& deltaMousePos = Mouse::GetDeltaPosition();
		transform.RotateX(mouseSpeed * deltaMousePos.y).RotateY(mouseSpeed * deltaMousePos.x).ClampPitch();
	}

	// move
	{
		glm::vec3 movementInput = transform.GetHorizontalRightVector() * GetKeyAxis(Window::GetWindow(), GLFW_KEY_D, GLFW_KEY_A) + transform.GetHorizontalForwardVector() * GetKeyAxis(Window::GetWindow(), GLFW_KEY_W, GLFW_KEY_S);
		if (movementInput.x != 0 || movementInput.y != 0 || movementInput.z != 0)
		{
			movementInput = glm::normalize(movementInput);
		}
		m_movement.SetMovementInput(movementInput);
	}

	// jump
	{
		bool currSpace = glfwGetKey(Window::GetWindow(), GLFW_KEY_SPACE);
		if (!m_prevSpace && currSpace)
		{
			m_movement.Jump();
		}
		m_prevSpace = currSpace;
	}

	// use input
	m_use.SetInput(Mouse::IsButtonDown(Mouse::Button::Left));

	m_movement.Update(deltaTime);
	m_use.Update();

	//g_Audio->SetListener3DAttributes(transform.GetPosition(), m_movement.GetVelocity(), transform.GetHorizontalForwardVector());
}

void APlayer::FixedUpdate(float fixedDeltaTime)
{
	m_movement.FixedUpdate(fixedDeltaTime);
}

void APlayer::Draw()
{
	const Transform& transform = GetTransform();
	renderer->SetCameraData(transform.GetPosition(), transform.GetInverseMatrix(), glm::radians(60.0f), 0.01f, 100.0f);

	drawReticle();
}

void drawReticleStandard(const glm::vec2& position, float gap, float max, const glm::vec4& color)
{
	renderer->DrawScreenLine(position + glm::vec2{ gap, 0 }, position + glm::vec2{ max, 0 }, color);
	renderer->DrawScreenLine(position + glm::vec2{ -gap, 0 }, position + glm::vec2{ -max, 0 }, color);
	renderer->DrawScreenLine(position + glm::vec2{ 0, gap }, position + glm::vec2{ 0, max }, color);
	renderer->DrawScreenLine(position + glm::vec2{ 0, -gap }, position + glm::vec2{ 0, -max }, color);
}

void drawReticleDiagonal(const glm::vec2& position, float gap, float max, const glm::vec4& color) 
{
	renderer->DrawScreenLine(position + glm::vec2{ gap, gap }, position + glm::vec2{ max, max }, color);
	renderer->DrawScreenLine(position + glm::vec2{ -gap, gap }, position + glm::vec2{ -max, max }, color);
	renderer->DrawScreenLine(position + glm::vec2{ gap, -gap }, position + glm::vec2{ max, -max }, color);
	renderer->DrawScreenLine(position + glm::vec2{ -gap, -gap }, position + glm::vec2{ -max, -max }, color);
}

void APlayer::drawReticle()
{
	const glm::vec2 screenCenter = renderer->GetScreenExtent() * 0.5f;
	const glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };

	if (m_use.GetTarget())
		drawReticleDiagonal(screenCenter, 6.0f, 18.0f, color);
	else
		drawReticleStandard(screenCenter, 4.0f, 12.0f, color);
}

#pragma endregion

#pragma region APlayerNoClip

APlayerNoClip::APlayerNoClip(const glm::vec3& position) 
{
	GetTransform().SetPosition(position);
	Mouse::SetCursorMode(Mouse::CursorMode::Disabled);
}

APlayerNoClip::~APlayerNoClip()
{
	Mouse::SetCursorMode(Mouse::CursorMode::Normal);
}

void APlayerNoClip::Update(float deltaTime)
{
	const glm::vec3 inputDirection = GetTransform().GetHorizontalRightVector() * GetKeyAxis(Window::GetWindow(), GLFW_KEY_D, GLFW_KEY_A) +
		glm::vec3(0.0f, 1.0f, 0.0f) * GetKeyAxis(Window::GetWindow(), GLFW_KEY_E, GLFW_KEY_Q) +
		GetTransform().GetHorizontalForwardVector() * GetKeyAxis(Window::GetWindow(), GLFW_KEY_W, GLFW_KEY_S);

	if (inputDirection.x != 0.0f || inputDirection.y != 0.0f || inputDirection.z != 0.0f)
	{
		GetTransform().Translate(glm::normalize(inputDirection) * (5.0f * deltaTime));
	}

	const glm::vec2& deltaMousePos = Mouse::GetDeltaPosition();

	GetTransform().RotateX(0.001f * deltaMousePos.y).RotateY(0.001f * deltaMousePos.x).ClampPitch();
}

void APlayerNoClip::Draw()
{
	renderer->SetCameraData(GetTransform().GetPosition(), GetTransform().GetInverseMatrix(), glm::radians(60.0f), 0.01f, 100.0f);
}

#pragma endregion

#pragma region APropPowerSphere

APropPowerSphere::APropPowerSphere(const glm::vec3& position)
{
	GetTransform().SetPosition(position);
	m_position = position;

	m_rigidbody = physicsScene->CreateDynamic(
		physx::PxTransform{ position.x, position.y, position.z },
		physx::PxSphereGeometry(0.5f)
	);
	enableDamping();

	m_rigidbody->userData = this;

	m_physicsMaterial = physicsSystem->CreateMaterial(0.8f, 0.8f, 1.0f);
	PhysicsSetActorMaterial(m_rigidbody, m_physicsMaterial);

	m_mesh = renderer->LoadObjMesh("models/power_sphere.obj");
	m_material = renderer->LoadPbrMaterial("materials/power_sphere.json");
}

APropPowerSphere::~APropPowerSphere()
{
	PX_RELEASE(m_rigidbody)
	PX_RELEASE(m_physicsMaterial)
}

void APropPowerSphere::Update(float deltaTime)
{
	const float timeError = glm::min(physicsScene->GetFixedTimestep(), physicsScene->GetFixedUpdateTimeError());

	m_translationMatrix = GetTransform().SetPosition(m_position + m_velocity * timeError).GetTranslationMatrix();
	m_rotation = glm::slerp(m_rotation, m_currentRotation, glm::min(1.0f, 30.0f * deltaTime));
}

void APropPowerSphere::FixedUpdate(float fixedDeltaTime)
{
	const physx::PxTransform transform = m_rigidbody->getGlobalPose();

	const glm::vec3 lastPosition = m_position;

	m_position = { transform.p.x, transform.p.y, transform.p.z };
	m_velocity = (m_position - lastPosition) / fixedDeltaTime;

	m_currentRotation = { transform.q.w, transform.q.x, transform.q.y, transform.q.z };

	if (m_beingPushed)
	{
		physx::PxRigidBodyExt::addForceAtPos(*m_rigidbody, m_pushForce, m_pushPosition, physx::PxForceMode::eFORCE);
	}
}

void APropPowerSphere::Draw()
{
	renderer->Draw(m_mesh, m_translationMatrix * glm::mat4_cast(m_rotation), m_material);
}

void APropPowerSphere::enableDamping()
{
	m_rigidbody->setLinearDamping(1.0f);
	m_rigidbody->setAngularDamping(1.0f);
}

void APropPowerSphere::disableDamping()
{
	m_rigidbody->setLinearDamping(0.0f);
	m_rigidbody->setAngularDamping(0.0f);
}

void APropPowerSphere::StartUse(Actor* user, const physx::PxRaycastHit& hit)
{
	disableDamping();
	m_beingPushed = true;

	ContinueUse(user, hit); // trigger the first push position/force update
}

void APropPowerSphere::ContinueUse(Actor* user, const physx::PxRaycastHit& hit)
{
	const glm::vec3& userPosition = user->GetTransform().GetPosition();

	m_pushPosition = hit.position;
	m_pushForce = 
	{
		m_pushPosition.x - userPosition.x,
		m_pushPosition.y - userPosition.y,
		m_pushPosition.z - userPosition.z,
	};
	m_pushForce = m_pushForce.getNormalized() * 3.0f;
}

void APropPowerSphere::StopUse(Actor* user)
{
	m_beingPushed = false;
	enableDamping();
}

#pragma endregion

#pragma region APropTestModel

APropTestModel::APropTestModel(const std::string& meshName, const std::string& materialName, const glm::vec3& position)
{
	m_transform.SetPosition(position);
	m_mesh = renderer->LoadObjMesh(meshName);
	m_material = renderer->LoadPbrMaterial(materialName);
}

void APropTestModel::Draw()
{
	renderer->Draw(m_mesh, m_transform.GetMatrix(), m_material);
}

#pragma endregion

#pragma region ATrigger

static bool CheckTargetType(Actor* actor, ATrigger::TargetType targetType)
{
	switch (targetType)
	{
	case ATrigger::TARGET_TYPE_PLAYER:
		return actor->IsClass<APlayer>();
	case ATrigger::TARGET_TYPE_POWER_SPHERE:
		return actor->IsClass<APropPowerSphere>();
	}
	return false;
}

void ATrigger::OnTriggerEnter(Actor* other)
{
	if (m_event.empty() || !CheckTargetType(other, m_targetType))
		return;

	lua->CallGlobalFunction(m_event, "enter");
}

void ATrigger::OnTriggerExit(Actor* other)
{
	if (m_event.empty() || !CheckTargetType(other, m_targetType))
		return;

	lua->CallGlobalFunction(m_event, "exit");
}

void ATrigger::Draw()
{
	if (showTriggers) AFuncBrush::Draw();
}

#pragma endregion

#pragma region AWorldSpawn

AWorldSpawn::AWorldSpawn(
	const std::vector<MapData::Brush>& brushes,
	const glm::vec3& lightDirection,
	const glm::vec3& lightColor,
	const std::string& script
)
	: AFuncBrush(brushes)
{
	for (const auto& brush : brushes)
	{
		for (const auto& vertex : brush.Vertices)
		{
			m_geomMin.x = glm::min(vertex.x, m_geomMin.x);
			m_geomMin.y = glm::min(vertex.y, m_geomMin.y);
			m_geomMin.z = glm::min(vertex.z, m_geomMin.z);
			m_geomMax.x = glm::max(vertex.x, m_geomMax.x);
			m_geomMax.y = glm::max(vertex.y, m_geomMax.y);
			m_geomMax.z = glm::max(vertex.z, m_geomMax.z);
		}
	}

	m_lightDirection = lightDirection;
	m_lightColor = lightColor;

	if (!script.empty())
	{
		lua->DoFile(script);
	}
}

void AWorldSpawn::Draw()
{
	renderer->SetLightingData(m_lightDirection, m_lightColor);
	renderer->SetWorldBounds(m_geomMin, m_geomMax);
	AFuncBrush::Draw();
}

#pragma endregion
