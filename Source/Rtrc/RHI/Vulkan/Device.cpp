#include <algorithm>
#include <map>

#include <fmt/format.h>

#include <Rtrc/RHI/Vulkan/Device.h>
#include <Rtrc/RHI/Vulkan/Queue.h>
#include <Rtrc/RHI/Vulkan/Surface.h>
#include <Rtrc/RHI/Vulkan/Swapchain.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_VK_BEGIN

namespace
{

    bool CheckFormatSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkFormat expectedFormat)
    {
        uint32_t supportedFormatCount;
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, nullptr))
        {
            throw Exception("failed to get vulkan physical device surface format count");
        };

        std::vector<VkSurfaceFormatKHR> supportedFormats(supportedFormatCount);
        VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice, surface, &supportedFormatCount, supportedFormats.data()))
        {
            throw Exception("failed to get vulkan physical device surface formats");
        };
        
        for(auto &f : supportedFormats)
        {
            if(f.format == expectedFormat && f.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
            {
                return true;
            }
        }
        return false;
    }

} // namespace anonymous

VulkanDevice::VulkanDevice(
    VkInstance             instance,
    VulkanPhysicalDevice   physicalDevice,
    VkDevice               device,
    const QueueFamilyInfo &queueFamilyInfo)
    : instance_(instance), physicalDevice_(physicalDevice), device_(device), queueFamilies_(queueFamilyInfo)
{
    std::map<uint32_t, uint32_t> familyIndexToNextQueueIndex;
    auto getNextQueue = [&](const std::optional<uint32_t> &familyIndex) -> VkQueue
    {
        if(!familyIndex)
        {
            return nullptr;
        }
        VkQueue queue;
        auto &queueIndex = familyIndexToNextQueueIndex[familyIndex.value()];
        vkGetDeviceQueue(device_, familyIndex.value(), queueIndex, &queue);
        ++queueIndex;
        return queue;
    };

    graphicsQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex);
    presentQueue_ = getNextQueue(queueFamilies_.graphicsFamilyIndex);
    computeQueue_ = getNextQueue(queueFamilies_.computeFamilyIndex);
    transferQueue_ = getNextQueue(queueFamilies_.transferFamilyIndex);
}

VulkanDevice::~VulkanDevice()
{
    vkDestroyDevice(device_, VK_ALLOC);
}

Unique<Queue> VulkanDevice::GetQueue(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return MakeUnique<VulkanQueue>(graphicsQueue_, *queueFamilies_.graphicsFamilyIndex);
    case QueueType::Compute: return MakeUnique<VulkanQueue>(computeQueue_, *queueFamilies_.computeFamilyIndex);
    case QueueType::Transfer: return MakeUnique<VulkanQueue>(transferQueue_, *queueFamilies_.transferFamilyIndex);
    }
    Unreachable();
}

Unique<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDescription &desc, Window &window)
{
    assert(presentQueue_);
    auto surface = DynamicCast<VulkanSurface>(window.CreateVulkanSurface(instance_));

    // surface capabilities

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice_.GetNativeHandle(), surface->GetSurface(), &surfaceCapabilities))
    {
        throw Exception("failed to get vulkan physical device surface capabilities");
    };

    // check format

    const auto requiredFormat = TranslateTexelFormat(desc.format);
    if(!CheckFormatSupport(physicalDevice_.GetNativeHandle(), surface->GetSurface(), requiredFormat))
    {
        throw Exception(fmt::format("surface format {} is not supported", GetTexelFormatName(desc.format)));
    }

    // present mode

    uint32_t supportedPresentModeCount;
    VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->GetSurface(), &supportedPresentModeCount, nullptr),
        "failed to get vulkan surface present mode count");

    std::vector<VkPresentModeKHR> supportedPresentModes(supportedPresentModeCount);
    VK_FAIL_MSG(
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physicalDevice_.GetNativeHandle(), surface->GetSurface(),
            &supportedPresentModeCount, supportedPresentModes.data()),
        "failed to get vulkan surface present modes");

    auto presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(auto p : supportedPresentModes)
    {
        if(p == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // extent

    VkExtent2D extent = surfaceCapabilities.currentExtent;
    if(extent.width == UINT32_MAX)
    {
        const auto [fbWidth, fbHeight] = window.GetFramebufferSize();
        extent.width = std::clamp(
            static_cast<uint32_t>(fbWidth),
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);
        extent.height = std::clamp(
            static_cast<uint32_t>(fbHeight),
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);
    }

    // image count

    const uint32_t imageCount = std::clamp(
        desc.imageCount,
        surfaceCapabilities.minImageCount,
        surfaceCapabilities.maxImageCount ? surfaceCapabilities.maxImageCount : UINT32_MAX);
    if(imageCount != desc.imageCount)
    {
        throw Exception("unsupported image count");
    }

    // swapchain

    const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
        .sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface               = surface->GetSurface(),
        .minImageCount         = imageCount,
        .imageFormat           = requiredFormat,
        .imageColorSpace       = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent           = extent,
        .imageArrayLayers      = 1,
        .imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices   = nullptr,
        .preTransform          = surfaceCapabilities.currentTransform,
        .compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode           = presentMode,
        .clipped               = true
    };

    VkSwapchainKHR swapchain;
    VK_FAIL_MSG(
        vkCreateSwapchainKHR(device_, &swapchainCreateInfo, VK_ALLOC, &swapchain),
        "failed to create vulkan swapchain");

    auto queue = MakeUnique<VulkanQueue>(presentQueue_, queueFamilies_.graphicsFamilyIndex.value());
    return MakeUnique<VulkanSwapchain>(std::move(surface), std::move(queue), device_, swapchain);
}

RTRC_RHI_VK_END
