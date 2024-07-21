#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12WorkGraphPipeline, WorkGraphPipeline)
{
public:

    DirectX12WorkGraphPipeline(WorkGraphPipelineDesc desc, ComPtr<ID3D12StateObject> stateObject);

    const WorkGraphPipelineDesc &GetDesc() const RTRC_RHI_OVERRIDE { return desc_; }

    const auto &_internalGetStateObject() const { return stateObject_; }
    const auto &_internalGetProgramIdentifier() const { return programIdentifier_; }

private:

    WorkGraphPipelineDesc desc_;
    ComPtr<ID3D12StateObject> stateObject_;
    D3D12_PROGRAM_IDENTIFIER programIdentifier_;
};

RTRC_RHI_D3D12_END
