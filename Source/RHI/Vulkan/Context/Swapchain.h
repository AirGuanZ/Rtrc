#pragma once

#include <RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <RHI/Vulkan/Context/Surface.h>
#include <RHI/Vulkan/Queue/Queue.h>
#include <RHI/Vulkan/Resource/Texture.h>

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

    RPtr<BackBufferSemaphore> GetAcquireSemaphore() RTRC_RHI_OVERRIDE;

    RPtr<BackBufferSemaphore> GetPresentSemaphore() RTRC_RHI_OVERRIDE;

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
