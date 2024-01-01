#pragma once

#include <Rtrc/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanSwapchain, Swapchain)
{
public:

    VulkanSwapchain(
        RPtr<VulkanSurface> surface,
        RPtr<VulkanQueue>   presentQueue,
        const TextureDesc &imageDesc,
        VulkanDevice      *device,
        VkSwapchainKHR     swapchain);

    ~VulkanSwapchain() override;

    bool Acquire() RTRC_RHI_OVERRIDE;

    OPtr<BackBufferSemaphore> GetAcquireSemaphore() RTRC_RHI_OVERRIDE;
    OPtr<BackBufferSemaphore> GetPresentSemaphore() RTRC_RHI_OVERRIDE;

    bool Present() RTRC_RHI_OVERRIDE;

    int GetRenderTargetCount() const RTRC_RHI_OVERRIDE;

    const TextureDesc &GetRenderTargetDesc() const RTRC_RHI_OVERRIDE;

    RPtr<Texture> GetRenderTarget() const RTRC_RHI_OVERRIDE;

private:

    RPtr<VulkanSurface> surface_;
    RPtr<VulkanQueue>   presentQueue_;
    VulkanDevice      *device_;
    VkSwapchainKHR     swapchain_;

    uint32_t frameIndex_;
    uint32_t imageIndex_;
    std::vector<RPtr<VulkanTexture>> images_;
    std::vector<RPtr<VulkanBackBufferSemaphore>> imageAcquireSemaphores_;
    std::vector<RPtr<VulkanBackBufferSemaphore>> imagePresentSemaphores_;
};

RTRC_RHI_VK_END
