#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Tlas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/TlasPrebuildInfo.h>

RTRC_RHI_VK_BEGIN

VulkanTlasPrebuildInfo::VulkanTlasPrebuildInfo(
    VulkanDevice                            *device,
    Span<RayTracingInstanceArrayDesc>        instanceArrays,
    RayTracingAccelerationStructureBuildFlag flags)
    : device_(device), prebuildInfo_{}, vkBuildGeometryInfo_{}
{
#if RTRC_DEBUG
    instanceArrays_ = { instanceArrays.begin(), instanceArrays.end() };
#endif

    std::vector<uint32_t> vkPrimitiveCounts;

    vkGeometries_.reserve(instanceArrays.size());
    vkPrimitiveCounts.reserve(instanceArrays.size());
    for(const RayTracingInstanceArrayDesc &arr : instanceArrays)
    {
        VkGeometryFlagsKHR geometryFlags = {};
        if(arr.opaque)
        {
            geometryFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
        }
        if(arr.noDuplicateAnyHitInvocation)
        {
            geometryFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        }

        vkPrimitiveCounts.push_back(arr.instanceCount);
        vkGeometries_.push_back(VkAccelerationStructureGeometryKHR
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
        });
    }

    vkBuildGeometryInfo_ = VkAccelerationStructureBuildGeometryInfoKHR
    {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type          = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        .flags         = TranslateAccelerationStructureBuildFlags(flags),
        .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = static_cast<uint32_t>(vkGeometries_.size()),
        .pGeometries   = vkGeometries_.data()
    };

    rangeInfo_.resize(vkPrimitiveCounts.size());
    for(size_t i = 0; i < vkPrimitiveCounts.size(); ++i)
    {
        rangeInfo_[i].primitiveCount = vkPrimitiveCounts[i];
        rangeInfo_[i].firstVertex = 0;
        rangeInfo_[i].primitiveOffset = 0;
        rangeInfo_[i].transformOffset = 0;
    }

    VkAccelerationStructureBuildSizesInfoKHR sizes =
    {
        .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR
    };
    vkGetAccelerationStructureBuildSizesKHR(
        device_->_internalGetNativeDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &vkBuildGeometryInfo_, vkPrimitiveCounts.data(),
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

bool VulkanTlasPrebuildInfo::_internalIsCompatiableWith(Span<RayTracingInstanceArrayDesc> instanceArrays) const
{
    if(instanceArrays.size() != instanceArrays_.size())
    {
        return false;
    }
    for(uint32_t i = 0; i < instanceArrays.size(); ++i)
    {
        if(instanceArrays[i].instanceCount > instanceArrays_[i].instanceCount)
        {
            return false;
        }
    }
    return true;
}

#endif // #if RTRC_DEBUG

void VulkanTlasPrebuildInfo::_internalBuildTlas(
    VulkanCommandBuffer              *commandBuffer,
    Span<RayTracingInstanceArrayDesc> instanceArrays,
    const TlasPtr                    &tlas,
    BufferDeviceAddress               scratchBuffer)
{
#if RTRC_DEBUG
    if(!_internalIsCompatiableWith(instanceArrays))
    {
        throw Exception("Tlas build info is not compatible with instanceArrays");
    }
#endif

    for(uint32_t i = 0; i < instanceArrays.size(); ++i)
    {
        vkGeometries_[i].geometry.instances.data.deviceAddress = instanceArrays[i].instanceData.address;
        rangeInfo_[i].primitiveCount = instanceArrays[i].instanceCount;
    }

    VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = vkBuildGeometryInfo_;
    vkBuildInfo.dstAccelerationStructure = static_cast<VulkanTlas *>(tlas.Get())->_internalGetNativeTlas();
    vkBuildInfo.scratchData.deviceAddress = scratchBuffer.address;

    const VkAccelerationStructureBuildRangeInfoKHR *rangeInfoPointer = rangeInfo_.data();

    auto vkCmdBuffer = commandBuffer->_internalGetNativeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(vkCmdBuffer, 1, &vkBuildInfo, &rangeInfoPointer);
}

RTRC_RHI_VK_END
