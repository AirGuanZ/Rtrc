#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanQueue : public Queue
{
public:

    VulkanQueue(VkDevice device, VkQueue queue, uint32_t queueFamilyIndex);

    RC<CommandPool> CreateCommandPool() override;

    VkQueue GetQueue() const;

    void Submit(
        const RC<BackBufferSemaphore>        &waitBackBufferSemaphore,
        PipelineStage                         waitBackBufferStages,
        const std::vector<RC<CommandBuffer>> &commandBuffers,
        const RC<BackBufferSemaphore>        &signalBackBufferSemaphore,
        PipelineStage                         signalBackBufferStages,
        const RC<Fence>                      &signalFence) override;

private:

    VkDevice device_;
    VkQueue queue_;
    uint32_t queueFamilyIndex_;
};

RTRC_RHI_VK_END
