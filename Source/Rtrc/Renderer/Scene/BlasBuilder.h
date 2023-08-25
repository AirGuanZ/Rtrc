#pragma once

#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

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
        const Mesh::SharedRenderingData               &renderingData,
        RHI::RayTracingAccelerationStructureBuildFlags flags,
        const RC<Blas>                                &blas);

    void Build(
        CommandBuffer   &commandBuffer,
        const BuildInfo &buildInfo);

    void Finalize(CommandBuffer &commandBuffer);

private:

    static void ExtractVertexProperties(
        const Mesh::SharedRenderingData &sharedRenderingData,
        RHI::RayTracingVertexFormat     *vertexFormat,
        RHI::RayTracingIndexFormat      *indexFormat,
        RHI::BufferDeviceAddress        *vertexData,
        RHI::BufferDeviceAddress        *indexData,
        uint32_t                        *vertexStride);

    ObserverPtr<Device> device_;
    bool                needBarrier_;
};

RTRC_RENDERER_END
