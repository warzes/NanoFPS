#pragma once

#include "VulkanCore.h"
#include "Timer.h"

struct WindowCreateInfo final
{
	uint32_t width{ 1600 };
	uint32_t height{ 900 };
	bool resize = true;
};

class WindowContext final
{
public:
	WindowContext();

	bool Setup(const WindowCreateInfo& createInfo);
	void Shutdown();

	void PollEvent();

	void Exit();

	bool IsRequestedExit() const;

	uint32_t GetWindowWidth() const;
	uint32_t GetWindowHeight() const;

	HWND GetWindowHWND() { return m_hwnd; }
	HINSTANCE GetWindowInstance() { return m_handleInstance; }
private:
	friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM) noexcept;

	HINSTANCE              m_handleInstance{ nullptr };
	HWND                   m_hwnd{ nullptr };
	MSG                    m_msg{};
	uint32_t               m_windowWidth{ 0 };
	uint32_t               m_windowHeight{ 0 };

	bool                   m_minimized{ false };
	bool                   m_requestedExit{ false };
};