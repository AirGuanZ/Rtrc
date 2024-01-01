#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/RHI/Vulkan/RayTracing/Tlas.h>
#include <Rtrc/RHI/Vulkan/RayTracing/TlasPrebuildInfo.h>

RTRC_RHI_VK_BEGIN

VulkanTlasPrebuildInfo::VulkanTlasPrebuildInfo(
    VulkanDevice                             *device,
    const RayTracingInstanceArrayDesc        &instances,
    RayTracingAccelerationStructureBuildFlags flags)
    : device_(device), vkGeometry_{}, prebuildInfo_{}, rangeInfo_{}, vkBuildGeometryInfo_{}
{
#if RTRC_DEBUG
    instances_ = instances;
#endif
    
    {
        auto &arr = instances;
        VkGeometryFlagsKHR geometryFlags = {};
        //if(arr.opaque)
        //{
        //    geometryFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
        //}
        //if(arr.noDuplicateAnyHitInvocation)
        //{
        //    geometryFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        //}
        
        vkGeometry_ = VkAccelerationStructureGeometryKHR
        {
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
            .geometry     = VkAccelerationStructureGeometryDataKHR
            {
                .instances = VkAccelerationStructureGeometryInstancesDataKHR
                {
                    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR
                }
            },
            .flags = geometryFlags
        };
    }

    vkBuildGeometryInfo_ = VkAccelerationStructureBuildGeometryInfoKHR
    {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags         = TranslateAccelerationStructureBuildFlags(flags),
        .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = 1,
        .pGeometries   = &vkGeometry_
    };

    rangeInfo_.primitiveCount  = instances.instanceCount;
    rangeInfo_.firstVertex     = 0;
    rangeInfo_.primitiveOffset = 0;
    rangeInfo_.transformOffset = 0;

    VkAccelerationStructureBuildSizesInfoKHR sizes =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };
    vkGetAccelerationStructureBuildSizesKHR(
        device_->_internalGetNativeDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &vkBuildGeometryInfo_, &instances.instanceCount,
        &sizes);

    prebuildInfo_ = RayTracingAccelerationStructurePrebuildInfo
    {
        .accelerationStructureSize = sizes.accelerationStructureSize,
        .updateScratchSize = sizes.updateScratchSize,
        .buildScratchSize = sizes.buildScratchSize
    };
}

const RayTracingAccelerationStructurePrebuildInfo &VulkanTlasPrebuildInfo::GetPrebuildInfo() const
{
    return prebuildInfo_;
}

#if RTRC_DEBUG

bool VulkanTlasPrebuildInfo::_internalIsCompatiableWith(const RayTracingInstanceArrayDesc &instances) const
{
    if(instances.instanceCount > instances_.instanceCount)
    {
        return false;
    }
    return true;
}

#endif // #if RTRC_DEBUG

void VulkanTlasPrebuildInfo::_internalBuildTlas(
    VulkanCommandBuffer               *commandBuffer,
    const RayTracingInstanceArrayDesc &instances,
    const TlasOPtr                    &tlas,
    BufferDeviceAddress                scratchBuffer)
{
#if RTRC_DEBUG
    if(!_internalIsCompatiableWith(instances))
    {
        throw Exception("Tlas build info is not compatible with instanceArrays");
    }
#endif

    vkGeometry_.geometry.instances.data.deviceAddress = instances.instanceData.address;
    rangeInfo_.primitiveCount = instances.instanceCount;
    
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = vkBuildGeometryInfo_;
    vkBuildInfo.dstAccelerationStructure = static_cast<VulkanTlas *>(tlas.Get())->_internalGetNativeTlas();
    vkBuildInfo.scratchData.deviceAddress = scratchBuffer.address;

    const VkAccelerationStructureBuildRangeInfoKHR *rangeInfoPointer = &rangeInfo_;

    auto vkCmdBuffer = commandBuffer->_internalGetNativeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(vkCmdBuffer, 1, &vkBuildInfo, &rangeInfoPointer);
}

RTRC_RHI_VK_END
