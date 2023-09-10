#pragma once

#include <Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Resource/Mesh/Mesh.h>

RTRC_BEGIN

class Device;

RTRC_END

RTRC_RENDERER_BEGIN

class BlasBuilder : public Uncopyable
{
public:

    struct BuildInfo
    {
        RC<Blas>                    blas;
        BlasPrebuildInfo            prebuildInfo;
        RHI::RayTracingGeometryDesc geometryDesc;
    };

    explicit BlasBuilder(ObserverPtr<Device> device);

    BuildInfo Prepare(
        const Mesh                                    *mesh,
        RHI::RayTracingAccelerationStructureBuildFlags flags,
        const RC<Blas>                                &blas);

    void Build(
        CommandBuffer   &commandBuffer,
        const BuildInfo &buildInfo);

    void Finalize(CommandBuffer &commandBuffer);

private:

    static void ExtractVertexProperties(
        const Mesh                  *mesh,
        RHI::RayTracingVertexFormat *vertexFormat,
        RHI::RayTracingIndexFormat  *indexFormat,
        RHI::BufferDeviceAddress    *vertexData,
        RHI::BufferDeviceAddress    *indexData,
        uint32_t                    *vertexStride);

    ObserverPtr<Device> device_;
    bool                needBarrier_;
};

RTRC_RENDERER_END
