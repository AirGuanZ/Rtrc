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

    ComPtr<ID3D12WorkGraphProperties> workGraphProperties;
    RTRC_D3D12_FAIL_MSG(
        stateObject_.As(&workGraphProperties), "D3D12 work graph: fail to get work graph properties from state object");

    D3D12_WORK_GRAPH_MEMORY_REQUIREMENTS memoryRequirements;
    workGraphProperties->GetWorkGraphMemoryRequirements(0, &memoryRequirements);

    memoryRequirements_.minSize = memoryRequirements.MinSizeInBytes;
    memoryRequirements_.maxSize = memoryRequirements.MaxSizeInBytes;
    memoryRequirements_.sizeGranularity = memoryRequirements.SizeGranularityInBytes;
}

RTRC_RHI_D3D12_END
