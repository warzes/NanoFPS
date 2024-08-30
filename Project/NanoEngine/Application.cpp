#include "stdafx.h"
#include "Core.h"
#include "Application.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif

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

#pragma endregion

#pragma region Engine Application

EngineApplication::~EngineApplication()
{
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

		if (!m_window.Setup(*this, createInfo.window))
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