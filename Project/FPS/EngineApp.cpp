#include "Base.h"
#include "Core.h"
#include "RenderDevice.h"
#include "EngineWindow.h"
#include "EngineApp.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#endif

namespace
{
	float LastFrameTime{};
	float DeltaTime{};

	bool IsAppEnd = false;
}

void AppEnd()
{
	IsAppEnd = true;
}

bool EngineApp::Create()
{
	Window::Create({});
	Renderer::Init();

	LastFrameTime = static_cast<float>(glfwGetTime());

	IsAppEnd = false;
	return true;
}

void EngineApp::Destroy()
{
	Renderer::Close();
	Window::Destroy();
}

bool EngineApp::ShouldClose()
{
	return Window::ShouldClose() || IsAppEnd;
}

void EngineApp::BeginFrame()
{
	float currentFrame = static_cast<float>(glfwGetTime());
	DeltaTime = currentFrame - LastFrameTime;
	LastFrameTime = currentFrame;

	Window::Update();
}

void EngineApp::EndFrame()
{
	Renderer::Draw();
}

float EngineApp::GetDeltaTime()
{
	return DeltaTime;
}