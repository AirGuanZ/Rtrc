#pragma once

#include <Rtrc/RHI/DirectX12/Pipeline/BindingLayout.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12ComputePipeline, ComputePipeline)
{
public:

    RTRC_D3D12_IMPL_SET_NAME(pipelineState_)

    DirectX12ComputePipeline(
        OPtr<BindingLayout>          bindingLayout,
        ComPtr<ID3D12RootSignature> rootSignature,
        ComPtr<ID3D12PipelineState> pipelineState)
        : bindingLayout_(std::move(bindingLayout))
        , rootSignature_(std::move(rootSignature))
        , pipelineState_(std::move(pipelineState))
    {
        
    }

    const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE
    {
        return bindingLayout_;
    }

    const ComPtr<ID3D12PipelineState> &_internalGetNativePipelineState() const
    {
        return pipelineState_;
    }

    const auto &_internalGetRootSignature() const { return rootSignature_; }

private:

    OPtr<BindingLayout>          bindingLayout_;
    ComPtr<ID3D12RootSignature> rootSignature_;
    ComPtr<ID3D12PipelineState> pipelineState_;
};

RTRC_RHI_D3D12_END