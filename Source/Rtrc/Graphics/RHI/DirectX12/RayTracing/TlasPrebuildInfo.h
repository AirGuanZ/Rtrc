#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12TlasPrebuildInfo, TlasPrebuildInfo)
{
public:

    DirectX12TlasPrebuildInfo(
        DirectX12Device                          *device,
        const RayTracingInstanceArrayDesc        &instances,
        RayTracingAccelerationStructureBuildFlags flags);

    const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_OVERRIDE;

    void _internalBuildTlas(
        ID3D12GraphicsCommandList7        *commandList,
        const RayTracingInstanceArrayDesc &instances,
        const TlasPtr                     &tlas,
        BufferDeviceAddress                scratchBuffer);

private:

#if RTRC_DEBUG
    RayTracingInstanceArrayDesc instances_;
#endif

    DirectX12Device                                     *device_;
    RayTracingAccelerationStructurePrebuildInfo          prebuildInfo_;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs_;
};

RTRC_RHI_D3D12_END