#include "stdafx.h"
#include "001.h"

ApplicationCreateInfo Test001::GetConfig()
{
	return ApplicationCreateInfo();
}

class System;

class Resource
{
public:
	Resource() { puts("resource create"); }
	~Resource() { puts("resource destroy"); }

	std::shared_ptr<System> system;
};

class System : public std::enable_shared_from_this<System>
{
public:
	System() { puts("system create"); }
	~System() { puts("system destroy"); }

	std::shared_ptr<Resource> CreateRes()
	{ 
		auto res = std::make_shared<Resource>();
		res->system = shared_from_this();
		return res;
	}
};

class Creat
{
public:
	std::shared_ptr<System> Create() { return std::make_shared<System>(); }
};



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

	auto devices = instance->GetPhysicalDevices();


	Creat cr;

	std::shared_ptr<System> sys = cr.Create();

	std::shared_ptr<Resource> res = sys->CreateRes();
	std::shared_ptr<Resource> res2 = sys->CreateRes();

	sys.reset();
	res.reset();
	res2.reset();



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
