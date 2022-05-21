#pragma once

#include <Rtrc/RHI/Vulkan/Queue.h>
#include <Rtrc/RHI/Vulkan/Surface.h>

RTRC_RHI_VK_BEGIN

class VulkanSwapchain : public Swapchain
{
public:

    VulkanSwapchain(
        Unique<VulkanSurface> surface,
        Unique<VulkanQueue>   presentQueue,
        VkDevice              device,
        VkSwapchainKHR        swapchain);

    ~VulkanSwapchain() override;

    Shared<Texture2D> GetCurrentBackTexture() const override;

private:

    Unique<VulkanSurface> surface_;
    Unique<VulkanQueue>   presentQueue_;
    VkDevice              device_;
    VkSwapchainKHR        swapchain_;
};

RTRC_RHI_VK_END
