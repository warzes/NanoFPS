#include "Base.h"
#include "Core.h"
#include "EngineWindow.h"

#pragma region Globals

namespace
{
	GLFWwindow* WindowContext = nullptr;
	uint32_t WindowWidth = 0;
	uint32_t WindowHeight = 0;
	bool IsWindowResize = true;

	struct
	{
		glm::ivec2 position{};
		glm::ivec2 lastPosition{};
		glm::ivec2 delta{};
	} MouseState;
}

#pragma endregion

#pragma region Window

static void framesizeCallback([[maybe_unused]] GLFWwindow* window, int width, int height) noexcept
{
	width = std::max(width, 1);
	height = std::max(height, 1);

	if (WindowWidth != static_cast<uint32_t>(width) || WindowHeight != static_cast<uint32_t>(height))
		IsWindowResize = true;

	WindowWidth = static_cast<uint32_t>(width);
	WindowHeight = static_cast<uint32_t>(height);
}

static void mouseCallback([[maybe_unused]] GLFWwindow* window, double xPosIn, double yPosIn) noexcept
{
	MouseState.position = glm::ivec2{ xPosIn,yPosIn };
}

bool Window::Create(const WindowCreateInfo& createInfo)
{
	glfwSetErrorCallback([](int, const char* desc) noexcept { Fatal("GLFW error: " + std::string(desc)); });

	if (!glfwInit())
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

	WindowContext = glfwCreateWindow(createInfo.width, createInfo.height, createInfo.title.data(), nullptr, nullptr);
	if (!WindowContext)
	{
		Fatal("Failed to create GLFW window.");
		return false;
	}

	int xSize{};
	int ySize{};
	glfwGetFramebufferSize(WindowContext, &xSize, &ySize);
	WindowWidth = static_cast<uint32_t>(xSize);
	WindowHeight = static_cast<uint32_t>(ySize);

	int monitorLeft{};
	int monitorTop{};
	glfwGetMonitorPos(monitor, &monitorLeft, &monitorTop);

	glfwSetWindowPos(WindowContext, 
		videoMode->width / 2 - static_cast<int>(WindowWidth) / 2 + monitorLeft, 
		videoMode->height / 2 - static_cast<int>(WindowHeight) / 2 + monitorTop);

	//glfwSwapInterval(createInfo.vsync ? 1 : 0);

	glfwSetInputMode(WindowContext, GLFW_STICKY_KEYS, GLFW_TRUE); // сохраняет событие клавиши до его опроса через glfwGetKey()
	glfwSetInputMode(WindowContext, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE); // сохраняет событие кнопки мыши до его опроса через glfwGetMouseButton()

	glfwSetCursorPosCallback(WindowContext, mouseCallback);
	//glfwSetCursorEnterCallback(Engine.window, cursorEnterCallback);
	glfwSetFramebufferSizeCallback(WindowContext, framesizeCallback);

	MouseState.lastPosition = MouseState.position = Mouse::GetPosition();
	MouseState.delta = glm::ivec2(0);

	return true;
}

void Window::Destroy()
{
	glfwDestroyWindow(WindowContext);
	glfwTerminate();
}

bool Window::ShouldClose()
{
	return glfwWindowShouldClose(WindowContext) == GLFW_TRUE;
}

void Window::Update()
{
	IsWindowResize = false;
	glfwPollEvents();

	// TODO: ошибка при движении камеры мышью - ее почему-то дергает в первый кадр. а этот блок убирает дерганье но оставляет поворот не в ту сторону. разобраться
	//glm::dvec2 mousePosition;
	//glfwGetCursorPos(Window::GetWindow(), &mousePosition.x, &mousePosition.y);
	//MouseState.position = mousePosition;

	MouseState.delta = MouseState.position - MouseState.lastPosition;
	MouseState.lastPosition = MouseState.position;
}

GLFWwindow* Window::GetWindow()
{
	return WindowContext;
}

uint32_t Window::GetWidth()
{
	return WindowWidth;
}

uint32_t Window::GetHeight()
{
	return WindowHeight;
}

bool Window::IsResize()
{
	return IsWindowResize;
}

#pragma endregion

//==============================================================================
// Input
//==============================================================================
#pragma region Input

bool Keyboard::IsPressed(int key)
{
	return glfwGetKey(WindowContext, key) == GLFW_PRESS;
}

float Keyboard::GetKeyAxis(int posKey, int negKey)
{
	float value = 0.0f;
	if (glfwGetKey(WindowContext, posKey)) value += 1.0f;
	if (glfwGetKey(WindowContext, negKey)) value -= 1.0f;
	return value;
}

bool Mouse::IsButtonDown(Button button)
{
	return glfwGetMouseButton(WindowContext, static_cast<int>(button)) == GLFW_PRESS;
}

const glm::ivec2 Mouse::GetPosition()
{
	return MouseState.position;
}

const glm::ivec2 Mouse::GetDeltaPosition()
{
	return MouseState.delta;
}

void Mouse::SetPosition(const glm::ivec2& position)
{
	glfwSetCursorPos(WindowContext, position.x, position.y);
}

void Mouse::SetCursorMode(CursorMode mode)
{
	int mod = GLFW_CURSOR_NORMAL;

	if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_DISABLED;
	else if (mode == CursorMode::Disabled) mod = GLFW_CURSOR_HIDDEN;

	glfwSetInputMode(WindowContext, GLFW_CURSOR, mod);
}

#pragma endregion