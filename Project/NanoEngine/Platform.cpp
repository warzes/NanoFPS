#include "stdafx.h"
#include "Core.h"
#include "Platform.h"
#include "Application.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#	if defined(_WIN32)
#		pragma comment( lib, "Shcore.lib" )
#	endif
#endif

#pragma region Window

Window* currentWindow = nullptr;
Input* currentInput = nullptr;
extern EngineApplication* thisEngineApplication;

class WindowEvents final
{
public:
	static void MoveCallback([[maybe_unused]] GLFWwindow* window, int eventX, int eventY)
	{
		thisEngineApplication->m_app->Move(static_cast<int32_t>(eventX), static_cast<int32_t>(eventY));
	}

	static void framesizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height) noexcept // TODO: переделать
	{
		width = std::max(width, 1);
		height = std::max(height, 1);

		if (currentWindow->m_width != static_cast<uint32_t>(width) || currentWindow->m_height != static_cast<uint32_t>(height))
			currentWindow->IsWindowResize = true;

		currentWindow->m_width = static_cast<uint32_t>(width);
		currentWindow->m_height = static_cast<uint32_t>(height);
	}

	static void mouseCallback([[maybe_unused]] GLFWwindow* window, double xPosIn, double yPosIn) noexcept // TODO: переделать
	{
		currentInput->m_mouseState.position = glm::ivec2{ xPosIn,yPosIn };
	}
};

Window::Window(EngineApplication& engine)
	: m_engine(engine)
{
}

bool Window::Setup(const WindowCreateInfo& createInfo)
{
	glfwSetErrorCallback([](int, const char* desc) noexcept { Fatal("GLFW error: " + std::string(desc)); });

	if (glfwInit() != GLFW_TRUE)
	{
		Fatal("Failed to initialize GLFW");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_MAXIMIZED, createInfo.maximize);
	glfwWindowHint(GLFW_DECORATED, createInfo.decorate);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	// TODO: в примерах кроноса есть как сделать фулскрины

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor == nullptr)
	{
		Fatal("No Monitor detected");
		return false;
	}
	const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

	m_window = glfwCreateWindow(createInfo.width, createInfo.height, createInfo.title.data(), nullptr, nullptr);
	if (!m_window)
	{
		Fatal("Failed to create GLFW window.");
		return false;
	}

	currentWindow = this;

	int xSize{};
	int ySize{};
	glfwGetFramebufferSize(m_window, &xSize, &ySize);
	m_width = static_cast<uint32_t>(xSize);
	m_height = static_cast<uint32_t>(ySize);

	int monitorLeft{};
	int monitorTop{};
	glfwGetMonitorPos(monitor, &monitorLeft, &monitorTop);

	glfwSetWindowPos(m_window,
		videoMode->width / 2 - static_cast<int>(m_width) / 2 + monitorLeft,
		videoMode->height / 2 - static_cast<int>(m_height) / 2 + monitorTop);

	glfwSetInputMode(m_window, GLFW_STICKY_KEYS, GLFW_TRUE); // сохраняет событие клавиши до его опроса через glfwGetKey()
	glfwSetInputMode(m_window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE); // сохраняет событие кнопки мыши до его опроса через glfwGetMouseButton()

	glfwSetWindowPosCallback(m_window, WindowEvents::MoveCallback);

	glfwSetCursorPosCallback(m_window, WindowEvents::mouseCallback);
	//glfwSetCursorEnterCallback(Engine.window, cursorEnterCallback);
	glfwSetFramebufferSizeCallback(m_window, WindowEvents::framesizeCallback);

	return true;
}

void Window::Shutdown()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

bool Window::ShouldClose() const
{
	return glfwWindowShouldClose(m_window) == GLFW_TRUE;
}

void Window::Update()
{
	IsWindowResize = false;
	glfwPollEvents();
}

GLFWwindow* Window::GetWindow()
{
	return m_window;
}

uint32_t Window::GetWidth() const
{
	return m_width;
}

uint32_t Window::GetHeight() const
{
	return m_height;
}

#pragma endregion

#pragma region Key Code

static std::map<uint32_t, const char*> sKeyCodeString =
{
	{KEY_UNDEFINED,        "KEY_UNDEFINED"         },
	{KEY_SPACE,            "KEY_SPACE"             },
	{KEY_APOSTROPHE,       "KEY_APOSTROPHE"        },
	{KEY_COMMA,            "KEY_COMMA"             },
	{KEY_MINUS,            "KEY_MINUS"             },
	{KEY_PERIOD,           "KEY_PERIOD"            },
	{KEY_SLASH,            "KEY_SLASH"             },
	{KEY_0,                "KEY_0"                 },
	{KEY_1,                "KEY_1"                 },
	{KEY_2,                "KEY_2"                 },
	{KEY_3,                "KEY_3"                 },
	{KEY_4,                "KEY_4"                 },
	{KEY_5,                "KEY_5"                 },
	{KEY_6,                "KEY_6"                 },
	{KEY_7,                "KEY_7"                 },
	{KEY_8,                "KEY_8"                 },
	{KEY_9,                "KEY_9"                 },
	{KEY_SEMICOLON,        "KEY_SEMICOLON"         },
	{KEY_EQUAL,            "KEY_EQUAL"             },
	{KEY_A,                "KEY_A"                 },
	{KEY_B,                "KEY_B"                 },
	{KEY_C,                "KEY_C"                 },
	{KEY_D,                "KEY_D"                 },
	{KEY_E,                "KEY_E"                 },
	{KEY_F,                "KEY_F"                 },
	{KEY_G,                "KEY_G"                 },
	{KEY_H,                "KEY_H"                 },
	{KEY_I,                "KEY_I"                 },
	{KEY_J,                "KEY_J"                 },
	{KEY_K,                "KEY_K"                 },
	{KEY_L,                "KEY_L"                 },
	{KEY_M,                "KEY_M"                 },
	{KEY_N,                "KEY_N"                 },
	{KEY_O,                "KEY_O"                 },
	{KEY_P,                "KEY_P"                 },
	{KEY_Q,                "KEY_Q"                 },
	{KEY_R,                "KEY_R"                 },
	{KEY_S,                "KEY_S"                 },
	{KEY_T,                "KEY_T"                 },
	{KEY_U,                "KEY_U"                 },
	{KEY_V,                "KEY_V"                 },
	{KEY_W,                "KEY_W"                 },
	{KEY_X,                "KEY_X"                 },
	{KEY_Y,                "KEY_Y"                 },
	{KEY_Z,                "KEY_Z"                 },
	{KEY_LEFT_BRACKET,     "KEY_LEFT_BRACKET"      },
	{KEY_BACKSLASH,        "KEY_BACKSLASH"         },
	{KEY_RIGHT_BRACKET,    "KEY_RIGHT_BRACKET"     },
	{KEY_GRAVE_ACCENT,     "KEY_GRAVE_ACCENT"      },
	{KEY_WORLD_1,          "KEY_WORLD_1"           },
	{KEY_WORLD_2,          "KEY_WORLD_2"           },
	{KEY_ESCAPE,           "KEY_ESCAPE"            },
	{KEY_ENTER,            "KEY_ENTER"             },
	{KEY_TAB,              "KEY_TAB"               },
	{KEY_BACKSPACE,        "KEY_BACKSPACE"         },
	{KEY_INSERT,           "KEY_INSERT"            },
	{KEY_DELETE,           "KEY_DELETE"            },
	{KEY_RIGHT,            "KEY_RIGHT"             },
	{KEY_LEFT,             "KEY_LEFT"              },
	{KEY_DOWN,             "KEY_DOWN"              },
	{KEY_UP,               "KEY_UP"                },
	{KEY_PAGE_UP,          "KEY_PAGE_UP"           },
	{KEY_PAGE_DOWN,        "KEY_PAGE_DOWN"         },
	{KEY_HOME,             "KEY_HOME"              },
	{KEY_END,              "KEY_END"               },
	{KEY_CAPS_LOCK,        "KEY_CAPS_LOCK"         },
	{KEY_SCROLL_LOCK,      "KEY_SCROLL_LOCK"       },
	{KEY_NUM_LOCK,         "KEY_NUM_LOCK"          },
	{KEY_PRINT_SCREEN,     "KEY_PRINT_SCREEN"      },
	{KEY_PAUSE,            "KEY_PAUSE"             },
	{KEY_F1,               "KEY_F1"                },
	{KEY_F2,               "KEY_F2"                },
	{KEY_F3,               "KEY_F3"                },
	{KEY_F4,               "KEY_F4"                },
	{KEY_F5,               "KEY_F5"                },
	{KEY_F6,               "KEY_F6"                },
	{KEY_F7,               "KEY_F7"                },
	{KEY_F8,               "KEY_F8"                },
	{KEY_F9,               "KEY_F9"                },
	{KEY_F10,              "KEY_F10"               },
	{KEY_F11,              "KEY_F11"               },
	{KEY_F12,              "KEY_F12"               },
	{KEY_F13,              "KEY_F13"               },
	{KEY_F14,              "KEY_F14"               },
	{KEY_F15,              "KEY_F15"               },
	{KEY_F16,              "KEY_F16"               },
	{KEY_F17,              "KEY_F17"               },
	{KEY_F18,              "KEY_F18"               },
	{KEY_F19,              "KEY_F19"               },
	{KEY_F20,              "KEY_F20"               },
	{KEY_F21,              "KEY_F21"               },
	{KEY_F22,              "KEY_F22"               },
	{KEY_F23,              "KEY_F23"               },
	{KEY_F24,              "KEY_F24"               },
	{KEY_F25,              "KEY_F25"               },
	{KEY_KEY_PAD_0,        "KEY_KEY_PAD_0"         },
	{KEY_KEY_PAD_1,        "KEY_KEY_PAD_1"         },
	{KEY_KEY_PAD_2,        "KEY_KEY_PAD_2"         },
	{KEY_KEY_PAD_3,        "KEY_KEY_PAD_3"         },
	{KEY_KEY_PAD_4,        "KEY_KEY_PAD_4"         },
	{KEY_KEY_PAD_5,        "KEY_KEY_PAD_5"         },
	{KEY_KEY_PAD_6,        "KEY_KEY_PAD_6"         },
	{KEY_KEY_PAD_7,        "KEY_KEY_PAD_7"         },
	{KEY_KEY_PAD_8,        "KEY_KEY_PAD_8"         },
	{KEY_KEY_PAD_9,        "KEY_KEY_PAD_9"         },
	{KEY_KEY_PAD_DECIMAL,  "KEY_KEY_PAD_DECIMAL"   },
	{KEY_KEY_PAD_DIVIDE,   "KEY_KEY_PAD_DIVIDE"    },
	{KEY_KEY_PAD_MULTIPLY, "KEY_KEY_PAD_MULTIPLY"  },
	{KEY_KEY_PAD_SUBTRACT, "KEY_KEY_PAD_SUBTRACT"  },
	{KEY_KEY_PAD_ADD,      "KEY_KEY_PAD_ADD"       },
	{KEY_KEY_PAD_ENTER,    "KEY_KEY_PAD_ENTER"     },
	{KEY_KEY_PAD_EQUAL,    "KEY_KEY_PAD_EQUAL"     },
	{KEY_LEFT_SHIFT,       "KEY_LEFT_SHIFT"        },
	{KEY_LEFT_CONTROL,     "KEY_LEFT_CONTROL"      },
	{KEY_LEFT_ALT,         "KEY_LEFT_ALT"          },
	{KEY_LEFT_SUPER,       "KEY_LEFT_SUPER"        },
	{KEY_RIGHT_SHIFT,      "KEY_RIGHT_SHIFT"       },
	{KEY_RIGHT_CONTROL,    "KEY_RIGHT_CONTROL"     },
	{KEY_RIGHT_ALT,        "KEY_RIGHT_ALT"         },
	{KEY_RIGHT_SUPER,      "KEY_RIGHT_SUPER"       },
	{KEY_MENU,             "KEY_MENU"              },
};

const char* GetKeyCodeString(KeyCode code)
{
	if ((code < KEY_RANGE_FIRST) || (code >= KEY_RANGE_LAST))
		return sKeyCodeString[0];
	return sKeyCodeString[static_cast<uint32_t>(code)];
}

#pragma endregion

#pragma region Input

Input::Input(EngineApplication& engine)
	: m_engine(engine)
{
	currentInput = this;
}

bool Input::Setup()
{
	m_mouseState.lastPosition = m_mouseState.position = GetPosition();
	m_mouseState.delta = glm::ivec2(0);

	return true;
}

void Input::Shutdown()
{
}

void Input::Update()
{
	// TODO: ошибка при движении камеры мышью - ее почему-то дергает в первый кадр. а этот блок убирает дерганье но оставляет поворот не в ту сторону. разобраться
	//glm::dvec2 mousePosition;
	//glfwGetCursorPos(Window::GetWindow(), &mousePosition.x, &mousePosition.y);
	//MouseState.position = mousePosition;

	m_mouseState.delta = m_mouseState.position - m_mouseState.lastPosition;
	m_mouseState.lastPosition = m_mouseState.position;
}

bool Input::IsPressed(int key)
{
	return glfwGetKey(m_engine.GetWindow().m_window, key) == GLFW_PRESS;
}

float Input::GetKeyAxis(int posKey, int negKey)
{
	float value = 0.0f;
	if (glfwGetKey(m_engine.GetWindow().m_window, posKey)) value += 1.0f;
	if (glfwGetKey(m_engine.GetWindow().m_window, negKey)) value -= 1.0f;
	return value;
}

bool Input::IsButtonDown(MouseButton button)
{
	return glfwGetMouseButton(m_engine.GetWindow().m_window, static_cast<int>(button)) == GLFW_PRESS;
}

const glm::ivec2 Input::GetPosition()
{
	return m_mouseState.position;
}

const glm::ivec2 Input::GetDeltaPosition()
{
	return m_mouseState.delta;
}

void Input::SetPosition(const glm::ivec2& position)
{
	glfwSetCursorPos(m_engine.GetWindow().m_window, position.x, position.y);
}

void Input::SetCursorMode(CursorMode mode)
{
	int mod = GLFW_CURSOR_NORMAL;

	if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_DISABLED;
	else if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_HIDDEN;

	glfwSetInputMode(m_engine.GetWindow().m_window, GLFW_CURSOR, mod);
}

#pragma endregion