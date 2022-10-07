#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandPool : public CommandPool
{
public:

    VulkanCommandPool(VulkanDevice *device, QueueType type, VkCommandPool pool);

    ~VulkanCommandPool() override;

    void Reset() override;

    QueueType GetType() const override;

    Ptr<CommandBuffer> NewCommandBuffer() override;

private:

    void CreateCommandBuffer();

    VulkanDevice *device_;
    QueueType type_;
    VkCommandPool pool_;
    size_t nextFreeBufferIndex_;
    std::vector<Ptr<VulkanCommandBuffer>> commandBuffers_;
};

RTRC_RHI_VK_END
