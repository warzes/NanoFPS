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

#pragma region Scene

class Scene final
{
public:
	Scene() = default;
	~Scene() = default;
	Scene(const Scene&) = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&) = delete;
	Scene& operator=(Scene&&) = delete;

	void Update(float deltaTime);
	void FixedUpdate(float fixedDeltaTime);
	void Draw();

	template<class T, class... Args, std::enable_if_t<!std::is_array_v<T>, int> = 0>
	T* CreateActor(Args &&...args)
	{
		std::unique_ptr<T> actor = std::make_unique<T>(std::forward<Args>(args)...);
		T* actorRef = actor.get();
		m_actors.emplace_back(std::move(actor));
		return actorRef;
	}

	template<class T>
	[[nodiscard]] T* FindFirstActorOfClass() const
	{
		return reinterpret_cast<T*>(findFirstActorOfClassImpl(T::ClassName));
	}

	void Register(const std::string& name, Actor* actor);

	[[nodiscard]] Actor* FindActorWithName(const std::string& name) const;

private:
	[[nodiscard]] Actor* findFirstActorOfClassImpl(const std::string& className) const;

	std::vector<std::unique_ptr<Actor>> m_actors;
	std::vector<std::unique_ptr<Actor>> m_pendingDestroyActors;

	std::map<std::string, Actor*> m_registeredActors;
};

#pragma endregion