#include <span>

#include <Rtrc/RHI/Vulkan/Context/PhysicalDevice.h>

RTRC_RHI_VK_BEGIN

namespace
{

    bool IsSuitable(VkPhysicalDevice device, const DeviceDesc &desc)
    {
        // api version

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if(properties.apiVersion < RTRC_VULKAN_API_VERSION)
        {
            return false;
        }

        // basic extensions

        uint32_t supportedExtensionCount = 0;
        VK_FAIL_MSG(
            vkEnumerateDeviceExtensionProperties(device, nullptr, &supportedExtensionCount, nullptr),
            "failed to get vulkan device extension property count");

        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        VK_FAIL_MSG(
            vkEnumerateDeviceExtensionProperties(
                device, nullptr, &supportedExtensionCount, supportedExtensions.data()),
            "failed to get vulkan device extension properties");

        std::set<std::string> supportedExtensionNames;
        for(auto &e : supportedExtensions)
        {
            supportedExtensionNames.insert(e.extensionName);
        }

        auto requiredExtensions = VulkanPhysicalDevice::GetRequiredExtensions(desc);
        for(auto &required : requiredExtensions)
        {
            if(!supportedExtensionNames.contains(required))
            {
                return false;
            }
        }

        // features

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .dynamicRendering = true
        };
        VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES,
            .pNext = &dynamicRenderingFeatures,
            .timelineSemaphore = true
        };
        VkPhysicalDeviceSynchronization2Features sync2Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .pNext = &timelineSemaphoreFeatures,
            .synchronization2 = true
        };
        VkPhysicalDeviceFeatures2 feature2 = {};
        feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        feature2.pNext = &sync2Features;

        vkGetPhysicalDeviceFeatures2(device, &feature2);
        if(!dynamicRenderingFeatures.dynamicRendering ||
           !timelineSemaphoreFeatures.timelineSemaphore ||
           !sync2Features.synchronization2)
        {
            return false;
        }

        return true;
    }

    uint32_t RatePhysicalDevice(VkPhysicalDevice device)
    {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        uint32_t result = 0;
        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            result += 10000;
        }
        else if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            result += 2000;
        }
        result += properties.limits.maxImageDimension2D;
        return result;
    }

} // namespace anonymous

VulkanPhysicalDevice VulkanPhysicalDevice::Select(VkInstance instance, const DeviceDesc &desc)
{
    uint32_t deviceCount;
    VK_FAIL_MSG(
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
        "failed to get vulkan physical device count");
    if(!deviceCount)
    {
        return VulkanPhysicalDevice();
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    VK_FAIL_MSG(
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
        "fauled to get vulkan physical devices");

    uint32_t bestDeviceIndex = std::numeric_limits<uint32_t>::max();
    uint32_t bestScore = 0;
    for(uint32_t i = 0; i < deviceCount; ++i)
    {
        if(!IsSuitable(devices[i], desc))
            continue;

        const uint32_t newScore = RatePhysicalDevice(devices[i]);
        if(newScore > bestScore)
        {
            bestDeviceIndex = i;
            bestScore = newScore;
        }
    }

    return VulkanPhysicalDevice(bestDeviceIndex < deviceCount ? devices[bestDeviceIndex] : nullptr);
}

std::vector<const char*> VulkanPhysicalDevice::GetRequiredExtensions(const DeviceDesc &desc)
{
    std::vector<const char*> requiredExtensions =
    {
        VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
    };
    if(desc.supportSwapchain)
    {
        requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    return requiredExtensions;
}

VkPhysicalDeviceFeatures2* VulkanPhysicalDevice::GetRequiredFeatures(ObjectReleaser &arena)
{
    auto dynamicRenderingFeature = arena.Create<VkPhysicalDeviceDynamicRenderingFeatures>();
    dynamicRenderingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature->dynamicRendering = VK_TRUE;

    auto timelineFeature = arena.Create<VkPhysicalDeviceTimelineSemaphoreFeatures>();
    timelineFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
    timelineFeature->timelineSemaphore = VK_TRUE;

    auto sync2Feature = arena.Create<VkPhysicalDeviceSynchronization2Features>();
    sync2Feature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Feature->synchronization2 = VK_TRUE;

    auto features2 = arena.Create<VkPhysicalDeviceFeatures2>();
    std::memset(features2, 0, sizeof(VkPhysicalDeviceFeatures2));
    features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    dynamicRenderingFeature->pNext = nullptr;
    timelineFeature->pNext = dynamicRenderingFeature;
    sync2Feature->pNext = timelineFeature;
    features2->pNext = sync2Feature;

    return features2;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetGraphicsQueueFamily() const
{
    return graphicsQueueFamily_;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetComputeQueueFamily() const
{
    return computeQueueFamily_;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetTransferQueueFamily() const
{
    return transferQueueFamily_;
}

VkPhysicalDevice VulkanPhysicalDevice::GetNativeHandle() const
{
    return physicalDevice_;
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice device)
    : physicalDevice_(device)
{
    if(!device)
    {
        return;
    }

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for(uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        const auto flag = queueFamilies[i].queueFlags;
        const bool graphics = (flag & VK_QUEUE_GRAPHICS_BIT) != 0;
        const bool compute = (flag & VK_QUEUE_COMPUTE_BIT) != 0;
        const bool transfer = (flag & VK_QUEUE_TRANSFER_BIT) != 0;

        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamily_ = i;
        }
        if((!computeQueueFamily_ && compute) || (compute && !graphics))
        {
            computeQueueFamily_ = i;
        }
        if((!transferQueueFamily_ && transfer) || (transfer && !compute))
        {
            transferQueueFamily_ = i;
        }
    }
}

RTRC_RHI_VK_END
