#pragma once

#include <Rtrc/RHI/Vulkan/PhysicalDevice.h>

RTRC_RHI_VK_BEGIN

class VulkanDevice : public Device
{
public:

    struct QueueFamilyInfo
    {
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> computeFamilyIndex;
        std::optional<uint32_t> transferFamilyIndex;
    };

    VulkanDevice(
        VkInstance             instance,
        VulkanPhysicalDevice   physicalDevice,
        VkDevice               device,
        const QueueFamilyInfo &queueFamilyInfo);

    ~VulkanDevice() override;

    Unique<Queue> GetQueue(QueueType type) override;

    Unique<Swapchain> CreateSwapchain(const SwapchainDescription &desc, Window &window) override;

private:

    VkInstance           instance_;
    VulkanPhysicalDevice physicalDevice_;
    VkDevice             device_;

    VkQueue graphicsQueue_;
    VkQueue computeQueue_;
    VkQueue transferQueue_;
    VkQueue presentQueue_;

    QueueFamilyInfo queueFamilies_;
};

RTRC_RHI_VK_END
