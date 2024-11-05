#pragma once

#include "VulkanCore.h"

namespace vkf {

struct SurfaceCreateInfo final
{
#if defined(_WIN32)
	HINSTANCE hinstance{ nullptr };
	HWND      hwnd{ nullptr };
#endif
};

class Surface final
{
public:
	~Surface();

	bool Setup(Instance* instance, const SurfaceCreateInfo& createInfo);
	void Shutdown();

	operator VkSurfaceKHR() const { return m_surface; }

	[[nodiscard]] bool IsValid() const { return m_surface != nullptr; }

private:
	Instance*    m_instance{ nullptr };
	VkSurfaceKHR m_surface{ nullptr };
};

} // namespace vkf