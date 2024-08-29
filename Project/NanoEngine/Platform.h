#pragma once

#pragma region Input

enum MouseButton
{
	MOUSE_BUTTON_LEFT = 0x00000001,
	MOUSE_BUTTON_RIGHT = 0x00000002,
	MOUSE_BUTTON_MIDDLE = 0x00000004,
};

enum KeyCode
{
	KEY_UNDEFINED = 0,
	KEY_RANGE_FIRST = 32,
	KEY_SPACE = KEY_RANGE_FIRST,
	KEY_APOSTROPHE,
	KEY_COMMA,
	KEY_MINUS,
	KEY_PERIOD,
	KEY_SLASH,
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_SEMICOLON,
	KEY_EQUAL,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_LEFT_BRACKET,
	KEY_BACKSLASH,
	KEY_RIGHT_BRACKET,
	KEY_GRAVE_ACCENT,
	KEY_WORLD_1,
	KEY_WORLD_2,
	KEY_ESCAPE,
	KEY_ENTER,
	KEY_TAB,
	KEY_BACKSPACE,
	KEY_INSERT,
	KEY_DELETE,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_DOWN,
	KEY_UP,
	KEY_PAGE_UP,
	KEY_PAGE_DOWN,
	KEY_HOME,
	KEY_END,
	KEY_CAPS_LOCK,
	KEY_SCROLL_LOCK,
	KEY_NUM_LOCK,
	KEY_PRINT_SCREEN,
	KEY_PAUSE,
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	KEY_F25,
	KEY_KEY_PAD_0,
	KEY_KEY_PAD_1,
	KEY_KEY_PAD_2,
	KEY_KEY_PAD_3,
	KEY_KEY_PAD_4,
	KEY_KEY_PAD_5,
	KEY_KEY_PAD_6,
	KEY_KEY_PAD_7,
	KEY_KEY_PAD_8,
	KEY_KEY_PAD_9,
	KEY_KEY_PAD_DECIMAL,
	KEY_KEY_PAD_DIVIDE,
	KEY_KEY_PAD_MULTIPLY,
	KEY_KEY_PAD_SUBTRACT,
	KEY_KEY_PAD_ADD,
	KEY_KEY_PAD_ENTER,
	KEY_KEY_PAD_EQUAL,
	KEY_LEFT_SHIFT,
	KEY_LEFT_CONTROL,
	KEY_LEFT_ALT,
	KEY_LEFT_SUPER,
	KEY_RIGHT_SHIFT,
	KEY_RIGHT_CONTROL,
	KEY_RIGHT_ALT,
	KEY_RIGHT_SUPER,
	KEY_MENU,
	KEY_RANGE_LAST = KEY_MENU,
	TOTAL_KEY_COUNT,
};

struct KeyState
{
	bool  down = false;
	float timeDown = FLT_MAX;
};

const char* GetKeyCodeString(KeyCode code);

#pragma endregion

#pragma region Window

enum WindowState
{
	WINDOW_STATE_RESTORED = 0,
	WINDOW_STATE_ICONIFIED = 1,
	WINDOW_STATE_MAXIMIZED = 2,
};

struct SurfaceCreateInfo;

class Application;

struct WindowSize
{
	uint32_t width = 0;
	uint32_t height = 0;

	WindowSize() = default;
	WindowSize(uint32_t width_, uint32_t height_)
		: width(width_), height(height_) {}

	bool operator==(const WindowSize& other) const { return width == other.width && height == other.height; };
};

class Window
{
public:
	static std::unique_ptr<Window> GetImplHeadless(Application*);
	static std::unique_ptr<Window> GetImplNative(Application* pApp)
	{
		return GetImplGLFW(pApp);
	}

	virtual ~Window() = default;

	Window(const Window&) = delete;

	Window& operator=(const Window&) = delete;

	// Actually create a window.
	virtual Result Create(const char* title) { return SUCCESS; }

	// Signal an intent to quit.
	virtual void Quit() { mQuit = true; }

	// Destory the window.
	virtual Result Destroy() { return SUCCESS; }

	virtual bool   IsRunning() const { return !mQuit; }
	virtual Result Resize(const WindowSize&) { return SUCCESS; }
	virtual void   ProcessEvent() {}
	virtual void* NativeHandle() { return nullptr; }

	virtual WindowSize Size() const;

	WindowState GetState() const { return mState; }
	bool        IsRestored() const { return (GetState() == WINDOW_STATE_RESTORED); }
	bool        IsIconified() const { return (GetState() == WINDOW_STATE_ICONIFIED); }
	bool        IsMaximized() const { return (GetState() == WINDOW_STATE_MAXIMIZED); }

	virtual void FillSurfaceInfo(SurfaceCreateInfo*) const {}

protected:
	Window(Application*);

	Application* App() const { return mApp; }

private:
	Application* mApp;

	bool        mQuit = false;
	WindowState mState = WINDOW_STATE_RESTORED;

	static std::unique_ptr<Window> GetImplGLFW(Application*);

	friend class Application;
	void SetState(WindowState state) { mState = state; }
};

#pragma endregion
