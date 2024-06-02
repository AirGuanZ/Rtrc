#pragma once

#include <Rtrc/Graphics/Shader/DSL/BindingGroup/BindingGroupDefinition.h>

RTRC_BEGIN

namespace BindingGroupDetail
{

    template<RtrcGroupSketch T, typename F, typename A = std::identity>
    static constexpr void ForEachFlattenMember(
        const F &f, RHI::ShaderStageFlags stageMask = T::_rtrcGroupDefaultStages, const A &accessor = std::identity{})
    {
        StructDetail::ForEachMember<T>(
            [&]<bool IsUniform, typename M>
            (M T::* ptr, const char *name, RHI::ShaderStageFlags stages, BindingFlags bindingFlags)
        {
            auto newAccessor = [&]<typename P>(P p) constexpr -> decltype(auto)
            {
                static_assert(std::is_pointer_v<P>);
                return &(accessor(p)->*ptr);
            };
            if constexpr(RtrcGroupSketch<M>)
            {
                BindingGroupDetail::ForEachFlattenMember<M>(f, stageMask & stages, newAccessor);
            }
            else
            {
                f.template operator()<IsUniform, M>(name, stageMask & stages, newAccessor, bindingFlags);
            }
        });
    }

    template<RtrcGroupStruct T>
    consteval bool HasUniform()
    {
        bool ret = false;
        BindingGroupDetail::ForEachFlattenMember<T>(
            [&ret]<bool IsUniform, typename M, typename A>
            (const char *, RHI::ShaderStageFlags, const A &, BindingFlags)
        {
            if constexpr(IsUniform)
            {
                ret = true;
            }
        });
        return ret;
    }

    template<RtrcGroupStruct T>
    size_t GetUniformDWordCount()
    {
        size_t dwordCount = 0;
        BindingGroupDetail::ForEachFlattenMember<T>(
            [&dwordCount]<bool IsUniform, typename M, typename A>
            (const char *, RHI::ShaderStageFlags, const A &, BindingFlags)
        {
            if constexpr(IsUniform)
            {
                const size_t memSize = GetDeviceDWordCount<M>();
                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (dwordCount % 4) + memSize > 4;
                }
                if(needNewLine)
                {
                    dwordCount = ((dwordCount + 3) >> 2) << 2;
                }
                dwordCount += memSize;
            }
        });
        return dwordCount;
    }
    
    template<RtrcGroupStruct T>
    void FlattenUniformsToConstantBufferData(const T &data, void *output)
    {
        size_t deviceDWordOffset = 0;
        BindingGroupDetail::ForEachFlattenMember<T>(
            [&]<bool IsUniform, typename M, typename A>
            (const char *, RHI::ShaderStageFlags, const A & accessor, BindingFlags)
        {
            if constexpr(IsUniform)
            {
                const size_t memberSize = GetDeviceDWordCount<M>();

                bool needNewLine;
                if constexpr(std::is_array_v<M> || RtrcStruct<M>)
                {
                    needNewLine = true;
                }
                else
                {
                    needNewLine = (deviceDWordOffset % 4) + memberSize > 4;
                }
                if(needNewLine)
                {
                    deviceDWordOffset = ((deviceDWordOffset + 3) >> 2) << 2;
                }

                constexpr T *nullT = nullptr;
                const size_t hostOffset = reinterpret_cast<size_t>(accessor(nullT));

                Rtrc::ToDeviceLayout<M>(&data, output, hostOffset, 4 * deviceDWordOffset);
                
                deviceDWordOffset += memberSize;
            }
        });
    }

    template<typename T>
    struct MemberProxyTrait
    {
        using MemberProxyElement = T;
        static constexpr size_t ArraySize = 0;
    };

    template<typename T, size_t N>
    struct MemberProxyTrait<T[N]>
    {
        using MemberProxyElement = T;
        static constexpr size_t ArraySize = N;
    };
    
    template<RtrcGroupStruct T>
    const BindingGroupLayout::Desc &ToBindingGroupLayout()
    {
        static BindingGroupLayout::Desc ret = []
        {
            BindingGroupLayout::Desc desc;
            BindingGroupDetail::ForEachFlattenMember<T>(
                [&desc]<bool IsUniform, typename M, typename A>
                (const char *, RHI::ShaderStageFlags stages, const A &, BindingFlags flags)
            {
                if constexpr(!IsUniform)
                {
                    using MemberElement = typename MemberProxyTrait<M>::MemberProxyElement;
                    constexpr size_t arraySize = MemberProxyTrait<M>::ArraySize;
                    BindingGroupLayout::BindingDesc binding;
                    binding.type = MemberElement::BindingType;
                    binding.stages = stages;
                    binding.arraySize = arraySize ? std::make_optional(static_cast<uint32_t>(arraySize)) : std::nullopt;
                    binding.bindless = flags.Contains(BindingFlagBit::Bindless);
                    desc.bindings.push_back(binding);
                    desc.variableArraySize |= flags.Contains(BindingFlagBit::VariableArraySize);
                }
            });
            if constexpr(HasUniform<T>())
            {
                BindingGroupLayout::BindingDesc binding;
                binding.type = RHI::BindingType::ConstantBuffer;
                binding.stages = T::_rtrcGroupDefaultStages;
                desc.bindings.push_back(binding);
            }
            if(desc.variableArraySize && HasUniform<T>())
            {
                assert(desc.bindings[desc.bindings.size() - 2].bindless);
                std::swap(desc.bindings[desc.bindings.size() - 2], desc.bindings[desc.bindings.size() - 1]);
            }
            return desc;
        }();
        return ret;
    }

    template<RtrcGroupStruct T>
    const BindingGroupLayout::Desc &GetBindingGroupLayoutDesc()
    {
        return BindingGroupDetail::ToBindingGroupLayout<T>();
    }
    
    template<RtrcGroupStruct T>
    void DeclareRenderGraphResourceUses(RGPassImpl *pass, const T &value, RHI::PipelineStageFlag stages)
    {
        BindingGroupDetail::ForEachFlattenMember<T>(
            [&]<bool IsUniform, typename M, typename A>
            (const char *, RHI::ShaderStageFlags shaderStages, const A &accessor, BindingFlags)
        {
            const RHI::PipelineStageFlag pipelineStages = ShaderStagesToPipelineStages(shaderStages) & stages;
            if constexpr(!IsUniform)
            {
                using MemberElement = typename MemberProxyTrait<M>::MemberProxyElement;
                constexpr size_t arraySize = MemberProxyTrait<M>::ArraySize;
                static_assert(
                    arraySize == 0 || MemberElement::BindingType != RHI::BindingType::ConstantBuffer,
                    "ConstantBuffer array hasn't been supported");
                if constexpr(arraySize > 0)
                {
                    constexpr bool CanDeclare = requires
                    {
                        (*accessor(&value))[0].DeclareRenderGraphResourceUsage(pass, pipelineStages);
                    };
                    for(size_t i = 0; i < arraySize; ++i)
                    {
                        if constexpr(CanDeclare)
                        {
                            (*accessor(&value))[i].DeclareRenderGraphResourceUsage(pass, pipelineStages);
                        }
                    }
                }
                else
                {
                    constexpr bool CanDeclare = requires
                    {
                        accessor(&value)->DeclareRenderGraphResourceUsage(pass, pipelineStages);
                    };
                    if constexpr(CanDeclare)
                    {
                        accessor(&value)->DeclareRenderGraphResourceUsage(pass, pipelineStages);
                    }
                }
            }
        });
    }

    template<RtrcGroupStruct T>
    void ApplyBindingGroup(RHI::Device *device, ConstantBufferManagerInterface *cbMgr, BindingGroup &group, const T &value)
    {
        RHI::BindingGroupUpdateBatch batch;
        int index = 0;
        bool swapUniformAndVariableSizedArray = false;
        BindingGroupDetail::ForEachFlattenMember<T>(
            [&]<bool IsUniform, typename M, typename A>
            (const char *name, RHI::ShaderStageFlags, const A &accessor, BindingFlags flags)
        {
            if constexpr(!IsUniform)
            {
                using MemberElement = typename MemberProxyTrait<M>::MemberProxyElement;
                constexpr size_t arraySize = MemberProxyTrait<M>::ArraySize;
                static_assert(
                    arraySize == 0 || MemberElement::BindingType != RHI::BindingType::ConstantBuffer,
                    "ConstantBuffer array hasn't been supported");
                if constexpr(arraySize > 0)
                {
                    auto ApplyArrayElements = [&, as = arraySize](int actualIndex)
                    {
                        for(size_t i = 0; i < as; ++i)
                        {
                            if constexpr(IsRC<std::remove_cvref_t<decltype((*accessor(&value))[i].GetRtrcObject())>>)
                            {
                                if(auto obj = (*accessor(&value))[i].GetRtrcObject())
                                {
                                    batch.Append(*group.GetRHIObject(), actualIndex, i, obj->GetRHIObject());
                                }
                            }
                            else
                            {
                                if(auto rhiObj = (*accessor(&value))[i].GetRtrcObject().GetRHIObject())
                                {
                                    batch.Append(*group.GetRHIObject(), actualIndex, i, std::move(rhiObj));
                                }
                            }
                        }
                    };

                    assert(!swapUniformAndVariableSizedArray);
                    swapUniformAndVariableSizedArray =
                        BindingGroupDetail::HasUniform<T>() &&
                        flags.Contains(BindingFlagBit::VariableArraySize);

                    ApplyArrayElements(swapUniformAndVariableSizedArray ? (index + 1) : index);
                    ++index;
                }
                else if constexpr(MemberElement::BindingType == RHI::BindingType::ConstantBuffer)
                {
                    if(accessor(&value)->GetRtrcObject())
                    {
                        auto cbuffer = accessor(&value)->GetRtrcObject();
                        batch.Append(*group.GetRHIObject(), index++, RHI::ConstantBufferUpdate
                        {
                            cbuffer->GetFullBufferRHIObject().Get(),
                            cbuffer->GetSubBufferOffset(),
                            cbuffer->GetSubBufferSize()
                        });
                    }
                    else
                    {
                        using CBufferStruct = typename MemberElement::Struct;
                        if(!cbMgr)
                        {
                            throw Exception(fmt::format(
                                "ApplyBindingGroup: Constant buffer manager is nullptr when "
                                "setting constant buffer {} without giving pre-created cb object", name));
                        }
                        auto cbuffer = cbMgr->CreateConstantBuffer(static_cast<const CBufferStruct &>(*accessor(&value)));
                        batch.Append(*group.GetRHIObject(), index++, RHI::ConstantBufferUpdate{
                            cbuffer->GetFullBufferRHIObject().Get(),
                            cbuffer->GetSubBufferOffset(),
                            cbuffer->GetSubBufferSize()
                        });
                    }
                }
                else if constexpr(IsRC<std::remove_cvref_t<decltype(accessor(&value)->GetRtrcObject())>>)
                {
                    if(auto obj = accessor(&value)->GetRtrcObject())
                    {
                        batch.Append(*group.GetRHIObject(), index, obj->GetRHIObject());
                    }
                    ++index;
                }
                else
                {
                    if(auto rhiObj = accessor(&value)->GetRtrcObject().GetRHIObject())
                    {
                        batch.Append(*group.GetRHIObject(), index, std::move(rhiObj));
                    }
                    ++index;
                }
            }
        });
        if constexpr(HasUniform<T>())
        {
            const size_t dwordCount = GetUniformDWordCount<T>();

            std::vector<unsigned char> deviceData(dwordCount * 4);
            BindingGroupDetail::FlattenUniformsToConstantBufferData(value, deviceData.data());
            
            const int actualIndex = swapUniformAndVariableSizedArray ? (index - 1) : index;
            auto cbuffer = cbMgr->CreateConstantBuffer(deviceData.data(), deviceData.size());
            batch.Append(*group.GetRHIObject(), actualIndex, RHI::ConstantBufferUpdate
            {
                cbuffer->GetFullBufferRHIObject().Get(),
                cbuffer->GetSubBufferOffset(),
                cbuffer->GetSubBufferSize()
            });
        }

        device->UpdateBindingGroups(batch);
    }

    template<RtrcGroupStruct T>
    void ApplyBindingGroup(
        RHI::Device *device, DynamicBufferManager *cbMgr, const RC<BindingGroup> &group, const T &value)
    {
        BindingGroupDetail::ApplyBindingGroup(device, cbMgr, *group, value);
    }

} // namespace BindingGroupDetail

using BindingGroupDetail::GetBindingGroupLayoutDesc;
using BindingGroupDetail::DeclareRenderGraphResourceUses;
using BindingGroupDetail::ApplyBindingGroup;

template<typename T>
void BindingGroup::Set(const T &value)
{
    static_assert(BindingGroupDetail::RtrcGroupStruct<T>);
    BindingGroupDetail::ApplyBindingGroup(
        manager_->_internalGetRHIDevice(), manager_->_internalGetDefaultConstantBufferManager(), *this, value);
}

RTRC_END
