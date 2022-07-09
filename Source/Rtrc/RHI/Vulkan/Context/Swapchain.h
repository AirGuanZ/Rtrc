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
        Ptr<VulkanSurface>   surface,
        Ptr<VulkanQueue>     presentQueue,
        const Texture2DDesc &imageDesc,
        VkDevice             device,
        VkSwapchainKHR       swapchain);

    ~VulkanSwapchain() override;

    bool Acquire() override;

    Ptr<BackBufferSemaphore> GetAcquireSemaphore() override;

    Ptr<BackBufferSemaphore> GetPresentSemaphore() override;

    void Present() override;

    int GetRenderTargetCount() const override;

    const Texture2DDesc &GetRenderTargetDesc() const override;

    Ptr<Texture> GetRenderTarget() const override;

private:

    Ptr<VulkanSurface> surface_;
    Ptr<VulkanQueue>   presentQueue_;
    VkDevice           device_;
    VkSwapchainKHR     swapchain_;

    uint32_t frameIndex_;
    uint32_t imageIndex_;
    std::vector<Ptr<VulkanTexture2D>> images_;
    std::vector<Ptr<VulkanBackBufferSemaphore>> imageAcquireSemaphores_;
    std::vector<Ptr<VulkanBackBufferSemaphore>> imagePresentSemaphores_;
};

RTRC_RHI_VK_END
