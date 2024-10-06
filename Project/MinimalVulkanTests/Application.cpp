#include "stdafx.h"
#include "Application.h"

// TODO:
// - вариант с WinAPI

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif

Application* thisApplication{ nullptr };

void Print(const std::string& text)
{
	assert(thisApplication);
	thisApplication->Print(text);
}
void Warning(const std::string& text)
{
	assert(thisApplication);
	thisApplication->Warning(text);
}
void Error(const std::string& text)
{
	assert(thisApplication);
	thisApplication->Error(text);
}
void Fatal(const std::string& text)
{
	assert(thisApplication);
	thisApplication->Fatal(text);
}

Application::Application()
{
	thisApplication = this;
}

void Application::Run()
{
	if (init())
	{
		while (!shouldClose())
		{
			update();
			draw();
		}
	}
	close();
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
	m_isExit = true;
}

int Application::GetWindowWidth() const
{
	return m_windowWidth;
}

int Application::GetWindowHeight() const
{
	return m_windowHeight;
}

double Application::GetDeltaTime() const
{
	return m_deltaTime;
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
	m_isExit = true;
}

bool Application::init()
{
	auto config = GetConfig();

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(config.window.width, config.window.height, config.window.title.data(), nullptr, nullptr);

	glfwGetFramebufferSize(m_window, &m_windowWidth, &m_windowHeight);
	m_startTime = glfwGetTime();
	m_isExit = false;

	return Init();
}

void Application::close()
{
	Close();
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

void Application::update()
{
	glfwPollEvents();

	double currentTime = glfwGetTime();
	m_deltaTime = currentTime - m_startTime;
	m_startTime = currentTime;

	Update();
}

void Application::draw()
{
	Draw();
}

bool Application::shouldClose() const
{
	return glfwWindowShouldClose(m_window) || m_isExit;
}