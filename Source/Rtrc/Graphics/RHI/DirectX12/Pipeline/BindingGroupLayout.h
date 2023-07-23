#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/Helper/D3D12BindingGroupLayoutAux.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12BindingGroupLayout, BindingGroupLayout)
{
public:

    explicit DirectX12BindingGroupLayout(BindingGroupLayoutDesc desc)
        : desc_(std::move(desc))
    {
        d3d12Desc_ = Helper::GenerateD3D12BindingGroupLayout(desc_);
    }

    const BindingGroupLayoutDesc &GetDesc() const RTRC_RHI_OVERRIDE { return desc_; }

    const Helper::D3D12BindingGroupLayoutDesc &_internalGetD3D12Desc() const { return d3d12Desc_; }

private:

    BindingGroupLayoutDesc              desc_;
    Helper::D3D12BindingGroupLayoutDesc d3d12Desc_;
};

RTRC_RHI_D3D12_END
