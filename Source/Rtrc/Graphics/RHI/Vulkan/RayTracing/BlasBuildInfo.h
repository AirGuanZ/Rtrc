#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandBuffer;

RTRC_RHI_IMPLEMENT(VulkanBlasBuildInfo, BlasBuildInfo)
{
public:

    VulkanBlasBuildInfo(
        VulkanDevice *device, Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlag flags);

    RayTracingAccelerationStructurePrebuildInfo GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

    void _internalBuildBlas(
        VulkanCommandBuffer *commandBuffer, const BlasPtr &blas, BufferDeviceAddress scratchBuffer) const;

private:

    VulkanDevice *device_;
    std::vector<VkAccelerationStructureGeometryKHR> vkGeometries_;
    std::vector<uint32_t> vkPrimitiveCounts_;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfo_;
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildGeometryInfo_;
};

RTRC_RHI_VK_END
