#include "stdafx.h"
#include "Application.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif

Application* thisApplication{ nullptr };

auto windowClassName = L"window class";

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (msg)
	{
	case WM_DESTROY:
		thisApplication->Exit();
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

Application::Application()
{
	thisApplication = this;
}

void Application::Run()
{
	Timer timer{};
	double accumulator{};

	uint64_t frameTimeCurrent = timer.Microseconds();
	uint64_t frameTimeLast = frameTimeCurrent;

	constexpr const double microsecondsToSeconds = 1.0 / 1000000.0;
	constexpr const double fixedFramerate = 60.0;
	constexpr const double fixedDeltaTimeMicroseconds = (1.0 / fixedFramerate) / microsecondsToSeconds;
	constexpr const double fixedDeltaTimeSeconds = fixedDeltaTimeMicroseconds * microsecondsToSeconds;

	if (start())
	{
		while (!m_requestedExit)
		{
			frameTimeCurrent = timer.Microseconds();
			const uint64_t frameTimeDelta = frameTimeCurrent - frameTimeLast;
			const double variableDeltaTimeSeconds = frameTimeDelta * microsecondsToSeconds;
			frameTimeLast = frameTimeCurrent;

			if (frameTimeDelta)
				m_framerateArray[m_framerateTick] = 1000000.0 / frameTimeDelta;
			m_framerateTick = (m_framerateTick + 1) % m_framerateArray.size();

			accumulator += frameTimeDelta;
			while (accumulator >= fixedDeltaTimeMicroseconds)
			{
				fixedUpdate((float)fixedDeltaTimeSeconds);
				accumulator -= fixedDeltaTimeMicroseconds;
			}
			update((float)variableDeltaTimeSeconds);
		}
	}
	shutdown();
}

void Application::Print(const std::string& text)
{
	puts(text.c_str());
}

void Application::Warning(const std::string& text)
{
	Print("WARNING: " + text);
}

void Application::Error(const std::string& text)
{
	Print("ERROR: " + text);
}

void Application::Fatal(const std::string& text)
{
	Print(("FATAL: " + text).c_str());
	m_requestedExit = true;
}

float Application::GetFramerate() const
{
	float framerate{ 0.0f };
	for (auto value : m_framerateArray)
		framerate += value;
	return framerate / m_framerateArray.size();
}

uint32_t Application::GetWindowWidth() const
{
	return m_windowWidth;
}

uint32_t Application::GetWindowHeight() const
{
	return m_windowHeight;
}

std::optional<std::vector<char>> Application::ReadFile(const std::filesystem::path& filename)
{
	std::ifstream file{ filename, std::ios::ate | std::ios::binary };
	if (!file.is_open()) 
	{
		Error("Failed to open file: " + filename.string());
		return std::nullopt;
	}

	const size_t fileSize = static_cast<size_t>(file.tellg());

	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), static_cast<std::streamsize>(fileSize));
	file.close();

	return buffer;
}

void Application::Exit()
{
	m_requestedExit = true;
}

bool Application::start()
{
	m_requestedExit = false;

	auto appCreateInfo = GetConfig();

	if (!createWindow(appCreateInfo))
		return false;

	return Start();
}

bool Application::createWindow(const ApplicationCreateInfo& createInfo)
{
	m_handleInstance = GetModuleHandle(nullptr);

	WNDCLASSEX windowClassInfo{ .cbSize = sizeof(WNDCLASSEX) };
	windowClassInfo.style = CS_HREDRAW | CS_VREDRAW;
	windowClassInfo.lpfnWndProc = WndProc;
	windowClassInfo.hInstance = m_handleInstance;
	windowClassInfo.hIcon = LoadIcon(m_handleInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	windowClassInfo.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClassInfo.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowClassInfo.lpszClassName = windowClassName;
	windowClassInfo.hIconSm = LoadIcon(m_handleInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	RegisterClassEx(&windowClassInfo);

	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	const int windowLeft = screenWidth / 2 - createInfo.window.width / 2;
	const int windowTop = screenHeight / 2 - createInfo.window.height / 2;

	m_hwnd = CreateWindow(windowClassName, L"Game", WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, windowLeft, windowTop, createInfo.window.width, createInfo.window.height, nullptr, nullptr, m_handleInstance, nullptr);
	ShowWindow(m_hwnd, SW_SHOW);
	SetForegroundWindow(m_hwnd);
	SetFocus(m_hwnd);

	RECT rect;
	GetClientRect(m_hwnd, &rect);
	m_windowWidth = rect.right - rect.left;
	m_windowHeight = rect.bottom - rect.top;

	return true;
}

void Application::shutdown()
{
	Shutdown();
	DestroyWindow(m_hwnd);
}

void Application::fixedUpdate(float deltaTime)
{
	FixedUpdate(deltaTime);
}

void Application::beginFrame()
{
}

void Application::present()
{
}

void Application::pollEvent()
{
	while (PeekMessage(&m_msg, m_hwnd, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&m_msg);
		DispatchMessage(&m_msg);
	}
}

void Application::update(float deltaTime)
{
	Update(deltaTime);

	if (!m_minimized)
	{
		beginFrame();
		Frame(deltaTime);
		present();
	}

	pollEvent();
}