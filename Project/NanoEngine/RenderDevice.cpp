#include "stdafx.h"
#include "Core.h"
#include "RenderCore.h"
#include "RenderResources.h"
#include "RenderDevice.h"
#include "RenderSystem.h"

#pragma region RenderDevice

RenderDevice::RenderDevice(EngineApplication& engine, RenderSystem& render)
	: m_engine(engine)
	, m_render(render)
{
}

VkDevice& RenderDevice::GetVkDevice()
{
	return m_render.GetVkDevice();
}

VmaAllocatorPtr RenderDevice::GetVmaAllocator()
{
	return m_render.GetVmaAllocator();
}

DeviceQueuePtr RenderDevice::GetGraphicsDeviceQueue() const
{
	return m_render.GetVkGraphicsQueue();
}
DeviceQueuePtr RenderDevice::GetPresentDeviceQueue() const
{
	return m_render.GetVkPresentQueue();
}
DeviceQueuePtr RenderDevice::GetTransferDeviceQueue() const
{
	return m_render.GetVkTransferQueue();
}
DeviceQueuePtr RenderDevice::GetComputeDeviceQueue() const
{
	return m_render.GetVkComputeQueue();
}

uint32_t RenderDevice::GetGraphicsQueueFamilyIndex() const
{
	return GetGraphicsDeviceQueue()->QueueFamily;
}

uint32_t RenderDevice::GetComputeQueueFamilyIndex() const
{
	return GetComputeDeviceQueue()->QueueFamily;
}

uint32_t RenderDevice::GetTransferQueueFamilyIndex() const
{
	return GetTransferDeviceQueue()->QueueFamily;
}

std::array<uint32_t, 3> RenderDevice::GetAllQueueFamilyIndices() const
{
	return { GetGraphicsQueueFamilyIndex(), GetComputeQueueFamilyIndex(), GetTransferQueueFamilyIndex() };
}

Result RenderDevice::CreateFence(const FenceCreateInfo& createInfo, Fence** ppFence)
{
	ASSERT_NULL_ARG(ppFence);
	return createObject(createInfo, mFences, ppFence);
}

void RenderDevice::DestroyFence(const Fence* fence)
{
	ASSERT_NULL_ARG(fence);
	destroyObject(mFences, fence);
}

Result RenderDevice::allocateObject(Fence** ppObject)
{
	Fence* pObject = new Fence();
	if (IsNull(pObject)) return ERROR_ALLOCATION_FAILED;
	*ppObject = pObject;
	return SUCCESS;
}

template<typename ObjectT, typename CreateInfoT, typename ContainerT>
inline Result RenderDevice::createObject(const CreateInfoT& createInfo, ContainerT& container, ObjectT** ppObject)
{
	// Allocate object
	ObjectT* pObject = nullptr;
	Result   ppxres = allocateObject(&pObject);
	if (Failed(ppxres)) return ppxres;

	// Set parent
	pObject->setParent(this);
	// Create internal objects
	ppxres = pObject->create(createInfo);
	if (Failed(ppxres)) 
	{
		pObject->destroy();
		delete pObject;
		pObject = nullptr;
		return ppxres;
	}
	// Store
	container.push_back(ObjPtr<ObjectT>(pObject));
	// Assign
	*ppObject = pObject;
	// Success
	return SUCCESS;
}

template<typename ObjectT, typename ContainerT>
void RenderDevice::destroyObject(ContainerT& container, const ObjectT* pObject)
{
	// Make sure object is in container
	auto it = std::find_if(
		std::begin(container),
		std::end(container),
		[pObject](const ObjPtr<ObjectT>& elem) -> bool {
			bool res = (elem == pObject);
			return res; });
	if (it == std::end(container)) return;

	// Copy pointer
	ObjPtr<ObjectT> object = *it;
	// Remove object pointer from container
	RemoveElement(object, container);
	// Destroy internal objects
	object->destroy();
	// Delete allocation
	ObjectT* ptr = object.Get();
	delete ptr;
}

template<typename ObjectT>
void RenderDevice::destroyAllObjects(std::vector<ObjPtr<ObjectT>>& container)
{
	size_t n = container.size();
	for (size_t i = 0; i < n; ++i)
	{
		// Get object pointer
		ObjPtr<ObjectT> object = container[i];
		// Destroy internal objects
		object->destroy();
		// Delete allocation
		ObjectT* ptr = object.Get();
		delete ptr;
	}
	// Clear container
	container.clear();
}


//===================================================================
// OLD
//===================================================================

VulkanFencePtr RenderDevice::CreateFence(const FenceCreateInfo& createInfo)
{
	VulkanFencePtr res = std::make_shared<VulkanFence>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

VulkanSemaphorePtr RenderDevice::CreateSemaphore(const SemaphoreCreateInfo& createInfo)
{
	VulkanSemaphorePtr res = std::make_shared<VulkanSemaphore>(*this, createInfo);
	return res->IsValid() ? res : nullptr;
}

#pragma endregion


