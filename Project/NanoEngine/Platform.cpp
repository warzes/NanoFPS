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

//=============================================================================
#pragma region [ Window ]

Window* currentWindow = nullptr;
Input* currentInput = nullptr;
extern EngineApplication* thisEngineApplication;

static std::map<int32_t, int32_t> sKeyCodeMap = {
	{ GLFW_KEY_SPACE,           KEY_SPACE            },
	{ GLFW_KEY_APOSTROPHE,      KEY_APOSTROPHE       },
	{ GLFW_KEY_COMMA,           KEY_COMMA            },
	{ GLFW_KEY_MINUS,           KEY_MINUS            },
	{ GLFW_KEY_PERIOD,          KEY_PERIOD           },
	{ GLFW_KEY_SLASH,           KEY_SLASH            },
	{ GLFW_KEY_0,               KEY_0                },
	{ GLFW_KEY_1,               KEY_1                },
	{ GLFW_KEY_2,               KEY_2                },
	{ GLFW_KEY_3,               KEY_3                },
	{ GLFW_KEY_4,               KEY_4                },
	{ GLFW_KEY_5,               KEY_5                },
	{ GLFW_KEY_6,               KEY_6                },
	{ GLFW_KEY_7,               KEY_7                },
	{ GLFW_KEY_8,               KEY_8                },
	{ GLFW_KEY_9,               KEY_9                },
	{ GLFW_KEY_SEMICOLON,       KEY_SEMICOLON        },
	{ GLFW_KEY_EQUAL,           KEY_EQUAL            },
	{ GLFW_KEY_A,               KEY_A                },
	{ GLFW_KEY_B,               KEY_B                },
	{ GLFW_KEY_C,               KEY_C                },
	{ GLFW_KEY_D,               KEY_D                },
	{ GLFW_KEY_E,               KEY_E                },
	{ GLFW_KEY_F,               KEY_F                },
	{ GLFW_KEY_G,               KEY_G                },
	{ GLFW_KEY_H,               KEY_H                },
	{ GLFW_KEY_I,               KEY_I                },
	{ GLFW_KEY_J,               KEY_J                },
	{ GLFW_KEY_K,               KEY_K                },
	{ GLFW_KEY_L,               KEY_L                },
	{ GLFW_KEY_M,               KEY_M                },
	{ GLFW_KEY_N,               KEY_N                },
	{ GLFW_KEY_O,               KEY_O                },
	{ GLFW_KEY_P,               KEY_P                },
	{ GLFW_KEY_Q,               KEY_Q                },
	{ GLFW_KEY_R,               KEY_R                },
	{ GLFW_KEY_S,               KEY_S                },
	{ GLFW_KEY_T,               KEY_T                },
	{ GLFW_KEY_U,               KEY_U                },
	{ GLFW_KEY_V,               KEY_V                },
	{ GLFW_KEY_W,               KEY_W                },
	{ GLFW_KEY_X,               KEY_X                },
	{ GLFW_KEY_Y,               KEY_Y                },
	{ GLFW_KEY_Z,               KEY_Z                },
	{ GLFW_KEY_LEFT_BRACKET,    KEY_LEFT_BRACKET     },
	{ GLFW_KEY_BACKSLASH,       KEY_BACKSLASH        },
	{ GLFW_KEY_RIGHT_BRACKET,   KEY_RIGHT_BRACKET    },
	{ GLFW_KEY_GRAVE_ACCENT,    KEY_GRAVE_ACCENT     },
	{ GLFW_KEY_WORLD_1,         KEY_WORLD_1          },
	{ GLFW_KEY_WORLD_2,         KEY_WORLD_2          },
	{ GLFW_KEY_ESCAPE,          KEY_ESCAPE           },
	{ GLFW_KEY_ENTER,           KEY_ENTER            },
	{ GLFW_KEY_TAB,             KEY_TAB              },
	{ GLFW_KEY_BACKSPACE,       KEY_BACKSPACE        },
	{ GLFW_KEY_INSERT,          KEY_INSERT           },
	{ GLFW_KEY_DELETE,          KEY_DELETE           },
	{ GLFW_KEY_RIGHT,           KEY_RIGHT            },
	{ GLFW_KEY_LEFT,            KEY_LEFT             },
	{ GLFW_KEY_DOWN,            KEY_DOWN             },
	{ GLFW_KEY_UP,              KEY_UP               },
	{ GLFW_KEY_PAGE_UP,         KEY_PAGE_UP          },
	{ GLFW_KEY_PAGE_DOWN,       KEY_PAGE_DOWN        },
	{ GLFW_KEY_HOME,            KEY_HOME             },
	{ GLFW_KEY_END,             KEY_END              },
	{ GLFW_KEY_CAPS_LOCK,       KEY_CAPS_LOCK        },
	{ GLFW_KEY_SCROLL_LOCK,     KEY_SCROLL_LOCK      },
	{ GLFW_KEY_NUM_LOCK,        KEY_NUM_LOCK         },
	{ GLFW_KEY_PRINT_SCREEN,    KEY_PRINT_SCREEN     },
	{ GLFW_KEY_PAUSE,           KEY_PAUSE            },
	{ GLFW_KEY_F1,              KEY_F1               },
	{ GLFW_KEY_F2,              KEY_F2               },
	{ GLFW_KEY_F3,              KEY_F3               },
	{ GLFW_KEY_F4,              KEY_F4               },
	{ GLFW_KEY_F5,              KEY_F5               },
	{ GLFW_KEY_F6,              KEY_F6               },
	{ GLFW_KEY_F7,              KEY_F7               },
	{ GLFW_KEY_F8,              KEY_F8               },
	{ GLFW_KEY_F9,              KEY_F9               },
	{ GLFW_KEY_F10,             KEY_F10              },
	{ GLFW_KEY_F11,             KEY_F11              },
	{ GLFW_KEY_F12,             KEY_F12              },
	{ GLFW_KEY_F13,             KEY_F13              },
	{ GLFW_KEY_F14,             KEY_F14              },
	{ GLFW_KEY_F15,             KEY_F15              },
	{ GLFW_KEY_F16,             KEY_F16              },
	{ GLFW_KEY_F17,             KEY_F17              },
	{ GLFW_KEY_F18,             KEY_F18              },
	{ GLFW_KEY_F19,             KEY_F19              },
	{ GLFW_KEY_F20,             KEY_F20              },
	{ GLFW_KEY_F21,             KEY_F21              },
	{ GLFW_KEY_F22,             KEY_F22              },
	{ GLFW_KEY_F23,             KEY_F23              },
	{ GLFW_KEY_F24,             KEY_F24              },
	{ GLFW_KEY_F25,             KEY_F25              },
	{ GLFW_KEY_KP_0,            KEY_KEY_PAD_0        },
	{ GLFW_KEY_KP_1,            KEY_KEY_PAD_1        },
	{ GLFW_KEY_KP_2,            KEY_KEY_PAD_2        },
	{ GLFW_KEY_KP_3,            KEY_KEY_PAD_3        },
	{ GLFW_KEY_KP_4,            KEY_KEY_PAD_4        },
	{ GLFW_KEY_KP_5,            KEY_KEY_PAD_5        },
	{ GLFW_KEY_KP_6,            KEY_KEY_PAD_6        },
	{ GLFW_KEY_KP_7,            KEY_KEY_PAD_7        },
	{ GLFW_KEY_KP_8,            KEY_KEY_PAD_8        },
	{ GLFW_KEY_KP_9,            KEY_KEY_PAD_9        },
	{ GLFW_KEY_KP_DECIMAL,      KEY_KEY_PAD_DECIMAL  },
	{ GLFW_KEY_KP_DIVIDE,       KEY_KEY_PAD_DIVIDE   },
	{ GLFW_KEY_KP_MULTIPLY,     KEY_KEY_PAD_MULTIPLY },
	{ GLFW_KEY_KP_SUBTRACT,     KEY_KEY_PAD_SUBTRACT },
	{ GLFW_KEY_KP_ADD,          KEY_KEY_PAD_ADD      },
	{ GLFW_KEY_KP_ENTER,        KEY_KEY_PAD_ENTER    },
	{ GLFW_KEY_KP_EQUAL,        KEY_KEY_PAD_EQUAL    },
	{ GLFW_KEY_LEFT_SHIFT,      KEY_LEFT_SHIFT       },
	{ GLFW_KEY_LEFT_CONTROL,    KEY_LEFT_CONTROL     },
	{ GLFW_KEY_LEFT_ALT,        KEY_LEFT_ALT         },
	{ GLFW_KEY_LEFT_SUPER,      KEY_LEFT_SUPER       },
	{ GLFW_KEY_RIGHT_SHIFT,     KEY_RIGHT_SHIFT      },
	{ GLFW_KEY_RIGHT_CONTROL,   KEY_RIGHT_CONTROL    },
	{ GLFW_KEY_RIGHT_ALT,       KEY_RIGHT_ALT        },
	{ GLFW_KEY_RIGHT_SUPER,     KEY_RIGHT_SUPER      },
	{ GLFW_KEY_MENU,            KEY_MENU             },
};

class WindowEvents final
{
public:
	static void FrameSizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height) noexcept
	{
		width = std::max(width, 1);
		height = std::max(height, 1);

		if (currentWindow->m_width != static_cast<uint32_t>(width) || currentWindow->m_height != static_cast<uint32_t>(height))
		{
			currentWindow->m_width = static_cast<uint32_t>(width);
			currentWindow->m_height = static_cast<uint32_t>(height);

			thisEngineApplication->resizeCallback(currentWindow->m_width, currentWindow->m_height);
		}
	}

	static void IconifyCallback([[maybe_unused]] GLFWwindow* window, int iconified) noexcept
	{
		currentWindow->m_state = (iconified ? WINDOW_STATE_ICONIFIED : WINDOW_STATE_RESTORED);
		thisEngineApplication->windowIconifyCallback((iconified == GLFW_TRUE) ? true : false);
	}

	static void MaximizeCallback([[maybe_unused]] GLFWwindow* window, int maximized) noexcept
	{
		currentWindow->m_state = (maximized ? WINDOW_STATE_MAXIMIZED : WINDOW_STATE_RESTORED);
		thisEngineApplication->windowMaximizeCallback((maximized == GLFW_TRUE) ? true : false);
	}

	static void MouseButtonCallback(GLFWwindow* window, int event_button, int event_action, int event_mods) noexcept
	{
		MouseButton buttons{ MouseButton::None };
		if (event_button == GLFW_MOUSE_BUTTON_LEFT) buttons   = MouseButton::Left;
		if (event_button == GLFW_MOUSE_BUTTON_RIGHT) buttons  = MouseButton::Right;
		if (event_button == GLFW_MOUSE_BUTTON_MIDDLE) buttons = MouseButton::Middle;

		double event_x;
		double event_y;
		glfwGetCursorPos(window, &event_x, &event_y);

		if (event_action == GLFW_PRESS)
			thisEngineApplication->mouseDownCallback(static_cast<int32_t>(event_x), static_cast<int32_t>(event_y), buttons);
		else if (event_action == GLFW_RELEASE)
			thisEngineApplication->mouseUpCallback(static_cast<int32_t>(event_x), static_cast<int32_t>(event_y), buttons);

		ImGui_ImplGlfw_MouseButtonCallback(window, event_button, event_action, event_mods);
	}

	static void MouseMoveCallback(GLFWwindow* window, double event_x, double event_y) noexcept
	{
		MouseButton buttons{ MouseButton::None };
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) buttons   = MouseButton::Left;
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) buttons  = MouseButton::Right;
		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) buttons = MouseButton::Middle;
		thisEngineApplication->mouseMoveCallback(static_cast<int32_t>(event_x), static_cast<int32_t>(event_y), buttons);
	}

	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) noexcept
	{
		thisEngineApplication->scrollCallback(static_cast<float>(xoffset), static_cast<float>(yoffset));
		ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
	}

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) noexcept
	{
		bool hasKey = (sKeyCodeMap.find(key) != sKeyCodeMap.end());
		if (!hasKey) Warning("GLFW key not supported, key=" + std::to_string(key));

		if (hasKey) 
		{
			KeyCode appKey = static_cast<KeyCode>(sKeyCodeMap[key]);

			if (action == GLFW_PRESS) {
				thisEngineApplication->keyDownCallback(appKey);
			}
			else if (action == GLFW_RELEASE) {
				thisEngineApplication->keyUpCallback(appKey);
			}
		}

		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	}

	static void CharCallback(GLFWwindow* window, unsigned int c) noexcept
	{
		ImGui_ImplGlfw_CharCallback(window, c);
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

	glfwSetFramebufferSizeCallback(m_window, WindowEvents::FrameSizeCallback);
	glfwSetWindowIconifyCallback(m_window, WindowEvents::IconifyCallback);
	glfwSetWindowMaximizeCallback(m_window, WindowEvents::MaximizeCallback);
	glfwSetMouseButtonCallback(m_window, WindowEvents::MouseButtonCallback);
	glfwSetCursorPosCallback(m_window, WindowEvents::MouseMoveCallback);
	glfwSetScrollCallback(m_window, WindowEvents::ScrollCallback);
	glfwSetKeyCallback(m_window, WindowEvents::KeyCallback);
	glfwSetCharCallback(m_window, WindowEvents::CharCallback);

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

//=============================================================================
#pragma region [ Key Code ]

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

//=============================================================================
#pragma region [ Input ]

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