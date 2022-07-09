#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanQueue : public Queue
{
public:

    VulkanQueue(VkDevice device, VkQueue queue, QueueType type, uint32_t queueFamilyIndex);

    QueueType GetType() const override;

    Ptr<CommandPool> CreateCommandPool() override;

    void WaitIdle() override;

    void Submit(
        const Ptr<BackBufferSemaphore> &waitBackBufferSemaphore,
        PipelineStage                   waitBackBufferStages,
        Span<Ptr<CommandBuffer>>        commandBuffers,
        const Ptr<BackBufferSemaphore> &signalBackBufferSemaphore,
        PipelineStage                   signalBackBufferStages,
        const Ptr<Fence>               &signalFence) override;

    VkQueue GetNativeQueue() const;

    uint32_t GetNativeFamilyIndex() const;

private:

    VkDevice device_;
    VkQueue queue_;
    QueueType type_;
    uint32_t queueFamilyIndex_;
};

RTRC_RHI_VK_END
