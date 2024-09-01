#pragma once

#pragma region Window

class EngineApplication;

struct WindowCreateInfo final
{
	std::string_view title = "Game";
	int width = 1600;
	int height = 900;

	bool maximize = false;
	bool decorate = true;
	bool vsync = true;
};

class Window final
{
	friend class WindowEvents;
	friend class Input;
public:
	Window(EngineApplication& engine);

	[[nodiscard]] bool Setup(const WindowCreateInfo& createInfo);
	void Shutdown();

	[[nodiscard]] bool ShouldClose() const;
	void Update();

	[[nodiscard]] GLFWwindow* GetWindow();
	[[nodiscard]] uint32_t GetWidth() const;
	[[nodiscard]] uint32_t GetHeight() const;

private:
	EngineApplication& m_engine;
	GLFWwindow* m_window = nullptr;
	uint32_t m_width = 0;
	uint32_t m_height = 0;
	bool IsWindowResize = true; // TODO: убрать
};

#pragma endregion

#pragma region Key Code

enum class Button
{
	Left = GLFW_MOUSE_BUTTON_LEFT,
	Right = GLFW_MOUSE_BUTTON_RIGHT,
	Middle = GLFW_MOUSE_BUTTON_MIDDLE,
};

enum class CursorMode
{
	Disabled,
	Hidden,
	Normal
};

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

struct KeyState final
{
	float timeDown = FLT_MAX;
	bool  down = false;
};

[[nodiscard]] const char* GetKeyCodeString(KeyCode code);

#pragma endregion

#pragma region Input

class Input final
{
	friend class WindowEvents;
public:

	Input(EngineApplication& engine);

	[[nodiscard]] bool Setup();
	void Shutdown();

	void Update();

	// Keyboard
	bool IsPressed(int key); // TODO: использовать кей код
	float GetKeyAxis(int posKey, int negKey);

	// Mouse
	[[nodiscard]] bool IsButtonDown(Button button);
	[[nodiscard]] const glm::ivec2 GetPosition();
	[[nodiscard]] const glm::ivec2 GetDeltaPosition();
	void SetPosition(const glm::ivec2& position);

	void SetCursorMode(CursorMode mode);


private:
	EngineApplication& m_engine;

	struct
	{
		glm::ivec2 position{};
		glm::ivec2 lastPosition{};
		glm::ivec2 delta{};
	} m_mouseState; // TODO: перенести в класс input
};

#pragma endregion
