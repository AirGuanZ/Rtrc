#include <span>

#include <Rtrc/Graphics/RHI/Vulkan/Context/PhysicalDevice.h>

RTRC_RHI_VK_BEGIN

namespace VkPhysicalDeviceDetail
{

    bool IsSuitable(VkPhysicalDevice device, const DeviceDesc &desc)
    {
        // Api version

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);
        if(properties.apiVersion < RTRC_VULKAN_API_VERSION)
        {
            return false;
        }

        // Basic extensions

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

        // Features

        VkPhysicalDeviceCustomBorderColorFeaturesEXT customBorderColorFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT
        };
        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
            .pNext = &customBorderColorFeatures
        };
        VkPhysicalDeviceSynchronization2Features sync2Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
            .pNext = &dynamicRenderingFeatures
        };
        VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures =
        {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES,
            .pNext = &sync2Features,
        };
        VkPhysicalDeviceFeatures2 feature2 = {};
        feature2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        feature2.pNext = &descriptorIndexingFeatures;

        vkGetPhysicalDeviceFeatures2(device, &feature2);

        const bool customBorderColor = customBorderColorFeatures.customBorderColors &&
                                       customBorderColorFeatures.customBorderColorWithoutFormat;

        const bool bindless = descriptorIndexingFeatures.descriptorBindingPartiallyBound &&
                              descriptorIndexingFeatures.runtimeDescriptorArray &&
                              descriptorIndexingFeatures.descriptorBindingVariableDescriptorCount &&
                              descriptorIndexingFeatures.descriptorBindingUpdateUnusedWhilePending &&
                              descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing;

        if(!customBorderColor ||
           !dynamicRenderingFeatures.dynamicRendering ||
           !sync2Features.synchronization2 ||
           !bindless)
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

} // namespace VkPhysicalDeviceDetail

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
        "failed to get vulkan physical devices");

    uint32_t bestDeviceIndex = std::numeric_limits<uint32_t>::max();
    uint32_t bestScore = 0;
    for(uint32_t i = 0; i < deviceCount; ++i)
    {
        if(!VkPhysicalDeviceDetail::IsSuitable(devices[i], desc))
            continue;

        const uint32_t newScore = VkPhysicalDeviceDetail::RatePhysicalDevice(devices[i]);
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
        // The following three extensions has been promoted to core 1.2/1.3
        // VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        // VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME
        // VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME
    };
    if(desc.supportSwapchain)
    {
        requiredExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    return requiredExtensions;
}

VkPhysicalDeviceFeatures2* VulkanPhysicalDevice::GetRequiredFeatures(ObjectReleaser &arena)
{
    auto customBorderColorFeature = arena.Create<VkPhysicalDeviceCustomBorderColorFeaturesEXT>();
    customBorderColorFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT;
    customBorderColorFeature->customBorderColors = true;
    customBorderColorFeature->customBorderColorWithoutFormat = true;

    auto dynamicRenderingFeature = arena.Create<VkPhysicalDeviceDynamicRenderingFeatures>();
    dynamicRenderingFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeature->dynamicRendering = true;

    auto sync2Feature = arena.Create<VkPhysicalDeviceSynchronization2Features>();
    sync2Feature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Feature->synchronization2 = true;

    auto bindlessFeature = arena.Create<VkPhysicalDeviceDescriptorIndexingFeatures>();
    std::memset(bindlessFeature, 0, sizeof(VkPhysicalDeviceDescriptorIndexingFeatures));
    bindlessFeature->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
    bindlessFeature->descriptorBindingPartiallyBound = true;
    bindlessFeature->runtimeDescriptorArray = true;
    bindlessFeature->descriptorBindingVariableDescriptorCount = true;
    bindlessFeature->descriptorBindingUpdateUnusedWhilePending = true;
    bindlessFeature->shaderSampledImageArrayNonUniformIndexing = true;

    auto features2 = arena.Create<VkPhysicalDeviceFeatures2>();
    std::memset(features2, 0, sizeof(VkPhysicalDeviceFeatures2));
    features2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

    customBorderColorFeature->pNext = nullptr;
    dynamicRenderingFeature ->pNext = customBorderColorFeature;
    sync2Feature            ->pNext = dynamicRenderingFeature;
    bindlessFeature         ->pNext = sync2Feature;
    features2               ->pNext = bindlessFeature;

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

const VkPhysicalDeviceProperties &VulkanPhysicalDevice::GetNativeProperties() const
{
    return properties_;
}

VulkanPhysicalDevice::VulkanPhysicalDevice(VkPhysicalDevice device)
    : physicalDevice_(device), properties_{}
{
    if(!device)
    {
        return;
    }

    vkGetPhysicalDeviceProperties(physicalDevice_, &properties_);

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
