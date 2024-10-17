#include "Base.h"
#include "Core.h"
#include "RenderContext.h"
#include "EngineWindow.h"
#include "EngineApp.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "glfw3.lib" )
#	pragma comment( lib, "simdjson.lib" )
#	pragma comment( lib, "physfs-static.lib" )
#	pragma comment( lib, "lua.lib" )

#pragma comment( lib, "PhysX_64.lib" )
#pragma comment( lib, "PhysXFoundation_64.lib" )
#pragma comment( lib, "PhysXCooking_64.lib" )
#pragma comment( lib, "PhysXCommon_64.lib" )

//#pragma comment( lib, "LowLevel_static_64.lib" )
//#pragma comment( lib, "LowLevelAABB_static_64.lib" )
//#pragma comment( lib, "LowLevelDynamics_static_64.lib" )
#pragma comment( lib, "PhysXCharacterKinematic_static_64.lib" )
#pragma comment( lib, "PhysXExtensions_static_64.lib" )


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
	float LastFrameTime{ 0.0f };
	float DeltaTime{0.0f};

	bool IsAppEnd = false;
}

bool EngineApp::Create(const EngineCreateInfo& createInfo)
{
	IsAppEnd = false;

	FileSystem::Init();
	FileSystem::Mount("Data", "/");

	if (!Window::Create(createInfo.window)) return false;
	if (!RenderContext::Create(createInfo.renderContext)) return false;

	if (IsAppEnd) return false; // произошли фатальные ошибки

	LastFrameTime = static_cast<float>(glfwGetTime());
	DeltaTime = 0.0f;
	return true;
}

void EngineApp::Destroy()
{
	RenderContext::Destroy();
	Window::Destroy();
	FileSystem::Shutdown();
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