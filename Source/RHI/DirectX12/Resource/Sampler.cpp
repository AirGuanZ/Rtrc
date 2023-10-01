#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Resource/Sampler.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Sampler::DirectX12Sampler(DirectX12Device *device, const SamplerDesc &desc)
    : device_(device), desc_(desc), sampler_{}
{
    sampler_ = device_->_internalAllocateCPUDescriptorHandle_Sampler();
    RTRC_SCOPE_FAIL{ device_->_internalFreeCPUDescriptorHandle_Sampler(sampler_); };

    const D3D12_SAMPLER_DESC d3dDesc = TranslateSamplerDesc(desc);
    device_->_internalGetNativeDevice()->CreateSampler(&d3dDesc, sampler_);
}

DirectX12Sampler::~DirectX12Sampler()
{
    device_->_internalFreeCPUDescriptorHandle_Sampler(sampler_);
}

RTRC_RHI_D3D12_END
