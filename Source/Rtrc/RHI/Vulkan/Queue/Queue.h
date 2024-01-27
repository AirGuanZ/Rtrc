#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanQueue, Queue)
{
public:

#if RTRC_STATIC_RHI
    RTRC_RHI_QUEUE_COMMON
#endif

    VulkanQueue(VulkanDevice *device, VkQueue queue, QueueType type, uint32_t queueFamilyIndex);

    QueueType GetType() const RTRC_RHI_OVERRIDE;

    void WaitIdle() RTRC_RHI_OVERRIDE;

    void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<RPtr<CommandBuffer>>     commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        OPtr<Fence>                   signalFence) RTRC_RHI_OVERRIDE;

    QueueSessionID GetCurrentSessionID() RTRC_RHI_OVERRIDE;
    QueueSessionID GetSynchronizedSessionID() RTRC_RHI_OVERRIDE;

    VkQueue _internalGetNativeQueue() const;

    uint32_t _internalGetNativeFamilyIndex() const;

    UPtr<CommandPool> _internalCreateCommandPoolImpl() const;

    void _internalOnExternalHostDeviceSynchronization();

private:

    VulkanDevice          *device_;
    VkQueue                queue_;
    QueueType              type_;
    uint32_t               queueFamilyIndex_;
    std::atomic<QueueSessionID> currentSessionID_;
    std::atomic<QueueSessionID> synchronizedSessionID_;
};

RTRC_RHI_VK_END
