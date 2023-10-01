#pragma once

#include <RHI/Vulkan/Common.h>

class VulkanCommandBuffer;

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTlasPrebuildInfo, TlasPrebuildInfo)
{
public:

    VulkanTlasPrebuildInfo(
        VulkanDevice                             *device,
        const RayTracingInstanceArrayDesc        &instances,
        RayTracingAccelerationStructureBuildFlags flags);

    const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

#if RTRC_DEBUG
    bool _internalIsCompatiableWith(const RayTracingInstanceArrayDesc &instances) const;
#endif

    void _internalBuildTlas(
        VulkanCommandBuffer               *commandBuffer,
        const RayTracingInstanceArrayDesc &instances,
        const TlasPtr                     &tlas,
        BufferDeviceAddress                scratchBuffer);

private:

#if RTRC_DEBUG
    RayTracingInstanceArrayDesc instances_;
#endif

    VulkanDevice *device_;
    VkAccelerationStructureGeometryKHR          vkGeometry_;
    RayTracingAccelerationStructurePrebuildInfo prebuildInfo_;
    VkAccelerationStructureBuildRangeInfoKHR    rangeInfo_;
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildGeometryInfo_;
};

RTRC_RHI_VK_END
