#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBackBufferSemaphore : public BackBufferSemaphore
{
public:

    VulkanBackBufferSemaphore(VkDevice device, VkSemaphore binarySemaphore);

    ~VulkanBackBufferSemaphore() override;

    VkSemaphore GetBinarySemaphore() const;

private:

    VkDevice device_;
    VkSemaphore binarySemaphore_;
};

RTRC_RHI_VK_END
