#pragma once

#include <optional>

#include <RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanPhysicalDevice
{
public:

    static VulkanPhysicalDevice Select(VkInstance instance, const DeviceDesc &desc);

    static std::vector<const char*> GetRequiredExtensions(const DeviceDesc &desc);
    
    static VkPhysicalDeviceFeatures2 GetRequiredFeatures(
        const DeviceDesc &desc, std::unique_ptr<unsigned char[]> &storage);

    VulkanPhysicalDevice();

    VulkanPhysicalDevice(VkPhysicalDevice device, bool enableRayTracing);

    std::optional<uint32_t> GetGraphicsQueueFamily() const;
    std::optional<uint32_t> GetComputeQueueFamily() const;
    std::optional<uint32_t> GetTransferQueueFamily() const;

    VkPhysicalDevice GetNativeHandle() const;
    const VkPhysicalDeviceProperties &GetNativeProperties() const;

    const VkPhysicalDeviceSubgroupProperties &_internalGetSubgroupProperties() const;
    const std::optional<VkPhysicalDeviceAccelerationStructurePropertiesKHR> &_internalGetASProperties() const;
    const std::optional<VkPhysicalDeviceRayTracingPipelinePropertiesKHR> &_internalGetRtPipelineProperties() const;

private:

    VkPhysicalDevice physicalDevice_;
    VkPhysicalDeviceProperties2 properties_;

    std::optional<uint32_t> graphicsQueueFamily_;
    std::optional<uint32_t> computeQueueFamily_;
    std::optional<uint32_t> transferQueueFamily_;

    VkPhysicalDeviceSubgroupProperties                                subgroupProperties_;
    std::optional<VkPhysicalDeviceAccelerationStructurePropertiesKHR> asProperties_;
    std::optional<VkPhysicalDeviceRayTracingPipelinePropertiesKHR>    rtPipelineProperties_;
};

RTRC_RHI_VK_END
