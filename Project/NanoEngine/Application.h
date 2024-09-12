#pragma once

#include "Platform.h"
#include "RenderSystem.h"

#pragma region Create Application Info

struct EngineApplicationCreateInfo final
{
	std::string_view logFilePath = "Log.txt";

	WindowCreateInfo window{};
	RenderCreateInfo render{};
};

#pragma endregion

#pragma region IApplication

class EngineApplication;

class IApplication
{
	friend class EngineApplication;
public:
	IApplication() = default;
	virtual ~IApplication() = default;

	[[nodiscard]] virtual const EngineApplicationCreateInfo Config() const { return {}; }

	virtual void Setup() {}
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
	virtual void Render() {}

	void Quit();

	void Print(const std::string& msg);
	void Warning(const std::string& msg);
	void Error(const std::string& msg);
	void Fatal(const std::string& msg);

	Window& GetWindow();
	Input& GetInput();
	RenderSystem& GetRender();

	EngineApplication* engine = nullptr;
};

#pragma endregion

#pragma region Engine Application

class EngineApplication final
{
	friend class Window;
	friend class WindowEvents;
public:
	template<typename T>
	static void Run()
	{
		EngineApplication engine;
		engine.run((IApplication*)new T);
	}

	void Quit();
	void Failed();

	void Print(const std::string& msg);
	void Warning(const std::string& msg);
	void Error(const std::string& msg);
	void Fatal(const std::string& msg);

	Window& GetWindow() { return m_window; }
	Input& GetInput() { return m_input; }
	RenderSystem& GetRender() { return m_render; }

	const KeyState& GetKeyState(KeyCode code) const;

	bool IsWindowIconified() const;
	bool IsWindowMaximized() const;

private:
	EngineApplication();
	~EngineApplication();

	void run(IApplication* app);
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

	IApplication* m_app = nullptr;

	enum class StatusApp : uint8_t
	{
		NonInit,
		Success,
		ErrorFailed,
		Quit
	};
	StatusApp m_status = StatusApp::NonInit;

	struct
	{
		std::ofstream fileStream;
	} m_log;

	Window m_window;
	Input m_input;
	RenderSystem m_render;

	int32_t m_previousMouseX = INT32_MAX;
	int32_t m_previousMouseY = INT32_MAX;
	KeyState m_keyStates[TOTAL_KEY_COUNT] = { false, 0.0f };
};

#pragma endregion