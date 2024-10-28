#include "WindowContext.h"

namespace
{
	auto windowClassName = L"window class";
	WindowContext* thisWindowContext = nullptr;

	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		switch (msg)
		{
		case WM_DESTROY:
			thisWindowContext->Exit();
			DestroyWindow(hWnd);
			PostQuitMessage(0);
			break;
		case WM_PAINT:
			ValidateRect(hWnd, nullptr);
			break;
		default:
			break;
		}
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

WindowContext::WindowContext()
{
	thisWindowContext = this;
}

bool WindowContext::Setup(const WindowCreateInfo& createInfo)
{
	m_handleInstance = GetModuleHandle(nullptr);

	WNDCLASSEX windowClassInfo{ .cbSize = sizeof(WNDCLASSEX) };
	windowClassInfo.style = CS_HREDRAW | CS_VREDRAW;
	windowClassInfo.lpfnWndProc = WndProc;
	windowClassInfo.hInstance = m_handleInstance;
	windowClassInfo.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClassInfo.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowClassInfo.lpszClassName = windowClassName;
	RegisterClassEx(&windowClassInfo);

	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	const int windowLeft = screenWidth / 2 - (int)createInfo.width / 2;
	const int windowTop = screenHeight / 2 - (int)createInfo.height / 2;

	m_hwnd = CreateWindow(windowClassName, L"Game",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		windowLeft, windowTop,
		(int)createInfo.width, (int)createInfo.height,
		nullptr, nullptr, m_handleInstance, nullptr);
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	m_windowWidth = static_cast<uint32_t>(rect.right - rect.left);
	m_windowHeight = static_cast<uint32_t>(rect.bottom - rect.top);

	return true;
}

void WindowContext::Shutdown()
{
	DestroyWindow(m_hwnd);
}

void WindowContext::PollEvent()
{
	while (PeekMessage(&m_msg, m_hwnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&m_msg);
		DispatchMessage(&m_msg);
	}
}

void WindowContext::Exit()
{
	m_requestedExit = true;
}

bool WindowContext::IsRequestedExit() const
{
	return m_requestedExit;
}

uint32_t WindowContext::GetWindowWidth() const
{
	return m_windowWidth;
}

uint32_t WindowContext::GetWindowHeight() const
{
	return m_windowHeight;
}
