#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBackBufferSemaphore, BackBufferSemaphore)
{
public:

    VulkanBackBufferSemaphore(VkDevice device, VkSemaphore binarySemaphore);

    ~VulkanBackBufferSemaphore() override;

    VkSemaphore _internalGetBinarySemaphore() const;

private:

    VkDevice device_;
    VkSemaphore binarySemaphore_;
};

RTRC_RHI_VK_END
