#pragma once

#include <Rtrc/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/RHI/Vulkan/Context/Surface.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>

RTRC_RHI_VK_BEGIN

class VulkanSwapchain : public Swapchain
{
public:

    VulkanSwapchain(
        RC<VulkanSurface>    surface,
        RC<VulkanQueue>      presentQueue,
        const Texture2DDesc &imageDesc,
        VkDevice             device,
        VkSwapchainKHR       swapchain);

    ~VulkanSwapchain() override;

    bool Acquire() override;

    RC<BackBufferSemaphore> GetAcquireSemaphore() override;

    RC<BackBufferSemaphore> GetPresentSemaphore() override;

    void Present() override;

    int GetRenderTargetCount() const override;

    const Texture2DDesc &GetRenderTargetDesc() const override;

    RC<Texture> GetRenderTarget() const override;

private:

    RC<VulkanSurface> surface_;
    RC<VulkanQueue>   presentQueue_;
    VkDevice          device_;
    VkSwapchainKHR    swapchain_;

    uint32_t frameIndex_;
    uint32_t imageIndex_;
    std::vector<RC<VulkanTexture2D>> images_;
    std::vector<RC<VulkanBackBufferSemaphore>> imageAcquireSemaphores_;
    std::vector<RC<VulkanBackBufferSemaphore>> imagePresentSemaphores_;
};

RTRC_RHI_VK_END
