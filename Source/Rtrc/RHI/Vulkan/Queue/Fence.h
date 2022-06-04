#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanFence : public Fence
{
public:

    VulkanFence(VkDevice device, VkFence fence);

    ~VulkanFence() override;

    void Reset() override;

    void Wait() override;

    VkFence GetNativeFence() const;

private:

    VkDevice device_;
    VkFence fence_;
};

RTRC_RHI_VK_END
