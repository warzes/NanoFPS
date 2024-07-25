#include "Base.h"
#include "Core.h"
#include "RenderContext.h"
#include "EngineWindow.h"
#include "EngineApp.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#	pragma comment( lib, "simdjson.lib" )

#	if defined(_DEBUG)
#		pragma comment( lib, "GenericCodeGend.lib" )
#		pragma comment( lib, "glslangd.lib" )
#		pragma comment( lib, "glslang-default-resource-limitsd.lib" )
#		pragma comment( lib, "HLSLd.lib" )
#		pragma comment( lib, "MachineIndependentd.lib" )
#		pragma comment( lib, "OGLCompilerd.lib" )
#		pragma comment( lib, "OSDependentd.lib" )
#		pragma comment( lib, "SPIRVd.lib" )

#	else
#		pragma comment( lib, "GenericCodeGen.lib" )
#		pragma comment( lib, "glslang.lib" )
#		pragma comment( lib, "glslang-default-resource-limits.lib" )
#		pragma comment( lib, "HLSL.lib" )
#		pragma comment( lib, "MachineIndependent.lib" )
#		pragma comment( lib, "OGLCompiler.lib" )
#		pragma comment( lib, "OSDependent.lib" )
#		pragma comment( lib, "SPIRV.lib" )

#	endif
#endif

namespace
{
	float LastFrameTime{};
	float DeltaTime{};

	bool IsAppEnd = false;
}

bool EngineApp::Create(const EngineCreateInfo& createInfo)
{
	IsAppEnd = false;
	if (!Window::Create(createInfo.window)) return false;
	if (!RenderContext::Create(createInfo.renderContext)) return false;

	if (IsAppEnd) return false; // произошли фатальные ошибки

	LastFrameTime = static_cast<float>(glfwGetTime());
	return true;
}

void EngineApp::Destroy()
{
	RenderContext::Destroy();
	Window::Destroy();
}

bool EngineApp::ShouldClose()
{
	return Window::ShouldClose() || IsAppEnd;
}

void EngineApp::Update()
{
	float currentFrame = static_cast<float>(glfwGetTime());
	DeltaTime = currentFrame - LastFrameTime;
	LastFrameTime = currentFrame;

	Window::Update();
}

void EngineApp::BeginFrame()
{
	RenderContext::BeginFrame();
}

void EngineApp::EndFrame()
{
	RenderContext::EndFrame();
}

float EngineApp::GetDeltaTime()
{
	return DeltaTime;
}

void EngineApp::Exit()
{
	IsAppEnd = true;
}