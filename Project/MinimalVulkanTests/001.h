#pragma once

#include "Application.h"
#include <VulkanWrapper.h>

class Test001 final : public Application
{
public:
	ApplicationCreateInfo GetConfig() final;
	bool Start() final;
	void Shutdown() final;
	void FixedUpdate(float deltaTime) final;
	void Update(float deltaTime) final;
	void Frame(float deltaTime) final;

private:
};