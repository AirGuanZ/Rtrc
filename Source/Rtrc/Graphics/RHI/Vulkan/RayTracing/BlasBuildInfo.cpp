#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Blas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/BlasBuildInfo.h>

RTRC_RHI_VK_BEGIN

VulkanBlasBuildInfo::VulkanBlasBuildInfo(
    VulkanDevice *device, Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlag flags)
    : device_(device), vkBuildGeometryInfo_{}
{
    auto ToVkDeviceAddress = [&](BufferDeviceAddress address)
    {
        VkDeviceOrHostAddressConstKHR ret;
        ret.deviceAddress = address.address;
        return ret;
    };
    
    vkGeometries_.reserve(geometries.size());
    vkPrimitiveCounts_.reserve(geometries.size());
    for(auto &geometry : geometries)
    {
        vkPrimitiveCounts_.push_back(geometry.primitiveCount);

        VkAccelerationStructureGeometryDataKHR data;
        if(geometry.type == RayTracingGeometryType::Triangles)
        {
            data.triangles = VkAccelerationStructureGeometryTrianglesDataKHR
            {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat  = TranslateGeometryVertexFormat(geometry.vertexFormat),
                .vertexData    = ToVkDeviceAddress(geometry.vertexData),
                .vertexStride  = geometry.vertexStride,
                .maxVertex     = geometry.vertexCount,
                .indexType     = TranslateIndexFormat(geometry.indexFormat),
                .indexData     = ToVkDeviceAddress(geometry.indexData),
                .transformData = ToVkDeviceAddress(geometry.transformData)
            };
        }
        else
        {
            data.aabbs = VkAccelerationStructureGeometryAabbsDataKHR
            {
                .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                .data   = ToVkDeviceAddress(geometry.aabbData),
                .stride = geometry.aabbStride
            };
        }
        const VkGeometryTypeKHR type = TranslateGeometryType(geometry.type);
        vkGeometries_.push_back(VkAccelerationStructureGeometryKHR
        {
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = type,
            .geometry     = data
        });
    }

    vkBuildGeometryInfo_ = VkAccelerationStructureBuildGeometryInfoKHR
    {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags         = TranslateAccelerationStructureBuildFlags(flags),
        .mode          = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
        .geometryCount = static_cast<uint32_t>(vkGeometries_.size()),
        .pGeometries   = vkGeometries_.data()
    };

    rangeInfo_.resize(vkPrimitiveCounts_.size());
    for(size_t i = 0; i < vkPrimitiveCounts_.size(); ++i)
    {
        rangeInfo_[i].primitiveCount = vkPrimitiveCounts_[i];
        rangeInfo_[i].firstVertex = 0;
        rangeInfo_[i].primitiveOffset = 0;
        rangeInfo_[i].transformOffset = 0;
    }
}

RayTracingAccelerationStructurePrebuildInfo VulkanBlasBuildInfo::GetPrebuildInfo() const
{
    VkAccelerationStructureBuildSizesInfoKHR sizes;
    vkGetAccelerationStructureBuildSizesKHR(
        device_->_internalGetNativeDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &vkBuildGeometryInfo_, vkPrimitiveCounts_.data(),
        &sizes);

    return RayTracingAccelerationStructurePrebuildInfo
    {
        .accelerationStructureSize = sizes.accelerationStructureSize,
        .updateScratchSize         = sizes.updateScratchSize,
        .buildScratchSize          = sizes.buildScratchSize
    };
}

void VulkanBlasBuildInfo::_internalBuildBlas(
    VulkanCommandBuffer *commandBuffer, const BlasPtr &blas, BufferDeviceAddress scratchBuffer) const
{
    VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = vkBuildGeometryInfo_;
    vkBuildInfo.dstAccelerationStructure = static_cast<VulkanBlas *>(blas.Get())->_internalGetNativeBlas();
    vkBuildInfo.scratchData.deviceAddress = scratchBuffer.address;
    
    const VkAccelerationStructureBuildRangeInfoKHR *rangeInfoPointer = rangeInfo_.data();

    auto vkCmdBuffer = commandBuffer->_internalGetNativeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(vkCmdBuffer, 1, &vkBuildInfo, &rangeInfoPointer);
}

RTRC_RHI_VK_END
