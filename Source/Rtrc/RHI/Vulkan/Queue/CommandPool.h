#pragma once

#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandPool : public CommandPool
{
public:

    VulkanCommandPool(VkDevice device, VkCommandPool pool);

    ~VulkanCommandPool() override;

    void Reset() override;

    Ptr<CommandBuffer> NewCommandBuffer() override;

private:

    void CreateCommandBuffer();

    VkDevice device_;
    VkCommandPool pool_;
    size_t nextFreeBufferIndex_;
    std::vector<Ptr<VulkanCommandBuffer>> commandBuffers_;
};

RTRC_RHI_VK_END
