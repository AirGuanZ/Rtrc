#pragma once

#include <optional>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>
#include <Rtrc/Utility/Arena.h>

RTRC_RHI_VK_BEGIN

class VulkanPhysicalDevice
{
public:

    static VulkanPhysicalDevice Select(VkInstance instance, const DeviceDesc &desc);

    static std::vector<const char*> GetRequiredExtensions(const DeviceDesc &desc);
    
    static VkPhysicalDeviceFeatures2 GetRequiredFeatures(
        const DeviceDesc &desc, std::unique_ptr<unsigned char[]> &storage);

    explicit VulkanPhysicalDevice(VkPhysicalDevice device = nullptr);

    std::optional<uint32_t> GetGraphicsQueueFamily() const;
    std::optional<uint32_t> GetComputeQueueFamily() const;
    std::optional<uint32_t> GetTransferQueueFamily() const;

    VkPhysicalDevice GetNativeHandle() const;
    const VkPhysicalDeviceProperties &GetNativeProperties() const;

private:

    VkPhysicalDevice physicalDevice_;
    VkPhysicalDeviceProperties properties_;

    std::optional<uint32_t> graphicsQueueFamily_;
    std::optional<uint32_t> computeQueueFamily_;
    std::optional<uint32_t> transferQueueFamily_;
};

RTRC_RHI_VK_END
