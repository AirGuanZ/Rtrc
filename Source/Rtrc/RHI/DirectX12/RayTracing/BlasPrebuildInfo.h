#pragma once

#include <Rtrc/RHI/DirectX12/RayTracing/Blas.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12BlasPrebuildInfo, BlasPrebuildInfo)
{
public:

    DirectX12BlasPrebuildInfo(
        DirectX12Device                          *device,
        Span<RayTracingGeometryDesc>              geometries,
        RayTracingAccelerationStructureBuildFlags flags);

    const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

#if RTRC_DEBUG
    bool _internalIsCompatiableWith(Span<RayTracingGeometryDesc> geometries) const;
#endif

    void _internalBuildBlas(
        ID3D12GraphicsCommandList7   *commandList,
        Span<RayTracingGeometryDesc>  geometries,
        const BlasOPtr               &blas,
        BufferDeviceAddress           scratchBuffer);

private:

#if RTRC_DEBUG
    std::vector<RayTracingGeometryDesc> geometries_;
#endif

    DirectX12Device                                     *device_;
    RayTracingAccelerationStructurePrebuildInfo          prebuildInfo_;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC>          dxGeometries_;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs_;
};

RTRC_RHI_D3D12_END
