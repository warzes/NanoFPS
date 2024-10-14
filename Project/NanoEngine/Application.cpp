#include "stdafx.h"
#include "Core.h"
#include "Application.h"

//=============================================================================
#pragma region [ Engine Application ]

EngineApplication* thisEngineApplication = nullptr;

void Print(const std::string& msg)
{
	assert(thisEngineApplication);
	thisEngineApplication->Print(msg);
}

void Warning(const std::string& msg)
{
	assert(thisEngineApplication);
	thisEngineApplication->Warning(msg);
}

void Error(const std::string& msg)
{
	assert(thisEngineApplication);
	thisEngineApplication->Error(msg);
}

void Fatal(const std::string& msg)
{
	assert(thisEngineApplication);
	thisEngineApplication->Fatal(msg);
}

EngineApplication::EngineApplication()
	: m_window(*this)
	, m_input(*this)
	, m_render(*this)
	, m_physics(*this)
{
	thisEngineApplication = this;
}

EngineApplication::~EngineApplication()
{
	m_physics.Shutdown();
	m_render.Shutdown();
	m_input.Shutdown();
	m_window.Shutdown();
	shutdownLog();
}

void EngineApplication::Run()
{
	assert(m_status == StatusApp::NonInit);

	// Init
	if (!setup())
		return;

	// Main Loop
	{
		while (m_status == StatusApp::Success)
		{
			if (m_window.ShouldClose()) break;

			float currentFrame = static_cast<float>(glfwGetTime());
			m_deltaTime = currentFrame - m_lastFrameTime;
			m_lastFrameTime = currentFrame;
			m_timeSinceLastTick += m_deltaTime;

			// Update
			m_input.ClearState();
			m_window.Update();
			m_input.Update();
			m_render.Update();

			// Fixed Update
			if (m_timeSinceLastTick >= m_fixedTimestep)
			{
				m_physics.FixedUpdate();
				FixedUpdate(m_fixedTimestep);
				m_timeSinceLastTick -= m_fixedTimestep;
			}

			Update();

			// Draw
			m_render.TestDraw();
			Render();
		}
	}
	m_render.WaitIdle();
	Shutdown();
}

bool EngineApplication::setup()
{
	EngineApplicationCreateInfo createInfo = Config();

	if (!initializeLog(createInfo.logFilePath))
		return false;

	if (!m_window.Setup(createInfo.window))
		return false;
	if (!m_input.Setup())
		return false;
	if (!m_render.Setup(createInfo.render))
		return false;
	if (!m_physics.Setup(createInfo.physics))
		return false;

	if (!Setup())
		return false;

	if (m_status == StatusApp::NonInit)
		m_status = StatusApp::Success;

	if (m_status != StatusApp::Success)
		return false;

	m_lastFrameTime = static_cast<float>(glfwGetTime());

	return true;
}

bool EngineApplication::initializeLog(std::string_view filePath)
{
	if (!filePath.empty())
		m_logFile.open(filePath);

	return true;
}

void EngineApplication::shutdownLog()
{
	if (m_logFile.is_open())
		m_logFile.close();
}

void EngineApplication::Quit()
{
	m_status = StatusApp::Quit;
}

void EngineApplication::Failed()
{
	m_status = StatusApp::ErrorFailed;
}

void EngineApplication::Print(const std::string& msg)
{
	// console
	puts(msg.data());

	// file
	if (m_logFile.is_open())
	{
		m_logFile << msg << std::endl;
		m_logFile.flush();
	}
}

void EngineApplication::Warning(const std::string& msg)
{
	Print("[WARNING] " + msg);
}

void EngineApplication::Error(const std::string& msg)
{
	Print("[ERROR] " + msg);
}

void EngineApplication::Fatal(const std::string& msg)
{
	m_status = StatusApp::ErrorFailed;
	Print("[FATAL] " + msg);
}

const KeyState& EngineApplication::GetKeyState(KeyCode code) const
{
	if ((code < KEY_RANGE_FIRST) || (code >= KEY_RANGE_LAST)) 
		return m_keyStates[0];

	return m_keyStates[static_cast<uint32_t>(code)];
}

bool EngineApplication::IsWindowIconified() const
{
	return m_window.IsIconified();
}

bool EngineApplication::IsWindowMaximized() const
{
	return m_window.IsMaximized();
}

void EngineApplication::resizeCallback(uint32_t width, uint32_t height)
{
	m_render.resize();
	Resize(width, height);
}

void EngineApplication::windowIconifyCallback(bool iconified)
{
	WindowIconify(iconified);
}

void EngineApplication::windowMaximizeCallback(bool maximized)
{
	WindowMaximize(maximized);
}

void EngineApplication::mouseDownCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse) 
		return;

	MouseDown(x, y, buttons);
}

void EngineApplication::mouseUpCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	MouseUp(x, y, buttons);
}

void EngineApplication::mouseMoveCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	int32_t dx = (m_previousMouseX != INT32_MAX) ? (x - m_previousMouseX) : 0;
	int32_t dy = (m_previousMouseY != INT32_MAX) ? (y - m_previousMouseY) : 0;

	m_input.m_mouseState.delta = { dx , dy };

	MouseMove(x, y, dx, dy, buttons);
	m_previousMouseX = x;
	m_previousMouseY = y;
}

void EngineApplication::scrollCallback(float dx, float dy)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	Scroll(dx, dy);
}

void EngineApplication::keyDownCallback(KeyCode key)
{
	if (ImGui::GetIO().WantCaptureKeyboard) 
		return;

	m_keyStates[key].down = true;
	//m_keyStates[key].timeDown = GetElapsedSeconds(); // TODO:
	KeyDown(key);
}

void EngineApplication::keyUpCallback(KeyCode key)
{
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	m_keyStates[key].down = false;
	m_keyStates[key].timeDown = FLT_MAX;
	KeyUp(key);
}

#pragma endregion