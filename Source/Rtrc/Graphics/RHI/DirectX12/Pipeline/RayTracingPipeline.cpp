#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_RHI_D3D12_BEGIN

DirectX12RayTracingPipeline::DirectX12RayTracingPipeline(
    ComPtr<ID3D12StateObject>   pipelineStateObject,
    Ptr<BindingLayout>          bindingLayout,
    ComPtr<ID3D12RootSignature> rootSignature,
    std::vector<std::wstring>   groupExportedNames)
    : pipelineStateObject_(std::move(pipelineStateObject))
    , bindingLayout_(std::move(bindingLayout))
    , rootSignature_(std::move(rootSignature))
    , groupExportedNames_(std::move(groupExportedNames))
{
    pipelineStateObject_.As(&properties_);
}

const Ptr<BindingLayout> &DirectX12RayTracingPipeline::GetBindingLayout() const
{
    return bindingLayout_;
}

void DirectX12RayTracingPipeline::GetShaderGroupHandles(
    uint32_t                   startGroupIndex,
    uint32_t                   groupCount,
    MutableSpan<unsigned char> outputData) const
{
    for(uint32_t i = 0; i < groupCount; ++i)
    {
        const uint32_t groupIndex = startGroupIndex + i;
        const void *identifier = properties_->GetShaderIdentifier(groupExportedNames_[groupIndex].c_str());
        std::memcpy(
            outputData.GetData() + i * D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES,
            identifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    }
}

RTRC_RHI_D3D12_END
