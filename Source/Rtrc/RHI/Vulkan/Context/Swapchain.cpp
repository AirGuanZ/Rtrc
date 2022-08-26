#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Context/Swapchain.h>
#include <Rtrc/RHI/Vulkan/Resource/Texture2D.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanSwapchain::VulkanSwapchain(
    Ptr<VulkanSurface>   surface,
    Ptr<VulkanQueue>     presentQueue,
    const Texture2DDesc &imageDesc,
    VulkanDevice        *device,
    VkSwapchainKHR       swapchain)
    : surface_(std::move(surface)), presentQueue_(std::move(presentQueue)), device_(device), swapchain_(swapchain)
{
    // images

    uint32_t imageCount;
    VK_FAIL_MSG(
        vkGetSwapchainImagesKHR(device_->GetNativeDevice(), swapchain_, &imageCount, nullptr),
        "failed to get vulkan swapchain image count");

    std::vector<VkImage> images(imageCount);
    VK_FAIL_MSG(
        vkGetSwapchainImagesKHR(device_->GetNativeDevice(), swapchain_, &imageCount, images.data()),
        "failed to get vulkan swapchain images");

    for(auto vkImage : images)
    {
        auto image = MakePtr<VulkanTexture2D>(
            imageDesc, device_, vkImage,
            VulkanMemoryAllocation{ nullptr, nullptr }, ResourceOwnership::None);
        image->SetName("SwapchainImage");
        images_.push_back(std::move(image));
    }

    // back buffer semaphores

    const VkSemaphoreCreateInfo semaphoreCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    for(uint32_t i = 0; i < imageCount; ++i)
    {
        VkSemaphore semaphore;

        {
            VK_FAIL_MSG(
                vkCreateSemaphore(device_->GetNativeDevice(), &semaphoreCreateInfo, VK_ALLOC, &semaphore),
                "failed to create vulkan semaphores for swapchain");
            RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_->GetNativeDevice(), semaphore, VK_ALLOC); };
            imageAcquireSemaphores_.push_back(
                MakePtr<VulkanBackBufferSemaphore>(device_->GetNativeDevice(), semaphore));
        }

        {
            VK_FAIL_MSG(
                vkCreateSemaphore(device_->GetNativeDevice(), &semaphoreCreateInfo, VK_ALLOC, &semaphore),
                "failed to create vulkan semaphores for swapchain");
            RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_->GetNativeDevice(), semaphore, VK_ALLOC); };
            imagePresentSemaphores_.push_back(
                MakePtr<VulkanBackBufferSemaphore>(device_->GetNativeDevice(), semaphore));
        }
    }

    frameIndex_ = 0;
    imageIndex_ = 0;
}

VulkanSwapchain::~VulkanSwapchain()
{
    vkDestroySwapchainKHR(device_->GetNativeDevice(), swapchain_, VK_ALLOC);
}

bool VulkanSwapchain::Acquire()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<uint32_t>(images_.size());

    auto &imageAcquireSemaphore = imageAcquireSemaphores_[frameIndex_];
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device_->GetNativeDevice(), swapchain_, UINT64_MAX,
        imageAcquireSemaphore->GetBinarySemaphore(), nullptr, &imageIndex_);

    if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }

    if(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return true;
    }

    throw Exception("failed to acquire vulkan swapchain image");
}

Ptr<BackBufferSemaphore> VulkanSwapchain::GetAcquireSemaphore()
{
    return imageAcquireSemaphores_[frameIndex_];
}

Ptr<BackBufferSemaphore> VulkanSwapchain::GetPresentSemaphore()
{
    return imagePresentSemaphores_[frameIndex_];
}

void VulkanSwapchain::Present()
{
    const auto presentSemaphore = imagePresentSemaphores_[frameIndex_]->GetBinarySemaphore();
    const VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &presentSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain_,
        .pImageIndices      = &imageIndex_
    };
    (void)vkQueuePresentKHR(presentQueue_->GetNativeQueue(), &presentInfo);
}

int VulkanSwapchain::GetRenderTargetCount() const
{
    return static_cast<int>(images_.size());
}

const Texture2DDesc &VulkanSwapchain::GetRenderTargetDesc() const
{
    return images_.front()->GetDesc();
}

Ptr<Texture2D> VulkanSwapchain::GetRenderTarget() const
{
    return images_[imageIndex_];
}

RTRC_RHI_VK_END
