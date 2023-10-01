#include <Graphics/Device/Device.h>
#include <Rtrc/Renderer/GPUScene/BlasBuilder.h>

RTRC_RENDERER_BEGIN

BlasBuilder::BlasBuilder(ObserverPtr<Device> device)
    : device_(std::move(device)), needBarrier_(false)
{
    
}

BlasBuilder::BuildInfo BlasBuilder::Prepare(
    const Mesh                                    *mesh,
    RHI::RayTracingAccelerationStructureBuildFlags flags,
    const RC<Blas>                                &blas)
{
    RHI::RayTracingVertexFormat vertexFormat;
    RHI::RayTracingIndexFormat indexFormat;
    RHI::BufferDeviceAddress vertexData, indexData;
    uint32_t vertexStride;
    ExtractVertexProperties(mesh, &vertexFormat, &indexFormat, &vertexData, &indexData, &vertexStride);
    
    const RHI::RayTracingGeometryDesc geometryDesc =
    {
        .type                        = RHI::RayTracingGeometryType::Triangles,
        .primitiveCount              = mesh->GetPrimitiveCount(),
        .opaque                      = true,
        .noDuplicateAnyHitInvocation = false,
        .trianglesData = RHI::RayTracingTrianglesGeometryData
        {
            .vertexFormat  = vertexFormat,
            .vertexStride  = vertexStride,
            .vertexCount   = mesh->GetVertexCount(),
            .indexFormat   = indexFormat,
            .hasTransform  = false,
            .vertexData    = vertexData,
            .indexData     = indexData,
            .transformData = { }
        }
    };

    auto prebuildInfo = device_->CreateBlasPrebuildinfo(geometryDesc, flags);

    return BuildInfo
    {
        .blas         = blas,
        .prebuildInfo = std::move(prebuildInfo),
        .geometryDesc = geometryDesc
    };
}

void BlasBuilder::Build(CommandBuffer &commandBuffer, const BuildInfo &buildInfo)
{
    commandBuffer.BuildBlas(buildInfo.blas, buildInfo.geometryDesc, buildInfo.prebuildInfo, nullptr);
    needBarrier_ = true;
}

void BlasBuilder::ExtractVertexProperties(
    const Mesh                  *mesh,
    RHI::RayTracingVertexFormat *vertexFormat,
    RHI::RayTracingIndexFormat  *indexFormat,
    RHI::BufferDeviceAddress    *vertexData,
    RHI::BufferDeviceAddress    *indexData,
    uint32_t                    *vertexStride)
{
    // TODO: fast path for common case where position is the first attribute in the first buffer

    auto ThrowNotFound = []
    {
        throw Exception(
            "Attribute with 'POSITION' semantic is not found in vertex layout of mesh for building blas");
    };

    const MeshLayout *meshLayout = mesh->GetLayout();
    const int vertexBufferIndex = meshLayout->GetVertexBufferIndexBySemantic(RTRC_VERTEX_SEMANTIC(POSITION, 0));
    if(vertexBufferIndex < 0)
    {
        ThrowNotFound();
    }
    const VertexBufferLayout *bufferLayout = meshLayout->GetVertexBufferLayouts()[vertexBufferIndex];

    const int attribIndex = bufferLayout->GetAttributeIndexBySemantic(RTRC_VERTEX_SEMANTIC(POSITION, 0));
    if(attribIndex < 0)
    {
        ThrowNotFound();
    }

    const VertexBufferLayout::VertexAttribute &attrib = bufferLayout->GetAttributes()[attribIndex];
    if(attrib.type != RHI::VertexAttributeType::Float3)
    {
        throw Exception("Only float3 position is supported for building blas");
    }
    *vertexFormat = RHI::RayTracingVertexFormat::Float3;

    const RHI::BufferDeviceAddress base = mesh->GetVertexBuffer(vertexBufferIndex)->GetDeviceAddress();
    *vertexData = base + attrib.byteOffsetInBuffer;
    *vertexStride = bufferLayout->GetElementStride();

    if(mesh->GetIndexBuffer())
    {
        switch(mesh->GetIndexFormat())
        {
        case RHI::IndexFormat::UInt16:
            *indexFormat = RHI::RayTracingIndexFormat::UInt16;
            break;
        case RHI::IndexFormat::UInt32:
            *indexFormat = RHI::RayTracingIndexFormat::UInt32;
            break;
        }
        *indexData = mesh->GetIndexBuffer()->GetDeviceAddress();
    }
    else
    {
        *indexFormat = RHI::RayTracingIndexFormat::None;
        *indexData = {};
    }
}

void BlasBuilder::Finalize(CommandBuffer &commandBuffer)
{
    if(needBarrier_)
    {
        commandBuffer.ExecuteBarrier(
            RHI::PipelineStage::BuildAS, 
            RHI::ResourceAccess::WriteAS,
            RHI::PipelineStage::All,
            RHI::ResourceAccess::ReadAS | RHI::ResourceAccess::ReadForBuildAS);
        needBarrier_ = false;
    }
}

RTRC_RENDERER_END
