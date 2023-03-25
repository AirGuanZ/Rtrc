#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandBuffer.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/Blas.h>
#include <Rtrc/Graphics/RHI/Vulkan/RayTracing/BlasPrebuildInfo.h>

RTRC_RHI_VK_BEGIN

VulkanBlasPrebuildInfo::VulkanBlasPrebuildInfo(
    VulkanDevice                             *device,
    Span<RayTracingGeometryDesc>              geometries,
    RayTracingAccelerationStructureBuildFlags flags)
    : device_(device), prebuildInfo_{}, vkBuildGeometryInfo_ {}
{
#if RTRC_DEBUG
    geometries_ = { geometries.begin(), geometries.end() };
#endif

    std::vector<uint32_t> vkPrimitiveCounts;

    vkGeometries_.reserve(geometries.size());
    vkPrimitiveCounts.reserve(geometries.size());
    for(auto &geometry : geometries)
    {
        vkPrimitiveCounts.push_back(geometry.primitiveCount);

        VkAccelerationStructureGeometryDataKHR data;
        if(geometry.type == RayTracingGeometryType::Triangles)
        {
            data.triangles = VkAccelerationStructureGeometryTrianglesDataKHR
            {
                .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
                .vertexFormat  = TranslateGeometryVertexFormat(geometry.trianglesData.vertexFormat),
                .vertexStride  = geometry.trianglesData.vertexStride,
                .maxVertex     = geometry.trianglesData.vertexCount,
                .indexType     = TranslateRayTracingIndexType(geometry.trianglesData.indexFormat)
            };
            // Dummy address for null-checking
            data.triangles.transformData.deviceAddress = geometry.trianglesData.hasTransform ? 1 : 0;
        }
        else
        {
            data.aabbs = VkAccelerationStructureGeometryAabbsDataKHR
            {
                .sType  = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
                .stride = geometry.proceduralData.aabbStride
            };
        }

        VkGeometryFlagsKHR geometryFlags = {};
        if(geometry.opaque)
        {
            geometryFlags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
        }
        if(geometry.noDuplicateAnyHitInvocation)
        {
            geometryFlags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
        }

        const VkGeometryTypeKHR type = TranslateGeometryType(geometry.type);
        vkGeometries_.push_back(VkAccelerationStructureGeometryKHR
        {
            .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
            .geometryType = type,
            .geometry     = data,
            .flags        = geometryFlags
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

const RayTracingAccelerationStructurePrebuildInfo &VulkanBlasPrebuildInfo::GetPrebuildInfo() const
{
    return prebuildInfo_;
}

#if RTRC_DEBUG

bool VulkanBlasPrebuildInfo::_internalIsCompatiableWith(Span<RayTracingGeometryDesc> geometries) const
{
    if(geometries.size() != geometries_.size())
    {
        return false;
    }
    for(uint32_t i = 0; i < geometries.size(); ++i)
    {
        if(geometries[i].type != geometries_[i].type)
        {
            return false;
        }
        if(geometries[i].primitiveCount > geometries_[i].primitiveCount)
        {
            return false;
        }

        if(geometries[i].type == RayTracingGeometryType::Triangles)
        {
            const RayTracingTrianglesGeometryData &data1 = geometries[i].trianglesData;
            const RayTracingTrianglesGeometryData &data2 = geometries_[i].trianglesData;
            if(data1.vertexFormat != data2.vertexFormat ||
               data1.vertexStride != data2.vertexStride ||
               data1.vertexCount > data2.vertexCount ||
               data1.indexFormat != data2.indexFormat ||
               data1.hasTransform != data2.hasTransform)
            {
                return false;
            }
        }
        else if(geometries[i].proceduralData.aabbStride != geometries_[i].proceduralData.aabbStride)
        {
            return false;
        }
    }
    return true;
}

#endif // #if RTRC_DEBUG

void VulkanBlasPrebuildInfo::_internalBuildBlas(
    VulkanCommandBuffer         *commandBuffer,
    Span<RayTracingGeometryDesc> geometries,
    const BlasPtr               &blas,
    BufferDeviceAddress          scratchBuffer)
{
#if RTRC_DEBUG
    if(!_internalIsCompatiableWith(geometries))
    {
        throw Exception("Blas build info is not compatible with geometries");
    }
#endif

    auto ToVkDeviceAddress = [&](BufferDeviceAddress address)
    {
        VkDeviceOrHostAddressConstKHR ret;
        ret.deviceAddress = address.address;
        return ret;
    };

    for(uint32_t i = 0; i < geometries.size(); ++i)
    {
        const RayTracingGeometryDesc &geometry = geometries[i];
        if(geometry.type == RayTracingGeometryType::Triangles)
        {
            VkAccelerationStructureGeometryTrianglesDataKHR &data = vkGeometries_[i].geometry.triangles;
            data.maxVertex = geometry.trianglesData.vertexCount;
            data.vertexData = ToVkDeviceAddress(geometry.trianglesData.vertexData);
            data.indexData = ToVkDeviceAddress(geometry.trianglesData.indexData);
            assert((data.transformData.deviceAddress != 0) == geometry.trianglesData.hasTransform);
            if(geometry.trianglesData.hasTransform)
            {
                data.transformData = ToVkDeviceAddress(geometry.trianglesData.transformData);
            }
        }
        else
        {
            VkAccelerationStructureGeometryAabbsDataKHR &data = vkGeometries_[i].geometry.aabbs;
            data.data = ToVkDeviceAddress(geometry.proceduralData.aabbData);
        }
        rangeInfo_[i].primitiveCount = geometry.primitiveCount;
    }

    VkAccelerationStructureBuildGeometryInfoKHR vkBuildInfo = vkBuildGeometryInfo_;
    vkBuildInfo.dstAccelerationStructure = static_cast<VulkanBlas *>(blas.Get())->_internalGetNativeBlas();
    vkBuildInfo.scratchData.deviceAddress = scratchBuffer.address;

    const VkAccelerationStructureBuildRangeInfoKHR *rangeInfoPointer = rangeInfo_.data();

    auto vkCmdBuffer = commandBuffer->_internalGetNativeCommandBuffer();
    vkCmdBuildAccelerationStructuresKHR(vkCmdBuffer, 1, &vkBuildInfo, &rangeInfoPointer);
}

RTRC_RHI_VK_END
