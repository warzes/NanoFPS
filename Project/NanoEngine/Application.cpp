#include "stdafx.h"
#include "Core.h"
#include "Application.h"

#pragma region IApplication

void IApplication::Quit()
{
	engine->Quit();
}

void IApplication::Print(const std::string& msg)
{
	engine->Print(msg);
}

void IApplication::Warning(const std::string& msg)
{
	engine->Warning(msg);
}

void IApplication::Error(const std::string& msg)
{
	engine->Error(msg);
}

void IApplication::Fatal(const std::string& msg)
{
	engine->Fatal(msg);
}

Window& IApplication::GetWindow()
{
	return engine->GetWindow();
}
Input& IApplication::GetInput()
{
	return engine->GetInput();
}

RenderSystem& IApplication::GetRender()
{
	return engine->GetRender();
}

#pragma endregion

#pragma region Engine Application

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
{
	thisEngineApplication = this;
}

EngineApplication::~EngineApplication()
{
	m_render.Shutdown();
	m_input.Shutdown();
	m_window.Shutdown();
	shutdownLog();
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

void EngineApplication::run(IApplication* app)
{
	assert(app);
	assert(m_status == StatusApp::NonInit);

	m_app = app;
	m_app->engine = this;

	// Init
	{
		const EngineApplicationCreateInfo createInfo = m_app->Config();

		if (!initializeLog(createInfo.logFilePath))
			return;

		if (!m_window.Setup(createInfo.window))
			return;
		if (!m_input.Setup())
			return;
		if (!m_render.Setup(createInfo.render))
			return;

		if (m_status == StatusApp::NonInit)
			m_status = StatusApp::Success;

		if (m_status != StatusApp::Success)
			return;
	}

	// Main Loop
	{
		while (m_status == StatusApp::Success)
		{
			if (m_window.ShouldClose()) break;

			// Update
			m_window.Update();
			m_input.Update();
			m_render.Update();

			// Draw
			m_render.TestDraw();
		}
	}
}

bool EngineApplication::initializeLog(std::string_view filePath)
{
	if (!filePath.empty())
	{
		m_logFile.open(filePath);
	}

	return true;
}

void EngineApplication::shutdownLog()
{
	if (m_logFile.is_open())
		m_logFile.close();
}

void EngineApplication::resizeCallback(uint32_t width, uint32_t height)
{
	m_render.resize();
	if (m_app) m_app->Resize(width, height);
}

void EngineApplication::windowIconifyCallback(bool iconified)
{
	if (m_app) m_app->WindowIconify(iconified);
}

void EngineApplication::windowMaximizeCallback(bool maximized)
{
	if (m_app) m_app->WindowMaximize(maximized);
}

void EngineApplication::mouseDownCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse) 
		return;

	if (m_app) m_app->MouseDown(x, y, buttons);
}

void EngineApplication::mouseUpCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (m_app) m_app->MouseUp(x, y, buttons);
}

void EngineApplication::mouseMoveCallback(int32_t x, int32_t y, MouseButton buttons)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	int32_t dx = (m_previousMouseX != INT32_MAX) ? (x - m_previousMouseX) : 0;
	int32_t dy = (m_previousMouseY != INT32_MAX) ? (y - m_previousMouseY) : 0;
	if (m_app) m_app->MouseMove(x, y, dx, dy, buttons);
	m_previousMouseX = x;
	m_previousMouseY = y;
}

void EngineApplication::scrollCallback(float dx, float dy)
{
	if (ImGui::GetIO().WantCaptureMouse)
		return;

	if (m_app) m_app->Scroll(dx, dy);
}

void EngineApplication::keyDownCallback(KeyCode key)
{
	if (ImGui::GetIO().WantCaptureKeyboard) 
		return;

	m_keyStates[key].down = true;
	//m_keyStates[key].timeDown = GetElapsedSeconds(); // TODO:
	if (m_app) m_app->KeyDown(key);
}

void EngineApplication::keyUpCallback(KeyCode key)
{
	if (ImGui::GetIO().WantCaptureKeyboard)
		return;

	m_keyStates[key].down = false;
	m_keyStates[key].timeDown = FLT_MAX;
	if (m_app) m_app->KeyUp(key);
}

#pragma endregion