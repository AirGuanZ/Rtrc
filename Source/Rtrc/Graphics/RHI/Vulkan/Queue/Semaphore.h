#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanSemaphore : public Semaphore
{
public:

    VulkanSemaphore(VkDevice device, VkSemaphore semaphore);

    ~VulkanSemaphore() override;

    uint64_t GetValue() const override;

    VkSemaphore GetNativeSemaphore() const;

private:

    VkDevice    device_;
    VkSemaphore semaphore_;
};

RTRC_RHI_VK_END
