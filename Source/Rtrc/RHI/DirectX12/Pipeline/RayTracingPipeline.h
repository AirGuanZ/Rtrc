#pragma once

#include <Rtrc/RHI/DirectX12/Pipeline/BindingLayout.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12RayTracingPipeline, RayTracingPipeline)
{
public:

    DirectX12RayTracingPipeline(
        ComPtr<ID3D12StateObject>   pipelineStateObject,
        OPtr<BindingLayout>          bindingLayout,
        ComPtr<ID3D12RootSignature> rootSignature,
        std::vector<std::wstring>   groupExportedNames);

    const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_OVERRIDE;

    void GetShaderGroupHandles(
        uint32_t               startGroupIndex,
        uint32_t               groupCount,
        MutSpan<unsigned char> outputData) const RTRC_RHI_OVERRIDE;

    const ComPtr<ID3D12StateObject> &_internalGetStateObject() const { return pipelineStateObject_; }
    const ComPtr<ID3D12RootSignature> &_internalGetRootSignature() const { return rootSignature_; }

private:

    ComPtr<ID3D12StateObject>           pipelineStateObject_;
    OPtr<BindingLayout>                  bindingLayout_;
    ComPtr<ID3D12RootSignature>         rootSignature_;
    std::vector<std::wstring>           groupExportedNames_;
    ComPtr<ID3D12StateObjectProperties> properties_;
};

RTRC_RHI_D3D12_END
