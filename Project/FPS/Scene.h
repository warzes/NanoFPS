#pragma once

#include "EngineMath.h"

#pragma region Actor

#define DEFINE_ACTOR_CLASS(className)                        \
	className(const className &) = delete;                   \
	className &operator=(const className &) = delete;        \
	className(className &&) = delete;                        \
	className &operator=(className &&) = delete;             \
	static inline const std::string &ClassName = #className; \
	const std::string &GetActorClassName() const override {  \
		return ClassName;                                    \
	}

class Actor
{
public:
	Actor() = default;
	Actor(const Actor&) = delete;
	Actor(Actor&&) = delete;
	virtual ~Actor() = default;

	Actor& operator=(const Actor&) = delete;
	Actor& operator=(Actor&&) = delete;

	[[nodiscard]] virtual const std::string& GetActorClassName() const = 0;

	template<class T>
	[[nodiscard]] bool IsClass() const
	{
		return T::ClassName == GetActorClassName();
	}

	template<class T>
	T* Cast()
	{
		return IsClass<T>() ? reinterpret_cast<T*>(this) : nullptr;
	}

	virtual void Update([[maybe_unused]] float deltaTime) {}
	virtual void FixedUpdate([[maybe_unused]] float fixedDeltaTime) {}
	virtual void Draw() {}

	virtual void LuaSignal([[maybe_unused]] lua_State* L) {}

	virtual void StartUse([[maybe_unused]] Actor* user, [[maybe_unused]] const physx::PxRaycastHit& hit) {}
	virtual void ContinueUse([[maybe_unused]] Actor* user, [[maybe_unused]] const physx::PxRaycastHit& hit) {}
	virtual void StopUse([[maybe_unused]] Actor* user) {}

	virtual void OnTriggerEnter([[maybe_unused]] Actor* other) {}
	virtual void OnTriggerExit([[maybe_unused]] Actor* other) {}

	[[nodiscard]] bool IsPendingDestroy() const { return m_pendingDestroy; }

	void Destroy() { m_pendingDestroy = true; }

	const Transform& GetTransform() const { return m_transform; }

	Transform& GetTransform() { return m_transform; }

private:
	bool      m_pendingDestroy = false;
	Transform m_transform{};
};

#pragma endregion

#pragma region APlayerNoClip

class APlayerNoClip final : public Actor
{
public:
	DEFINE_ACTOR_CLASS(APlayerNoClip)

	explicit APlayerNoClip(const glm::vec3& position);
	~APlayerNoClip() final;

	void Update(float deltaTime) final;
	void Draw() final;
};

#pragma endregion