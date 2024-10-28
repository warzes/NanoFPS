#include "VulkanSurface.h"
#include "VulkanInstance.h"

namespace vkf {

Surface::~Surface()
{
	Shutdown();
}

bool Surface::Setup(Instance* instance, const SurfaceCreateInfo& createInfo)
{
	assert(!m_surface);
	assert(instance);

	m_instance = instance;

#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = { .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	surfaceCreateInfo.hinstance                   = createInfo.hinstance;
	surfaceCreateInfo.hwnd                        = createInfo.hwnd;
	VkResult result = vkCreateWin32SurfaceKHR(*m_instance, &surfaceCreateInfo, nullptr, &m_surface);
#endif
	if (result != VK_SUCCESS)
	{
		Fatal("Failed to create window surface! - " + std::string(string_VkResult(result)));
		return false;
	}

	return true;
}

void Surface::Shutdown()
{
	if (m_surface) vkDestroySurfaceKHR(*m_instance, m_surface, nullptr);
	m_surface = nullptr;
}

} // namespace vkf