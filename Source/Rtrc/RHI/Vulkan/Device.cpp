#include <fmt/format.h>
#include <Rtrc/RHI/Vulkan/Device.h>
#include <Rtrc/RHI/Vulkan/Queue.h>
#include <Rtrc/RHI/Vulkan/Surface.h>
#include <Rtrc/RHI/Vulkan/Swapchain.h>
#include <Rtrc/Utils/ScopeGuard.h>
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

VulkanDevice::VulkanDevice(VkInstance instance, VulkanPhysicalDevice physicalDevice, VkDevice device)
    : instance_(instance), physicalDevice_(std::move(physicalDevice)), device_(device)
{

}

VulkanDevice::~VulkanDevice()
{
    vkDestroyDevice(device_, VK_ALLOC);
}

Unique<Queue> VulkanDevice::GetQueue(QueueType type)
{
    // TODO
    return {};
}

Unique<Swapchain> VulkanDevice::CreateSwapchain(const SwapchainDescription &desc, Window &window)
{
    auto surface = DynamicCast<VulkanSurface>(window.CreateVulkanSurface(instance_));

    // surface capabilities

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice_.GetNativeHandle(), surface->GetSurface(), &surfaceCapabilities))
    {
        throw Exception("failed to get vulkan physical device surface capabilities");
    };

    // check format

    if(!CheckFormatSupport(physicalDevice_.GetNativeHandle(), surface->GetSurface(), TranslateTexelFormat(desc.format)))
    {
        throw Exception(fmt::format("surface format {} is not supported", GetTexelFormatName(desc.format)));
    }

    // present mode

    // TODO
    return {};
}

RTRC_RHI_VK_END
