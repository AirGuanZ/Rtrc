#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanQueue;

RTRC_RHI_IMPLEMENT(VulkanFence, Fence)
{
public:

    VulkanFence(VkDevice device, VkFence fence);

    ~VulkanFence() override;

    void Reset() RTRC_RHI_OVERRIDE;

    void Wait() RTRC_RHI_OVERRIDE;

    VkFence _internalGetNativeFence() const;

private:

    friend class VulkanQueue;

    VkDevice device_;
    VkFence fence_;

    QueueSessionID               syncSessionID_;
    std::atomic<QueueSessionID> *syncSessionIDRecevier_;
};

RTRC_RHI_VK_END
