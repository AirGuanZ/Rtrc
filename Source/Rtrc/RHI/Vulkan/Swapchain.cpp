#include <Rtrc/RHI/Vulkan/Swapchain.h>

RTRC_RHI_VK_BEGIN

VulkanSwapchain::VulkanSwapchain(
    Unique<VulkanSurface> surface,
    Unique<VulkanQueue>   presentQueue,
    VkDevice              device,
    VkSwapchainKHR        swapchain)
    : surface_(std::move(surface)), presentQueue_(std::move(presentQueue)), device_(device), swapchain_(swapchain)
{
    
}

VulkanSwapchain::~VulkanSwapchain()
{
    vkDestroySwapchainKHR(device_, swapchain_, VK_ALLOC);
}

Shared<Texture2D> VulkanSwapchain::GetCurrentBackTexture() const
{
    // TODO
    return {};
}

RTRC_RHI_VK_END
