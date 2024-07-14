#pragma once

#pragma region Window

struct WindowCreateInfo final
{
	std::string_view title = "Game";
	int width = 1600;
	int height = 900;

	bool maximize = false;
	bool decorate = true;
	bool vsync = true;
};

namespace Window
{
	bool Create(const WindowCreateInfo& createInfo);
	void Destroy();

	bool ShouldClose();
	void Update();
	void Swap();

	GLFWwindow* GetWindow();
	uint32_t GetWidth();
	uint32_t GetHeight();
	bool IsResize();
}

#pragma endregion

#pragma region Input

namespace Keyboard
{
	bool IsPressed(int key);
}

namespace Mouse
{
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

	bool IsPressed(Button button);
	glm::ivec2 GetPosition();
	glm::ivec2 GetDelta();
	void SetPosition(const glm::ivec2& position);

	void SetCursorMode(CursorMode mode);
}