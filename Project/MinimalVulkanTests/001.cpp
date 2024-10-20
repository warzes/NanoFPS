#include "stdafx.h"
#include "001.h"

ApplicationCreateInfo Test001::GetConfig()
{
	return ApplicationCreateInfo();
}

bool Test001::Start()
{
	vkw::Context context;

#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

	vkw::InstanceCreateInfo ici{};
	ici.enableValidationLayers = enableValidationLayers;
	vkw::InstancePtr instance = context.CreateInstance(ici);
	if (!instance) return false;

	return true;
}

void Test001::Shutdown()
{

}

void Test001::FixedUpdate(float deltaTime)
{
}

void Test001::Update(float deltaTime)
{
}

void Test001::Frame(float deltaTime)
{
}
