/*
0002
- VulkanInstance 
- PhysicalDevice list
- Select optimal PhysicalDevice
- Create Logical device
*/

#include "01_VulkanCore.h"
#include "02_VulkanInstance.h"
#include "03_VulkanPhysicalDevice.h"

void StartLesson0002()
{
	try
	{
		VulkanInstance instance;
		std::vector<PhysicalDevice> physicalDevices = instance.QueryPhysicalDevices();

	}
	catch (const std::exception& msg)
	{
		puts(msg.what());
	}
	catch (...)
	{
		puts("Unknown error");
	}
}