#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandBuffer;

RTRC_RHI_IMPLEMENT(VulkanBlasPrebuildInfo, BlasPrebuildInfo)
{
public:

    VulkanBlasPrebuildInfo(
        VulkanDevice *device, Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlags flags);

    const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

#if RTRC_DEBUG
    bool _internalIsCompatiableWith(Span<RayTracingGeometryDesc> geometries) const;
#endif

    void _internalBuildBlas(
        VulkanCommandBuffer         *commandBuffer,
        Span<RayTracingGeometryDesc> geometries,
        const BlasOPtr              &blas,
        BufferDeviceAddress          scratchBuffer);

private:

#if RTRC_DEBUG
    std::vector<RayTracingGeometryDesc> geometries_;
#endif

    VulkanDevice *device_;
    std::vector<VkAccelerationStructureGeometryKHR> vkGeometries_;
    RayTracingAccelerationStructurePrebuildInfo prebuildInfo_;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfo_;
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildGeometryInfo_;
};

RTRC_RHI_VK_END
