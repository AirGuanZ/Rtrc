#pragma once

#include <d3d12_agility/d3dx12/d3dx12.h>

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

// TODO: optimize with collection
RTRC_RHI_IMPLEMENT(DirectX12RayTracingLibrary, RayTracingLibrary)
{
public:

    explicit DirectX12RayTracingLibrary(RayTracingLibraryDesc desc): desc_(std::move(desc)) { }

    const RayTracingLibraryDesc &_internalGetDesc() const { return desc_; }

private:

    RayTracingLibraryDesc desc_;
};

RTRC_RHI_D3D12_END
