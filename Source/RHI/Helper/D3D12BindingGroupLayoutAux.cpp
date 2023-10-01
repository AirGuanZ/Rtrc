#include <map>
#include <ranges>

#include <RHI/Helper/D3D12BindingGroupLayoutAux.h>
#include <RHI/RHI.h>
#include <Core/Enumerate.h>

RTRC_RHI_BEGIN

namespace Helper
{

    D3D12BindingGroupLayoutDesc GenerateD3D12BindingGroupLayout(const BindingGroupLayoutDesc &desc)
    {
        D3D12BindingGroupLayoutDesc ret;

        ret.bindings.resize(desc.bindings.size());
        auto &bindingAssignments = ret.bindings;
        auto &tables = ret.tables;
        auto &ranges = ret.ranges;

        std::map<std::pair<D3D12DescTable::ShaderVisibility, D3D12DescTable::Type>, int> classToTableIndex;

        int nextS = 0, nextT = 0, nextU = 0, nextC = 0;

        for(auto &&[bindingIndex, binding] : Enumerate(desc.bindings))
        {
            D3D12BindingAssignment &bindingAssignment = bindingAssignments[bindingIndex];

            const D3D12DescTable::ShaderVisibility shaderVisibility =
                binding.shaderStages == ShaderStage::VS ? D3D12DescTable::VS :
                binding.shaderStages == ShaderStage::FS ? D3D12DescTable::PS : D3D12DescTable::All;

            // Handle immutable samplers

            if(!binding.immutableSamplers.empty())
            {
                bindingAssignment.registerInSpace = nextS;
                nextS += static_cast<int>(binding.immutableSamplers.size());
                bindingAssignment.immutableSamplers.reserve(binding.immutableSamplers.size());
                std::ranges::transform(
                    binding.immutableSamplers,
                    std::back_inserter(bindingAssignment.immutableSamplers),
                    [&](const SamplerPtr &s)
                    {
                        return D3D12BindingAssignment::StaticSamplerRecord{ s->GetDesc(), shaderVisibility };
                    });
                continue;
            }

            // Find desc table

            const D3D12DescTable::Type tableType = binding.type == BindingType::Sampler ? D3D12DescTable::Sampler
                                                                                        : D3D12DescTable::SrvUavCbv;
            int tableIndex;
            if(auto it = classToTableIndex.find({ shaderVisibility, tableType }); it != classToTableIndex.end())
            {
                tableIndex = it->second;
            }
            else
            {
                tableIndex = static_cast<int>(tables.size());
                auto &table = tables.emplace_back();
                table.type = tableType;
                table.shaderVisibility = shaderVisibility;
            }

            bindingAssignment.tableIndex = tableIndex;
            D3D12DescTable &table = tables[tableIndex];

            // Assign slot(s) in range

            const int descCount = static_cast<int>(binding.arraySize.value_or(1));
            
            switch(binding.type)
            {
            using enum BindingType;
            case Texture:
            case Buffer:
            case StructuredBuffer:
            case AccelerationStructure:
                bindingAssignment.offsetInRange = table.srvCount;
                table.srvCount += descCount;
                bindingAssignment.registerInSpace = nextT;
                nextT += descCount;
                break;
            case RWTexture:
            case RWBuffer:
            case RWStructuredBuffer:
                bindingAssignment.offsetInRange = table.uavCount;
                table.uavCount += descCount;
                bindingAssignment.registerInSpace = nextU;
                nextU += descCount;
                break;
            case ConstantBuffer:
                bindingAssignment.offsetInRange = table.cbvCount;
                table.cbvCount += descCount;
                bindingAssignment.registerInSpace = nextC;
                nextC += descCount;
                break;
            case Sampler:
                bindingAssignment.offsetInRange = table.samplerCount;
                table.samplerCount += descCount;
                bindingAssignment.registerInSpace = nextS;
                nextS += descCount;
                break;
            }
        }

        // Allocate ranges
        
        for(const D3D12DescTable &table : tables)
        {
            if(table.srvCount)
            {
                ranges.push_back(D3D12DescRange
                {
                    .type = D3D12DescRange::Srv,
                    .count = static_cast<uint32_t>(table.srvCount),
                });
            }
            if(table.uavCount)
            {
                ranges.push_back(D3D12DescRange
                {
                    .type = D3D12DescRange::Uav,
                    .count = static_cast<uint32_t>(table.uavCount),
                });
            }
            if(table.cbvCount)
            {
                ranges.push_back(D3D12DescRange
                {
                    .type = D3D12DescRange::Cbv,
                    .count = static_cast<uint32_t>(table.cbvCount),
                });
            }
            if(table.samplerCount)
            {
                ranges.push_back(D3D12DescRange
                {
                    .type = D3D12DescRange::Sampler,
                    .count = static_cast<uint32_t>(table.samplerCount)
                });
            }
        }

        // Assign ranges to tables

        {
            int rangeIndex = 0;
            for(D3D12DescTable &table : tables)
            {
                const int rangeCount = static_cast<int>(table.srvCount != 0) + static_cast<int>(table.uavCount != 0) +
                                       static_cast<int>(table.cbvCount != 0) + static_cast<int>(table.samplerCount != 0);
                table.firstRange = rangeIndex;
                table.rangeCount = rangeCount;
                rangeIndex += rangeCount;
            }
        }

        // Assign ranges to bindings

        for(auto &&[bindingIndex, binding] : Enumerate(desc.bindings))
        {
            auto &bindingAssignment = bindingAssignments[bindingIndex];
            if(bindingAssignment.tableIndex < 0)
            {
                continue;
            }
            const D3D12DescTable &table = tables[bindingAssignment.tableIndex];

            switch(binding.type)
            {
            case BindingType::Texture:
            case BindingType::Buffer:
            case BindingType::StructuredBuffer:
            case BindingType::AccelerationStructure:
            case BindingType::Sampler:
                bindingAssignment.rangeIndexInTable = 0;
                bindingAssignment.offsetInTable = 0;
                break;
            case BindingType::RWTexture:
            case BindingType::RWBuffer:
            case BindingType::RWStructuredBuffer:
                bindingAssignment.rangeIndexInTable = table.srvCount != 0 ? 1 : 0;
                bindingAssignment.offsetInTable = table.srvCount;
                break;
            case BindingType::ConstantBuffer:
                bindingAssignment.rangeIndexInTable = (table.srvCount != 0) + (table.uavCount != 0);
                bindingAssignment.offsetInTable = table.srvCount + table.uavCount;
                break;
            }

            if(bindingAssignment.offsetInRange == 0)
            {
                const int rangeIndex = table.firstRange + bindingAssignment.rangeIndexInTable;
                ranges[rangeIndex].baseRegister = bindingAssignment.registerInSpace;
            }
        }

#if RTRC_DEBUG
        for(const auto &r : ranges)
        {
            assert(r.baseRegister != (std::numeric_limits<uint32_t>::max)());
        }
#endif

        return ret;
    }

} // namespace Helper

RTRC_RHI_END
