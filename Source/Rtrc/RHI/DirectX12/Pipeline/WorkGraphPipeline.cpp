#include <Rtrc/RHI/DirectX12/Pipeline/WorkGraphPipeline.h>

RTRC_RHI_D3D12_BEGIN

DirectX12WorkGraphPipeline::DirectX12WorkGraphPipeline(
    WorkGraphPipelineDesc desc, ComPtr<ID3D12StateObject> stateObject)
    : desc_(std::move(desc)), stateObject_(std::move(stateObject))
{
    ComPtr<ID3D12StateObjectProperties1> stateObjectProperties;
    RTRC_D3D12_FAIL_MSG(
        stateObject_.As(&stateObjectProperties), "D3D12 work graph: fail to get properties1 from state object");

    programIdentifier_ = stateObjectProperties->GetProgramIdentifier(L"DefaultWorkGraphProgramName");
}

RTRC_RHI_D3D12_END
