#pragma once

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

RTRC_BEGIN

class Device;

RTRC_END

RTRC_RENDERER_BEGIN

class BlasBuilder : public Uncopyable
{
public:

    explicit BlasBuilder(ObserverPtr<Device> device);

    void Build(
        CommandBuffer                                 &commandBuffer,
        const Mesh::SharedRenderingData               &renderingData,
        RHI::RayTracingAccelerationStructureBuildFlags flags,
        RC<Blas>                                      &blas);

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
