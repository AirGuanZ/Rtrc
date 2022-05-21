#pragma once

#include <Rtrc/RHI/Vulkan/PhysicalDevice.h>

RTRC_RHI_VK_BEGIN

class VulkanDevice : public Device
{
public:

    VulkanDevice(VkInstance instance, VulkanPhysicalDevice physicalDevice, VkDevice device);

    ~VulkanDevice() override;

    Unique<Queue> GetQueue(QueueType type) override;

    Unique<Swapchain> CreateSwapchain(const SwapchainDescription &desc, Window &window) override;

private:

    VkInstance           instance_;
    VulkanPhysicalDevice physicalDevice_;
    VkDevice             device_;
};

RTRC_RHI_VK_END
