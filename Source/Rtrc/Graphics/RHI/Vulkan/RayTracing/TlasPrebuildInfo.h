#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

class VulkanCommandBuffer;

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanTlasPrebuildInfo, TlasPrebuildInfo)
{
public:

    VulkanTlasPrebuildInfo(
        VulkanDevice                             *device,
        Span<RayTracingInstanceArrayDesc>         instanceArrays,
        RayTracingAccelerationStructureBuildFlags flags);

    const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

#if RTRC_DEBUG
    bool _internalIsCompatiableWith(Span<RayTracingInstanceArrayDesc> instanceArrays) const;
#endif

    void _internalBuildTlas(
        VulkanCommandBuffer              *commandBuffer,
        Span<RayTracingInstanceArrayDesc> instanceArrays,
        const TlasPtr                    &tlas,
        BufferDeviceAddress               scratchBuffer);

private:

#if RTRC_DEBUG
    std::vector<RayTracingInstanceArrayDesc> instanceArrays_;
#endif

    VulkanDevice *device_;
    std::vector<VkAccelerationStructureGeometryKHR> vkGeometries_;
    RayTracingAccelerationStructurePrebuildInfo prebuildInfo_;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfo_;
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildGeometryInfo_;
};

RTRC_RHI_VK_END
