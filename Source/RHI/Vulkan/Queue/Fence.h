#pragma once

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanFence, Fence)
{
public:

    VulkanFence(VkDevice device, VkFence fence);

    ~VulkanFence() override;

    void Reset() RTRC_RHI_OVERRIDE;

    void Wait() RTRC_RHI_OVERRIDE;

    VkFence _internalGetNativeFence() const;

private:

    VkDevice device_;
    VkFence fence_;
};

RTRC_RHI_VK_END
