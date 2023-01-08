#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanCommandPool, CommandPool)
{
public:

    VulkanCommandPool(VulkanDevice *device, QueueType type, VkCommandPool pool);

    ~VulkanCommandPool() override;

    void Reset() RTRC_RHI_OVERRIDE;

    QueueType GetType() const RTRC_RHI_OVERRIDE;

    Ptr<CommandBuffer> NewCommandBuffer() RTRC_RHI_OVERRIDE;

private:

    void CreateCommandBuffer();

    VulkanDevice *device_;
    QueueType type_;
    VkCommandPool pool_;
    size_t nextFreeBufferIndex_;
    std::vector<Ptr<VulkanCommandBuffer>> commandBuffers_;
};

RTRC_RHI_VK_END
