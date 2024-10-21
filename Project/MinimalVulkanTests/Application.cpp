#include "stdafx.h"
#include "Application.h"

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
			const double variableDeltaTimeSeconds = static_cast<double>(frameTimeDelta) * microsecondsToSeconds;
			frameTimeLast = frameTimeCurrent;

			if (frameTimeDelta)
				m_framerateArray[m_framerateTick] = 1000000.0 / static_cast<double>(frameTimeDelta);
			m_framerateTick = (m_framerateTick + 1) % m_framerateArray.size();

			accumulator += static_cast<double>(frameTimeDelta);
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
	double framerate{ 0.0 };
	for (auto value : m_framerateArray)
		framerate += value;
	return static_cast<float>(framerate / static_cast<double>(m_framerateArray.size()));
}

uint32_t Application::GetWindowWidth() const
{
	return m_windowWidth;
}

uint32_t Application::GetWindowHeight() const
{
	return m_windowHeight;
}

std::string Application::GetExtension(const std::string& uri)
{
	auto dotPos = uri.find_last_of('.');
	if (dotPos == std::string::npos)
	{
		Error("Uri has no extension");
		return "";
	}
	return uri.substr(dotPos + 1);
}

std::string Application::ReadFileString(const std::filesystem::path& path)
{
	auto bin = ReadFileBinary(path);
	return { bin.begin(), bin.end() };
}

std::vector<uint8_t> Application::ReadChunk(const std::filesystem::path& path, size_t offset, size_t count)
{
	std::ifstream file{ path, std::ios::binary | std::ios::ate };
	if (!file.is_open())
	{
		Error("Failed to open file: " + path.string());
		return {};
	}
	const size_t size = static_cast<size_t>(file.tellg());

	if (offset + count > size) return {};

	std::vector<uint8_t> data(count);

	// read file contents
	file.seekg(offset, std::ios::beg);
	file.read(reinterpret_cast<char*>(data.data()), count);
	file.close();

	return data;
}

std::vector<uint8_t> Application::ReadFileBinary(const std::filesystem::path& path)
{
	std::error_code ec;
	auto sizeFile = std::filesystem::file_size(path, ec);
	if (ec) { return {}; }
	return ReadChunk(path, 0, sizeFile);
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
	windowClassInfo.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClassInfo.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	windowClassInfo.lpszClassName = windowClassName;
	RegisterClassEx(&windowClassInfo);

	const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	const int windowLeft = screenWidth / 2 - (int)createInfo.window.width / 2;
	const int windowTop = screenHeight / 2 - (int)createInfo.window.height / 2;

	m_hwnd = CreateWindow(windowClassName, L"Game", 
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, 
		windowLeft, windowTop, 
		(int)createInfo.window.width, (int)createInfo.window.height,
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