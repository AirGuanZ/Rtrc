#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanQueue : public Queue
{
public:

    VulkanQueue(VulkanDevice *device, VkQueue queue, QueueType type, uint32_t queueFamilyIndex);

    QueueType GetType() const override;

    void WaitIdle() override;

    void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<Ptr<CommandBuffer>>      commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        const Ptr<Fence>             &signalFence) override;

    VkQueue GetNativeQueue() const;

    uint32_t GetNativeFamilyIndex() const;

    Ptr<CommandPool> CreateCommandPoolImpl() const;

private:

    VulkanDevice *device_;
    VkQueue queue_;
    QueueType type_;
    uint32_t queueFamilyIndex_;
};

RTRC_RHI_VK_END