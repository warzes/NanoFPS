#include "stdafx.h"
#include "Core.h"
#include "PhysicsSystem.h"
#include "Application.h"

#if defined(_MSC_VER)
#	pragma comment( lib, "PhysX_64.lib" )
#	pragma comment( lib, "PhysXFoundation_64.lib" )
#	pragma comment( lib, "PhysXCooking_64.lib" )
#	pragma comment( lib, "PhysXCommon_64.lib" )

//#	pragma comment( lib, "LowLevel_static_64.lib" )
//#	pragma comment( lib, "LowLevelAABB_static_64.lib" )
//#	pragma comment( lib, "LowLevelDynamics_static_64.lib" )
#	pragma comment( lib, "PhysXCharacterKinematic_static_64.lib" )
#	pragma comment( lib, "PhysXExtensions_static_64.lib" )
//#	pragma comment( lib, "PhysXPvdSDK_static_64.lib" )
#endif

namespace ph {

using namespace physx;

//=============================================================================
#pragma region [ Error Callback ]

void info(const std::string& error, const std::string& message, const std::string& file, int line) noexcept
{
	Print("PhysX (" + error + "): " + message + " at " + file + ":" + std::to_string(line));
}

void warning(const std::string& error, const std::string& message, const std::string& file, int line) noexcept
{
	Warning("PhysX (" + error + "): " + message + " at " + file + ":" + std::to_string(line));
}

void error(const std::string& error, const std::string& message, const std::string& file, int line) noexcept
{
	Error("PhysX (" + error + "): " + message + " at " + file + ":" + std::to_string(line));
}

class PhysicsErrorCallback final : public physx::PxErrorCallback
{
public:
	void reportError(PxErrorCode::Enum code, const char* message, const char* file, int line) final
	{
		const char* msgType = "Unknown Error";
		void (*loggingCallback)(const std::string&, const std::string&, const std::string&, int) = error;
		switch (code)
		{
		case physx::PxErrorCode::eNO_ERROR:          msgType = "No Error";            loggingCallback = info; break;
		case physx::PxErrorCode::eDEBUG_INFO:        msgType = "Debug Info";          loggingCallback = info; break;
		case physx::PxErrorCode::eDEBUG_WARNING:     msgType = "Debug Warning";       loggingCallback = warning; break;
		case physx::PxErrorCode::eINVALID_PARAMETER: msgType = "Invalid Parameter";   loggingCallback = error; break;
		case physx::PxErrorCode::eINVALID_OPERATION: msgType = "Invalid Operation";   loggingCallback = error; break;
		case physx::PxErrorCode::eOUT_OF_MEMORY:     msgType = "Out of Memory";       loggingCallback = error; break;
		case physx::PxErrorCode::eINTERNAL_ERROR:    msgType = "Internal Error";      loggingCallback = error; break;
		case physx::PxErrorCode::eABORT:             msgType = "Abort";               loggingCallback = error; break;
		case physx::PxErrorCode::ePERF_WARNING:      msgType = "Performance Warning"; loggingCallback = warning; break;
		default: break;
		}
		loggingCallback(msgType, message, file, line);
	}
};

#pragma endregion

//=============================================================================
#pragma region [ Physics System ]

PxDefaultAllocator      gDefaultAllocatorCallback;
PhysicsErrorCallback    gErrorCallback;
PxDefaultCpuDispatcher* gCpuDispatcher{ nullptr };

PhysicsSystem::PhysicsSystem(EngineApplication& engine)
	: m_engine(engine)
	, m_scene(engine, *this)
{
}

bool PhysicsSystem::Setup(const PhysicsCreateInfo& createInfo)
{
	m_enable       = createInfo.enable;
	m_scale.length = createInfo.typicalLength;
	m_scale.speed  = createInfo.typicalSpeed;

	m_foundation = PxCreateFoundation(PX_PHYSICS_VERSION, gDefaultAllocatorCallback, gErrorCallback);
	if (!m_foundation)
	{
		Fatal("PhysX foundation failed to create.");
		return false;
	}

	m_physics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_foundation, m_scale);
	if (!m_physics)
	{
		Fatal("Failed to create PhysX physics.");
		return false;
	}
	
	Print("PhysX Init " + std::to_string(PX_PHYSICS_VERSION_MAJOR) + "." + std::to_string(PX_PHYSICS_VERSION_MINOR) + "." + std::to_string(PX_PHYSICS_VERSION_BUGFIX));

	gCpuDispatcher = PxDefaultCpuDispatcherCreate(createInfo.cpuDispatcherNum);
	if (!gCpuDispatcher)
	{
		Fatal("Failed to create default PhysX CPU dispatcher.");
		return false;
	}


	m_defaultMaterial = CreateMaterial(createInfo.defaultMaterial);
	if (!m_defaultMaterial->IsValid())
	{
		Fatal("Failed to create default PhysX material.");
		return false;
	}

	if (!m_scene.Setup(createInfo.scene)) return false;

	return true;
}

void PhysicsSystem::Shutdown()
{
	m_scene.Shutdown();
	m_defaultMaterial.reset();
	PX_RELEASE(gCpuDispatcher);
	PX_RELEASE(m_physics);
	PX_RELEASE(m_foundation);
}

void PhysicsSystem::FixedUpdate()
{
	if (!m_enable) return;
	m_scene.FixedUpdate();
}

MaterialPtr PhysicsSystem::CreateMaterial(const MaterialCreateInfo& createInfo)
{
	return std::make_shared<Material>(m_engine, createInfo);
}

void PhysicsSystem::SetPaused(bool paused)
{
	m_enable = !paused;
}

#pragma endregion

} // namespace ph