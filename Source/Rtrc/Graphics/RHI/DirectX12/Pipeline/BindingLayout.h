#pragma once

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingGroupLayout.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12BindingLayout, BindingLayout)
{
public:
    
    DirectX12BindingLayout(DirectX12Device *device, BindingLayoutDesc desc);

    int _internalGetRootParamIndex(int bindingGroupIndex) const { return bindingGroupIndexToRootParamIndex_[bindingGroupIndex]; }

    // Returned desc.Flags is initialized to 0, which should to be set properly by caller
    const D3D12_ROOT_SIGNATURE_DESC &_internalGetRootSignatureDesc() const { return rootSignatureDesc_; }

    int _internalGetFirstPushConstantRootParamIndex() const { return firstPushConstantRootParamIndex_; }

    const BindingLayoutDesc &_internalGetDesc() const { return desc_; }

    const ComPtr<ID3D12RootSignature> &_internalGetRootSignature(bool allowIA) const;

private:

    DirectX12Device  *device_;
    BindingLayoutDesc desc_;
    
    std::vector<int> bindingGroupIndexToRootParamIndex_;
    int              firstPushConstantRootParamIndex_ = -1;

    std::vector<D3D12_DESCRIPTOR_RANGE>    descRanges_;
    std::vector<D3D12_ROOT_PARAMETER>      rootParams_;
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
    D3D12_ROOT_SIGNATURE_DESC              rootSignatureDesc_ = {};

    mutable tbb::spin_rw_mutex          rootSignatureMutex_;
    mutable ComPtr<ID3D12RootSignature> rootSignature_;
    mutable ComPtr<ID3D12RootSignature> rootSignatureAllowInputAssembler_;
};

RTRC_RHI_D3D12_END
