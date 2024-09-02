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
	if (m_log.fileStream.is_open())
	{
		m_log.fileStream << msg << std::endl;
		m_log.fileStream.flush();
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

			m_window.Update();
			m_input.Update();
			m_render.TestDraw();
		}
	}
}

bool EngineApplication::initializeLog(std::string_view filePath)
{
	if (!filePath.empty())
	{
		m_log.fileStream.open(filePath);
	}

	return true;
}

void EngineApplication::shutdownLog()
{
	if (m_log.fileStream.is_open())
		m_log.fileStream.close();
}

#pragma endregion