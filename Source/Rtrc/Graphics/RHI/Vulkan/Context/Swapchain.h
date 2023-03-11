#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/Texture.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanSwapchain, Swapchain)
{
public:

    VulkanSwapchain(
        Ptr<VulkanSurface>   surface,
        Ptr<VulkanQueue>     presentQueue,
        const TextureDesc &imageDesc,
        VulkanDevice        *device,
        VkSwapchainKHR       swapchain);

    ~VulkanSwapchain() override;

    bool Acquire() RTRC_RHI_OVERRIDE;

    Ptr<BackBufferSemaphore> GetAcquireSemaphore() RTRC_RHI_OVERRIDE;

    Ptr<BackBufferSemaphore> GetPresentSemaphore() RTRC_RHI_OVERRIDE;

    bool Present() RTRC_RHI_OVERRIDE;

    int GetRenderTargetCount() const RTRC_RHI_OVERRIDE;

    const TextureDesc &GetRenderTargetDesc() const RTRC_RHI_OVERRIDE;

    Ptr<Texture> GetRenderTarget() const RTRC_RHI_OVERRIDE;

private:

    Ptr<VulkanSurface> surface_;
    Ptr<VulkanQueue>   presentQueue_;
    VulkanDevice      *device_;
    VkSwapchainKHR     swapchain_;

    uint32_t frameIndex_;
    uint32_t imageIndex_;
    std::vector<Ptr<VulkanTexture>> images_;
    std::vector<Ptr<VulkanBackBufferSemaphore>> imageAcquireSemaphores_;
    std::vector<Ptr<VulkanBackBufferSemaphore>> imagePresentSemaphores_;
};

RTRC_RHI_VK_END
