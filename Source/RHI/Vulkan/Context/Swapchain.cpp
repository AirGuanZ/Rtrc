#include <Core/ScopeGuard.h>
#include <RHI/Vulkan/Context/Device.h>
#include <RHI/Vulkan/Context/Swapchain.h>
#include <RHI/Vulkan/Resource/Texture.h>

RTRC_RHI_VK_BEGIN

VulkanSwapchain::VulkanSwapchain(
    RPtr<VulkanSurface> surface,
    RPtr<VulkanQueue>   presentQueue,
    const TextureDesc &imageDesc,
    VulkanDevice      *device,
    VkSwapchainKHR     swapchain)
    : surface_(std::move(surface)), presentQueue_(std::move(presentQueue)), device_(device), swapchain_(swapchain)
{
    // images

    uint32_t imageCount;
    RTRC_VK_FAIL_MSG(
        vkGetSwapchainImagesKHR(device_->_internalGetNativeDevice(), swapchain_, &imageCount, nullptr),
        "failed to get vulkan swapchain image count");

    std::vector<VkImage> images(imageCount);
    RTRC_VK_FAIL_MSG(
        vkGetSwapchainImagesKHR(device_->_internalGetNativeDevice(), swapchain_, &imageCount, images.data()),
        "failed to get vulkan swapchain images");

    for(auto vkImage : images)
    {
        auto image = MakeRPtr<VulkanTexture>(
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
            RTRC_VK_FAIL_MSG(
                vkCreateSemaphore(device_->_internalGetNativeDevice(), &semaphoreCreateInfo, RTRC_VK_ALLOC, &semaphore),
                "failed to create vulkan semaphores for swapchain");
            RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_->_internalGetNativeDevice(), semaphore, RTRC_VK_ALLOC); };
            imageAcquireSemaphores_.push_back(
                MakeRPtr<VulkanBackBufferSemaphore>(device_->_internalGetNativeDevice(), semaphore));
        }

        {
            RTRC_VK_FAIL_MSG(
                vkCreateSemaphore(device_->_internalGetNativeDevice(), &semaphoreCreateInfo, RTRC_VK_ALLOC, &semaphore),
                "failed to create vulkan semaphores for swapchain");
            RTRC_SCOPE_FAIL{ vkDestroySemaphore(device_->_internalGetNativeDevice(), semaphore, RTRC_VK_ALLOC); };
            imagePresentSemaphores_.push_back(
                MakeRPtr<VulkanBackBufferSemaphore>(device_->_internalGetNativeDevice(), semaphore));
        }
    }

    frameIndex_ = 0;
    imageIndex_ = 0;
}

VulkanSwapchain::~VulkanSwapchain()
{
    vkDestroySwapchainKHR(device_->_internalGetNativeDevice(), swapchain_, RTRC_VK_ALLOC);
}

bool VulkanSwapchain::Acquire()
{
    frameIndex_ = (frameIndex_ + 1) % static_cast<uint32_t>(images_.size());

    auto &imageAcquireSemaphore = imageAcquireSemaphores_[frameIndex_];
    const VkResult acquireResult = vkAcquireNextImageKHR(
        device_->_internalGetNativeDevice(), swapchain_, UINT64_MAX,
        imageAcquireSemaphore->_internalGetBinarySemaphore(), VK_NULL_HANDLE, &imageIndex_);

    if(acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
    {
        return false;
    }

    if(acquireResult == VK_SUCCESS || acquireResult == VK_SUBOPTIMAL_KHR)
    {
        return true;
    }

    throw Exception("Failed to acquire vulkan swapchain image");
}

OPtr<BackBufferSemaphore> VulkanSwapchain::GetAcquireSemaphore()
{
    return OPtr<BackBufferSemaphore>(imageAcquireSemaphores_[frameIndex_].Get());
}

OPtr<BackBufferSemaphore> VulkanSwapchain::GetPresentSemaphore()
{
    return OPtr<BackBufferSemaphore>(imagePresentSemaphores_[frameIndex_].Get());
}

bool VulkanSwapchain::Present()
{
    const auto presentSemaphore = imagePresentSemaphores_[frameIndex_]->_internalGetBinarySemaphore();
    const VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &presentSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &swapchain_,
        .pImageIndices      = &imageIndex_
    };
    return vkQueuePresentKHR(presentQueue_->_internalGetNativeQueue(), &presentInfo) == VK_SUCCESS;
}

int VulkanSwapchain::GetRenderTargetCount() const
{
    return static_cast<int>(images_.size());
}

const TextureDesc &VulkanSwapchain::GetRenderTargetDesc() const
{
    return images_.front()->GetDesc();
}

RPtr<Texture> VulkanSwapchain::GetRenderTarget() const
{
    return images_[imageIndex_];
}

RTRC_RHI_VK_END
