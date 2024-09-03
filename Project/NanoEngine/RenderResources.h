#pragma once

#include "RenderCore.h"

#pragma region VulkanFence

struct FenceCreateInfo final
{
	bool signaled = false;
};

class VulkanFence final
{
	friend class RenderDevice;
public:
	VulkanFence(RenderDevice& device, const FenceCreateInfo& createInfo);
	~VulkanFence();

	bool IsValid() const { return m_fence != nullptr; }

	bool Wait(uint64_t timeout = UINT64_MAX);
	bool Reset();

	[[nodiscard]] VkResult Status() const;

	VkFence Get() { return m_fence; }

private:
	RenderDevice& m_device;
	VkFence m_fence{ nullptr };
};
using VulkanFencePtr = std::shared_ptr<VulkanFence>;

#pragma endregion

#pragma region VulkanSemaphore

struct SemaphoreCreateInfo final
{
	SemaphoreType semaphoreType = SEMAPHORE_TYPE_BINARY;
	uint64_t      initialValue = 0; // Timeline semaphore only
};

class VulkanSemaphore final
{
	friend class RenderDevice;
public:
	VulkanSemaphore(RenderDevice& device, const SemaphoreCreateInfo& createInfo);
	~VulkanSemaphore();

	bool IsValid() const { return m_semaphore != nullptr; }

	VkSemaphore Get() { return m_semaphore; }
	SemaphoreType GetSemaphoreType() const { return m_semaphoreType; }
	bool IsBinary() const { return m_semaphoreType == SEMAPHORE_TYPE_BINARY; }
	bool IsTimeline() const { return m_semaphoreType == SEMAPHORE_TYPE_TIMELINE; }
	uint64_t GetCounterValue() const; // Returns current timeline semaphore value

	bool Wait(uint64_t value, uint64_t timeout = UINT64_MAX) const;
	bool Signal(uint64_t value) const;

private:
	bool timelineWait(uint64_t value, uint64_t timeout) const;
	bool timelineSignal(uint64_t value) const;
	uint64_t timelineCounterValue() const;

	RenderDevice&    m_device;
	mutable uint64_t m_timelineSignaledValue = 0;
	VkSemaphore      m_semaphore{ nullptr };
	SemaphoreType    m_semaphoreType{ SEMAPHORE_TYPE_BINARY };
};
using VulkanSemaphorePtr = std::shared_ptr<VulkanSemaphore>;

#pragma endregion
