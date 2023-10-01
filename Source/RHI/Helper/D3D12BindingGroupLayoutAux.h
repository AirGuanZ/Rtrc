#pragma once

#include <RHI/RHIDeclaration.h>

RTRC_RHI_BEGIN

namespace Helper
{

    struct D3D12DescRange
    {
        enum Type
        {
            Sampler,
            Srv,
            Uav,
            Cbv
        };

        Type     type;
        uint32_t count;
        uint32_t baseRegister = (std::numeric_limits<uint32_t>::max)();
    };

    struct D3D12DescTable
    {
        enum Type
        {
            Sampler,
            SrvUavCbv
        };

        enum ShaderVisibility
        {
            VS,
            PS,
            All
        };

        Type type                         = SrvUavCbv;
        ShaderVisibility shaderVisibility = All;

        int firstRange = 0;
        int rangeCount = 0;

        int srvCount     = 0;
        int uavCount     = 0;
        int cbvCount     = 0;
        int samplerCount = 0;
    };

    struct D3D12BindingAssignment
    {
        int registerInSpace = 0;
        
        int tableIndex        = -1;
        int rangeIndexInTable = -1;
        int offsetInRange     = -1;
        int offsetInTable     = -1;

        struct StaticSamplerRecord
        {
            SamplerDesc desc;
            D3D12DescTable::ShaderVisibility shaderVisibility = D3D12DescTable::All;
        };
        std::vector<StaticSamplerRecord> immutableSamplers;
    };

    struct D3D12BindingGroupLayoutDesc
    {
        std::vector<D3D12DescRange>         ranges;
        std::vector<D3D12DescTable>         tables;
        std::vector<D3D12BindingAssignment> bindings;
    };

    /*
        Classify bindings with (visibility, { srvUavCbv, sampler }). Each class is mapped to a desc table.
        Registers are assigned in order of binding index. Static samplers are not included in any desc table.
    */
    D3D12BindingGroupLayoutDesc GenerateD3D12BindingGroupLayout(const BindingGroupLayoutDesc &desc);
    
} // namespace Helper

RTRC_RHI_END
