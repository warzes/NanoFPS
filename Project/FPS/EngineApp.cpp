#include "Base.h"
#include "Core.h"
#include "NewRenderer.h"
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
	NewRenderer::Init();

	LastFrameTime = static_cast<float>(glfwGetTime());

	IsAppEnd = false;
	return true;
}

void EngineApp::Destroy()
{
	NewRenderer::Close();
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
	NewRenderer::Draw();
}

float EngineApp::GetDeltaTime()
{
	return DeltaTime;
}