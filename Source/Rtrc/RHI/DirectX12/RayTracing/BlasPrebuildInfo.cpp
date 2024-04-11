#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/RayTracing/Blas.h>
#include <Rtrc/RHI/DirectX12/RayTracing/BlasPrebuildInfo.h>

RTRC_RHI_D3D12_BEGIN

DirectX12BlasPrebuildInfo::DirectX12BlasPrebuildInfo(
    DirectX12Device                          *device,
    Span<RayTracingGeometryDesc>              geometries,
    RayTracingAccelerationStructureBuildFlags flags)
    : device_(device), prebuildInfo_{}, buildInputs_{}
{
#if RTRC_DEBUG
    geometries_ = std::vector<RayTracingGeometryDesc>{ geometries.begin(), geometries.end() };
#endif

    dxGeometries_.reserve(geometries.size());
    for(auto geometry : geometries)
    {
        auto &dxGeometry = dxGeometries_.emplace_back();
        if(geometry.type == RayTracingGeometryType::Triangles)
        {
            const bool hasIndex = geometry.trianglesData.indexFormat != RayTracingIndexFormat::None;
            // Use dummy gpu virtual address '1' for null checking
            // See https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device5-getraytracingaccelerationstructureprebuildinfo
            dxGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
            dxGeometry.Triangles.Transform3x4 = geometry.trianglesData.hasTransform ? 1 : 0;
            dxGeometry.Triangles.IndexFormat  = TranslateRayTracingIndexType(geometry.trianglesData.indexFormat);
            dxGeometry.Triangles.VertexFormat = TranslateRayTracingVertexFormat(geometry.trianglesData.vertexFormat);
            dxGeometry.Triangles.IndexCount   = hasIndex ? 3 * geometry.primitiveCount : 0;
            dxGeometry.Triangles.VertexCount  = geometry.trianglesData.vertexCount;
            dxGeometry.Triangles.IndexBuffer  = hasIndex ? 1 : 0;
            dxGeometry.Triangles.VertexBuffer.StartAddress  = 1;
            dxGeometry.Triangles.VertexBuffer.StrideInBytes = geometry.trianglesData.vertexStride;
        }
        else
        {
            dxGeometry.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
            dxGeometry.AABBs.AABBCount           = geometry.primitiveCount;
            dxGeometry.AABBs.AABBs.StartAddress  = 1;
            dxGeometry.AABBs.AABBs.StrideInBytes = geometry.proceduralData.aabbStride;
        }
        dxGeometry.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
        if(geometry.opaque)
        {
            dxGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
        }
        if(geometry.noDuplicateAnyHitInvocation)
        {
            dxGeometry.Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
        }
    }

    buildInputs_.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildInputs_.Flags          = TranslateRayTracingAccelerationStructureBuildFlags(flags);
    buildInputs_.NumDescs       = static_cast<UINT>(dxGeometries_.size());
    buildInputs_.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildInputs_.pGeometryDescs = dxGeometries_.data();

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO dxPrebuildInfo;
    device_->_internalGetNativeDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs_, &dxPrebuildInfo);

    prebuildInfo_.accelerationStructureSize = dxPrebuildInfo.ResultDataMaxSizeInBytes;
    prebuildInfo_.buildScratchSize          = dxPrebuildInfo.ScratchDataSizeInBytes;
    prebuildInfo_.updateScratchSize         = dxPrebuildInfo.UpdateScratchDataSizeInBytes;
}

const RayTracingAccelerationStructurePrebuildInfo &DirectX12BlasPrebuildInfo::GetPrebuildInfo() const
{
    return prebuildInfo_;
}

#if RTRC_DEBUG

bool DirectX12BlasPrebuildInfo::_internalIsCompatiableWith(Span<RayTracingGeometryDesc> geometries) const
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

#endif

void DirectX12BlasPrebuildInfo::_internalBuildBlas(
    ID3D12GraphicsCommandList7  *commandList,
    Span<RayTracingGeometryDesc> geometries,
    const BlasOPtr              &blas,
    BufferDeviceAddress          scratchBuffer)
{
#if RTRC_DEBUG
    if(!_internalIsCompatiableWith(geometries))
    {
        throw Exception("Blas build info is not compatible with geometries");
    }
#endif

    for(uint32_t i = 0; i < geometries.size(); ++i)
    {
        const RayTracingGeometryDesc &geo = geometries[i];
        if(geo.type == RayTracingGeometryType::Triangles)
        {
            auto &out = dxGeometries_[i].Triangles;
            out.VertexCount               = geo.trianglesData.vertexCount;
            out.VertexBuffer.StartAddress = geo.trianglesData.vertexData.address;
            const bool hasIndex = geo.trianglesData.indexFormat != RayTracingIndexFormat::None;
            if(hasIndex)
            {
                out.IndexBuffer = geo.trianglesData.indexData.address;
                out.IndexCount = 3 * geo.primitiveCount;
            }
            assert((geo.trianglesData.transformData.address != 0) == geo.trianglesData.hasTransform);
            if(geo.trianglesData.hasTransform)
            {
                assert(geo.trianglesData.transformData.address);
                out.Transform3x4 = geo.trianglesData.transformData.address;
            }
        }
        else
        {
            auto &out = dxGeometries_[i].AABBs;
            out.AABBs.StartAddress = geo.proceduralData.aabbData.address;
        }
    }

    auto d3dBlas = static_cast<DirectX12Blas *>(blas.Get());

    //ComPtr<ID3D12DebugCommandList3> debugCommandList;
    //commandList->QueryInterface(IID_PPV_ARGS(debugCommandList.GetAddressOf()));
    //if(debugCommandList)
    //{
    //    auto d3dBuffer = static_cast<DirectX12Buffer *>(d3dBlas->_internalGetBuffer().Get());
    //    debugCommandList->AssertResourceAccess(
    //        d3dBuffer->_internalGetNativeBuffer().Get(),
    //        0, D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE);
    //}

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc;
    buildDesc.DestAccelerationStructureData    = d3dBlas->GetDeviceAddress().address;
    buildDesc.Inputs                           = buildInputs_;
    buildDesc.SourceAccelerationStructureData  = 0;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer.address;
    commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
}

RTRC_RHI_D3D12_END
