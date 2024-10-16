#pragma once

#include "Platform.h"
#include "RenderSystem.h"
#include "PhysicsSystem.h"

//=============================================================================
#pragma region [ Create Application Info ]

struct EngineApplicationCreateInfo final
{
	std::string_view      logFilePath = "Log.txt";
	WindowCreateInfo      window{};
	vkr::RenderCreateInfo render{};
	ph::PhysicsCreateInfo physics{};
};

#pragma endregion

//=============================================================================
#pragma region [ Engine Application ]

enum class StatusApp : uint8_t
{
	NonInit,
	Success,
	ErrorFailed,
	Quit
};

class EngineApplication
{
	friend class Window;
	friend class WindowEvents;
public:
	EngineApplication();
	virtual ~EngineApplication();

	void Run();

	virtual EngineApplicationCreateInfo Config() const { return {}; }
	virtual bool Setup() { return true; }
	virtual void Shutdown() {}

	// Window resize event
	virtual void Resize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height) {}
	// Window iconify event
	virtual void WindowIconify([[maybe_unused]] bool iconified) {}
	// Window maximize event
	virtual void WindowMaximize([[maybe_unused]] bool maximized) {}
	// Mouse down event
	virtual void MouseDown([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] MouseButton buttons) {}
	// Mouse up event
	virtual void MouseUp([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] MouseButton buttons) {}
	// Mouse move event
	virtual void MouseMove([[maybe_unused]] int32_t x, [[maybe_unused]] int32_t y, [[maybe_unused]] int32_t dx, [[maybe_unused]] int32_t dy, [[maybe_unused]] MouseButton buttons) {}
	// Key down event
	virtual void KeyDown([[maybe_unused]] KeyCode key) {}
	// Key up event
	virtual void KeyUp([[maybe_unused]] KeyCode key) {}
	// Mouse wheel or touchpad scroll event
	virtual void Scroll([[maybe_unused]] float dx, [[maybe_unused]] float dy) {}

	virtual void Update() {}
	virtual void FixedUpdate([[maybe_unused]] float fixedDeltaTime) {}
	virtual void Render() {}

	void Quit();
	void Failed();

	void Print(const std::string& msg);
	void Warning(const std::string& msg);
	void Error(const std::string& msg);
	void Fatal(const std::string& msg);

	Window& GetWindow() { return m_window; }
	Input& GetInput() { return m_input; }
	vkr::RenderSystem& GetRender() { return m_render; }
	vkr::RenderDevice& GetRenderDevice() { return m_render.GetRenderDevice(); }

	ph::PhysicsSystem& GetPhysicsSystem() { return m_physics; }
	ph::PhysicsScene& GetPhysicsScene() { return m_physics.GetScene(); }

	const KeyState& GetKeyState(KeyCode code) const;

	uint32_t GetWindowWidth() const { return m_window.GetWidth(); }
	uint32_t GetWindowHeight() const { return m_window.GetHeight(); }
	float GetWindowAspect() const { return static_cast<float>(GetWindowWidth()) / static_cast<float>(GetWindowHeight()); }

	bool IsWindowIconified() const;
	bool IsWindowMaximized() const;

	[[nodiscard]] float GetDeltaTime() const { return m_deltaTime; }
	[[nodiscard]] float GetFixedTimestep() const { return m_fixedTimestep; }
	[[nodiscard]] float GetFixedUpdateTimeError() const { return m_timeSinceLastTick; }

private:
	bool setup();

	bool initializeLog(std::string_view filePath);
	void shutdownLog();

	void resizeCallback(uint32_t width, uint32_t height);
	void windowIconifyCallback(bool iconified);
	void windowMaximizeCallback(bool maximized);
	void mouseDownCallback(int32_t x, int32_t y, MouseButton buttons);
	void mouseUpCallback(int32_t x, int32_t y, MouseButton buttons);
	void mouseMoveCallback(int32_t x, int32_t y, MouseButton buttons);
	void scrollCallback(float dx, float dy);
	void keyDownCallback(KeyCode key);
	void keyUpCallback(KeyCode key);

	std::ofstream     m_logFile;
	Window            m_window;
	Input             m_input;
	vkr::RenderSystem m_render;
	ph::PhysicsSystem m_physics;
	StatusApp         m_status = StatusApp::NonInit;
	int32_t           m_previousMouseX = INT32_MAX;
	int32_t           m_previousMouseY = INT32_MAX;
	KeyState          m_keyStates[TOTAL_KEY_COUNT] = { {false, 0.0f} };

	float             m_lastFrameTime{};
	float             m_deltaTime{};
	float             m_fixedTimestep{ 0.02f };
	float             m_timeSinceLastTick{ 0.0f };
};

#pragma endregion