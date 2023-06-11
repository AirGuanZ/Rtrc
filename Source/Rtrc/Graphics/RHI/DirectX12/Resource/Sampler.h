#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Sampler, Sampler)
{
public:

    DirectX12Sampler(DirectX12Device *device, const SamplerDesc &desc);
    ~DirectX12Sampler() override;

    const SamplerDesc &GetDesc() const RTRC_RHI_OVERRIDE { return desc_; }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetNativeSampler() const { return sampler_; }

private:

    DirectX12Device            *device_;
    SamplerDesc                 desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE sampler_;
};

RTRC_RHI_D3D12_END
