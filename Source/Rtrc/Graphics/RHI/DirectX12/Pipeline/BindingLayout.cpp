#include <shared_mutex>

#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/Helper/D3D12BindingGroupLayoutAux.h>
#include <Core/Enumerate.h>

RTRC_RHI_D3D12_BEGIN

namespace DirectX12BindingLayoutDetail
{

    D3D12_SHADER_VISIBILITY TranslateHelperShaderVisibility(Helper::D3D12DescTable::ShaderVisibility vis)
    {
        using enum Helper::D3D12DescTable::ShaderVisibility;
        return vis == All ? D3D12_SHADER_VISIBILITY_ALL :
               vis == VS  ? D3D12_SHADER_VISIBILITY_VERTEX :
                            D3D12_SHADER_VISIBILITY_PIXEL;
    }
    
} // namespace DirectX12BindingLayoutDetail

DirectX12BindingLayout::DirectX12BindingLayout(DirectX12Device *device, BindingLayoutDesc desc)
    : device_(device), desc_(std::move(desc))
{
    using namespace DirectX12BindingLayoutDetail;

    size_t rangeCount = 0;
    for(auto &groupLayout : desc_.groups)
    {
        const auto bindingGroupLayout = static_cast<DirectX12BindingGroupLayout *>(groupLayout.Get());
        rangeCount += bindingGroupLayout->_internalGetD3D12Desc().ranges.size();
    }
    rangeCount += static_cast<int>(desc_.unboundedAliases.size());
    descRanges_.reserve(rangeCount);

    bindingGroupIndexToRootParamIndex_.resize(desc_.groups.size());
    for(auto &&[groupIndex, groupLayout] : Enumerate(desc_.groups))
    {
        bindingGroupIndexToRootParamIndex_[groupIndex] = static_cast<int>(rootParams_.size());
        const auto bindingGroupLayout = static_cast<DirectX12BindingGroupLayout *>(groupLayout.Get());
        for(const Helper::D3D12DescTable &table : bindingGroupLayout->_internalGetD3D12Desc().tables)
        {
            auto &rootParam = rootParams_.emplace_back();

            rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParam.ShaderVisibility = TranslateHelperShaderVisibility(table.shaderVisibility);

            const size_t oldRangeCount = descRanges_.size();
            for(int i = 0; i < table.rangeCount; ++i)
            {
                const Helper::D3D12DescRange &iRange =
                    bindingGroupLayout->_internalGetD3D12Desc().ranges[table.firstRange + i];

                auto &oRange = descRanges_.emplace_back();
                oRange.RangeType =
                    iRange.type == Helper::D3D12DescRange::Sampler ? D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER :
                    iRange.type == Helper::D3D12DescRange::Srv     ? D3D12_DESCRIPTOR_RANGE_TYPE_SRV :
                    iRange.type == Helper::D3D12DescRange::Uav     ? D3D12_DESCRIPTOR_RANGE_TYPE_UAV :
                                                                     D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                oRange.NumDescriptors                    = iRange.count;
                oRange.BaseShaderRegister                = iRange.baseRegister;
                oRange.RegisterSpace                     = static_cast<UINT>(groupIndex);
                oRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
            }

            rootParam.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(table.rangeCount);
            if(table.rangeCount > 0)
            {
                rootParam.DescriptorTable.pDescriptorRanges = descRanges_.data() + oldRangeCount;
            }
            else
            {
                rootParam.DescriptorTable.pDescriptorRanges = nullptr;
            }
        }

        for(const auto &bindingAssignment : bindingGroupLayout->_internalGetD3D12Desc().bindings)
        {
            for(uint32_t i = 0; i < bindingAssignment.immutableSamplers.size(); ++i)
            {
                auto &s = bindingAssignment.immutableSamplers[i];
                const D3D12_SHADER_VISIBILITY vis = TranslateHelperShaderVisibility(s.shaderVisibility);
                D3D12_STATIC_SAMPLER_DESC d3d12Desc = TranslateStaticSamplerDesc(s.desc, vis);
                d3d12Desc.RegisterSpace = static_cast<UINT>(groupIndex);
                d3d12Desc.ShaderRegister = bindingAssignment.registerInSpace + i;
                staticSamplers_.push_back(d3d12Desc);
            }
        }
    }

    firstPushConstantRootParamIndex_ = static_cast<int>(rootParams_.size());
    for(auto &&[index, pcr] : Enumerate(desc_.pushConstantRanges))
    {
        auto &param = rootParams_.emplace_back();
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.Num32BitValues = pcr.size;
        param.Constants.RegisterSpace = static_cast<UINT>(desc_.groups.size());
        param.Constants.ShaderRegister = static_cast<UINT>(index);
        param.ShaderVisibility = pcr.stages == ShaderStage::VS ? D3D12_SHADER_VISIBILITY_VERTEX :
                                 pcr.stages == ShaderStage::FS ? D3D12_SHADER_VISIBILITY_PIXEL :
                                                                 D3D12_SHADER_VISIBILITY_ALL;
    }

    firstAliasRootParamIndex_ = static_cast<int>(rootParams_.size());
    for(auto &&[aliasIndex, alias] : Enumerate(desc_.unboundedAliases))
    {
        auto *srcGroupLayout = static_cast<DirectX12BindingGroupLayout *>(desc_.groups[alias.srcGroup].Get());
        auto &srcDesc = srcGroupLayout->GetDesc();
        assert(srcDesc.variableArraySize);

        const int srcSlot = static_cast<int>(srcGroupLayout->GetDesc().bindings.size() - 1);
        assert(srcDesc.bindings[srcSlot].bindless);
        
        auto &srcAssignment = srcGroupLayout->_internalGetD3D12Desc().bindings[srcSlot];
        auto &srcTable = srcGroupLayout->_internalGetD3D12Desc().tables[srcAssignment.tableIndex];

        auto &srcRange = descRanges_[srcTable.firstRange + srcAssignment.rangeIndexInTable];
        auto &dstRange = descRanges_.emplace_back();
        dstRange.RangeType                         = srcRange.RangeType;
        dstRange.NumDescriptors                    = srcRange.NumDescriptors;
        dstRange.BaseShaderRegister                = 0;
        dstRange.RegisterSpace                     = 1000u + static_cast<UINT>(aliasIndex);
        dstRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        const int incSize = static_cast<int>(
            srcTable.type == Helper::D3D12DescTable::SrvUavCbv ?
                device->_internalGetDescriptorHandleIncrementSize_CbvSrvUav() :
                device->_internalGetDescriptorHandleIncrementSize_Sampler());

        auto &dstAlias = aliases_.emplace_back();
        dstAlias.srcRootParamIndex = alias.srcGroup;
        dstAlias.srcTableIndex     = srcAssignment.tableIndex;
        dstAlias.rootParamIndex    = static_cast<int>(rootParams_.size());
        dstAlias.offsetInSrcTable  = srcAssignment.offsetInTable * incSize;

        auto &param = rootParams_.emplace_back();
        param.ParameterType                       = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.ShaderVisibility                    = TranslateHelperShaderVisibility(srcTable.shaderVisibility);
        param.DescriptorTable.NumDescriptorRanges = 1;
        param.DescriptorTable.pDescriptorRanges   = &dstRange;
    }

    std::ranges::sort(aliases_, [](const Alias &lhs, const Alias &rhs)
    {
        return lhs.srcRootParamIndex < rhs.srcRootParamIndex;
    });

    uint32_t nextAliasIndex = 0;
    groupIndexToAliases_.resize(desc_.groups.size());
    for(uint32_t i = 0; i < groupIndexToAliases_.size(); ++i)
    {
        uint32_t end = nextAliasIndex;
        while(end < aliases_.size() && aliases_[end].srcRootParamIndex == static_cast<int>(i))
        {
            ++end;
        }
        if(nextAliasIndex != end)
        {
            groupIndexToAliases_[i] = Span(&aliases_[nextAliasIndex], end - nextAliasIndex);
            nextAliasIndex = end;
        }
    }

    rootSignatureDesc_.NumParameters     = static_cast<UINT>(rootParams_.size());
    rootSignatureDesc_.pParameters       = rootParams_.data();
    rootSignatureDesc_.NumStaticSamplers = static_cast<UINT>(staticSamplers_.size());
    rootSignatureDesc_.pStaticSamplers   = staticSamplers_.data();
    rootSignatureDesc_.Flags             = D3D12_ROOT_SIGNATURE_FLAG_NONE;
}

const ComPtr<ID3D12RootSignature> &DirectX12BindingLayout::_internalGetRootSignature(bool allowIA) const
{
    {
        std::shared_lock lock(rootSignatureMutex_);
        if(allowIA)
        {
            if(rootSignatureAllowInputAssembler_)
            {
                return rootSignatureAllowInputAssembler_;
            }
        }
        else
        {
            if(rootSignature_)
            {
                return rootSignature_;
            }
        }
    }

    std::lock_guard lock(rootSignatureMutex_);
    if(allowIA)
    {
        if(rootSignatureAllowInputAssembler_)
        {
            return rootSignatureAllowInputAssembler_;
        }
    }
    else
    {
        if(rootSignature_)
        {
            return rootSignature_;
        }
    }

    ComPtr<ID3D10Blob> serializedRootSignature, serializeError;
    const D3D12_ROOT_SIGNATURE_DESC *desc = &rootSignatureDesc_;
    D3D12_ROOT_SIGNATURE_DESC tempDesc;
    if(allowIA)
    {
        tempDesc = rootSignatureDesc_;
        tempDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        desc = &tempDesc;
    }
    RTRC_D3D12_CHECK(D3D12SerializeRootSignature(
        desc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSignature.GetAddressOf(), serializeError.GetAddressOf()))
    {
        const char *errorMessage = serializeError ?
            static_cast<const char *>(serializeError->GetBufferPointer()) : "unknown";
        throw Exception(fmt::format(
            "Fail to serialize directx12 root signature. Error message: {}", errorMessage));
    };

    ComPtr<ID3D12RootSignature> rootSignature;
    RTRC_D3D12_FAIL_MSG(
        device_->_internalGetNativeDevice()->CreateRootSignature(
            0,
            serializedRootSignature->GetBufferPointer(),
            serializedRootSignature->GetBufferSize(),
            IID_PPV_ARGS(rootSignature.GetAddressOf())),
        "Fail to create directx12 root signature");

    if(allowIA)
    {
        rootSignatureAllowInputAssembler_ = rootSignature;
        return rootSignatureAllowInputAssembler_;
    }
    rootSignature_ = rootSignature;
    return rootSignature_;
}

RTRC_RHI_D3D12_END
