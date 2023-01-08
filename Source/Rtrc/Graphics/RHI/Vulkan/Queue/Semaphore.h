#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanSemaphore, Semaphore)
{
public:

    VulkanSemaphore(VkDevice device, VkSemaphore semaphore);

    ~VulkanSemaphore() override;

    uint64_t GetValue() const RTRC_RHI_OVERRIDE;

    VkSemaphore _internalGetNativeSemaphore() const;

private:

    VkDevice    device_;
    VkSemaphore semaphore_;
};

RTRC_RHI_VK_END
