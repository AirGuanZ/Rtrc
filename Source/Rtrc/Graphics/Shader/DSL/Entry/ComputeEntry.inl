#pragma once

#include <cassert>

#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Shader/DSL/BindingGroup/BindingGroupBuilder_eDSL.h>
#include <Rtrc/Graphics/Shader/ShaderBuilder.h>
#include <Rtrc/ShaderCommon/Preprocess/RegisterAllocator.h>

RTRC_EDSL_BEGIN

namespace ComputeEntryDetail
{

    inline auto &GetThreadGroupSizeStack()
    {
        static std::stack<Vector3u *> ret;
        return ret;
    }

    struct ComputeEntryIntermediates
    {
        std::string                           resourceDefinitions;
        Vector3u                              threadGroupSize;
        RecordContext                         recordContext;
        BindingGroupLayout::Desc              defaultBindingGroupLayoutDesc;
        std::vector<ShaderUniformType>        defaultUniformTypes;
        std::vector<BindingGroupLayout::Desc> bindingGroupLayoutDescs;
    };

    void BuildComputeEntry(
        Ref<Device>                            device,
        std::string                           &resourceDefinitions,
        const Vector3u                        &threadGroupSize,
        RecordContext                         &recordContext,
        BindingGroupLayout::Desc              &defaultBindingGroupLayoutDesc,
        std::vector<ShaderUniformType>        &defaultUniformTypes,
        std::vector<BindingGroupLayout::Desc> &bindingGroupLayoutDescs,
        RC<Shader>                            &outShader,
        int                                   &outDefaultBindingGroupIndex,
        std::vector<ValueItem>                &outDefaultBindingGroupValueItems);

    template<typename...Args, typename Func>
    void RecordComputeKernel(
        RHI::BackendType           backendType,
        const Func                &func,
        ComputeEntryIntermediates &result,
        ComputeEntry<Args...> *)
    {
        // Prepare eDSL args
        //    Resource:                          _rtrcBindingGroup{groupIndex}_name
        //    Value:                             _rtrcBindingGroup{groupIndex}.name
        //    Resource in default binding group: _rtrcDefaultBindingGroup_Resource{index}
        //    Value in default binding group:    _rtrcDefaultBindingGroup.Value{index}
        //    Static sampler:                    _rtrcStaticSamplers

        DisableStackVariableAllocation();
        std::tuple<Args...> args;
        {
            uint32_t bindingGroupIndex = 0;
            uint32_t resourceCountInDefaultBindingGroup = 0;
            uint32_t valueCountInDefaultBindingGroup = 0;

            auto regAlloc = ShaderResourceRegisterAllocator::Create(backendType);

            std::vector<std::string> partialResourceDefinitionsInDefaultBindingGroup;
            std::vector<RHI::BindingType> resourceBindingTypesInDefaultBindingGroup;

            auto AssignArgName = [&]<typename Arg>(Arg &arg)
            {
                if constexpr(RtrcDSLBindingGroup<Arg>)
                {
                    using BindingGroupStruct = BindingGroupDetail::RebindSketchBuilder<
                        Arg, BindingGroupDetail::BindingGroupStructBuilder_Default>;
                    result.bindingGroupLayoutDescs.push_back(
                        BindingGroupDetail::ToBindingGroupLayout<BindingGroupStruct>());

                    struct UniformVariable
                    {
                        ShaderUniformType type;
                        std::string name;
                    };
                    std::vector<UniformVariable> uniformVariables;

                    regAlloc->NewBindingGroup(bindingGroupIndex);

                    uint32_t resourceCountInBindingGroup = 0;
                    BindingGroupDetail::ForEachFlattenMember<Arg>(
                        [&]<bool IsUniform, typename M, typename A>
                        (const char *name, RHI::ShaderStageFlags, const A &accessor, BindingGroupDetail::BindingFlags flags)
                    {
                        assert(flags == BindingGroupDetail::BindingFlags::None &&
                               "eBindingGroup doesn't support bindless/varsized resource array");
                        if constexpr(IsUniform)
                        {
                            auto &var = uniformVariables.emplace_back();
                            var.type = ArgumentTrait::eValueTrait<M>::Type;
                            var.name = name;

                            accessor(arg).eVariableName = std::format(
                                "_rtrcBindingGroup{}.{}", bindingGroupIndex, name);
                        }
                        else
                        {
                            regAlloc->NewBinding(
                                resourceCountInBindingGroup, ArgumentTrait::eResourceTrait<M>::GetBindingDesc().type);

                            const std::string resourceName = std::format(
                                "_rtrcBindingGroup{}_{}", bindingGroupIndex, name);
                            result.resourceDefinitions += std::format(
                                "{}{} {}{};\n",
                                regAlloc->GetPrefix(), M::GetStaticTypeName(),
                                resourceName, regAlloc->GetSuffix());

                            accessor(arg).eVariableName = resourceName;
                            ++resourceCountInBindingGroup;
                        }
                    });

                    if(!uniformVariables.empty())
                    {
                        regAlloc->NewBinding(resourceCountInBindingGroup, RHI::BindingType::ConstantBuffer);

                        std::string uniformPropertyStr;
                        for(auto& var : uniformVariables)
                        {
                            uniformPropertyStr += std::format("{} {};", GetShaderUniformTypeName(var.type), var.name);
                        }

                        result.resourceDefinitions += std::format(
                            "struct _rtrc_generated_cbuffer_struct_{} {{ {} }};\n",
                            bindingGroupIndex, uniformPropertyStr);

                        result.resourceDefinitions += std::format(
                            "{}ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> _rtrcBindingGroup{}{};\n",
                            regAlloc->GetPrefix(), bindingGroupIndex, bindingGroupIndex, regAlloc->GetSuffix());

                        ++resourceCountInBindingGroup;
                    }

                    ++bindingGroupIndex;
                }
                else if constexpr(ArgumentTrait::eResourceTrait<Arg>::IsResource)
                {
                    result.defaultBindingGroupLayoutDesc.bindings.push_back(
                        ArgumentTrait::eResourceTrait<Arg>::GetBindingDesc());

                    partialResourceDefinitionsInDefaultBindingGroup.push_back(std::format(
                        "{} _rtrcDefaultBindingGroup_Resource{}",
                        Arg::GetStaticTypeName(), resourceCountInDefaultBindingGroup));

                    resourceBindingTypesInDefaultBindingGroup.push_back(
                        ArgumentTrait::eResourceTrait<Arg>::GetBindingDesc().type);

                    arg.eVariableName = std::format(
                        "_rtrcDefaultBindingGroup_Resource{}", resourceCountInDefaultBindingGroup);

                    ++resourceCountInDefaultBindingGroup;
                }
                else
                {
                    static_assert(ArgumentTrait::eValueTrait<Arg>::IsValue);

                    result.defaultUniformTypes.push_back(ArgumentTrait::eValueTrait<Arg>::Type);

                    arg.eVariableName = std::format(
                        "_rtrcDefaultBindingGroup.Value{}", valueCountInDefaultBindingGroup);

                    ++valueCountInDefaultBindingGroup;
                }
            };
            std::apply([&](auto&...arg) { (AssignArgName(arg), ...); }, args);

            // Default binding group
            if(resourceCountInDefaultBindingGroup || valueCountInDefaultBindingGroup)
            {
                regAlloc->NewBindingGroup(bindingGroupIndex);

                for(size_t i = 0; i < partialResourceDefinitionsInDefaultBindingGroup.size(); ++i)
                {
                    regAlloc->NewBinding(i, resourceBindingTypesInDefaultBindingGroup[i]);

                    result.resourceDefinitions += std::format(
                        "{}{}{};\n",
                        regAlloc->GetPrefix(),
                        partialResourceDefinitionsInDefaultBindingGroup[i],
                        regAlloc->GetSuffix());
                }

                if(valueCountInDefaultBindingGroup)
                {
                    regAlloc->NewBinding(resourceCountInDefaultBindingGroup++, RHI::BindingType::ConstantBuffer);

                    std::string uniformPropertyStr;
                    for(auto&& [index, type] : Enumerate(result.defaultUniformTypes))
                    {
                        uniformPropertyStr += std::format("{} Value{};", GetShaderUniformTypeName(type), index);
                    }

                    result.resourceDefinitions += std::format(
                        "struct _rtrc_generated_cbuffer_struct_default {{ {} }};\n", uniformPropertyStr);

                    result.resourceDefinitions += std::format(
                        "{}ConstantBuffer<_rtrc_generated_cbuffer_struct_default> _rtrcDefaultBindingGroup{};\n",
                        regAlloc->GetPrefix(), regAlloc->GetSuffix());

                    ++resourceCountInDefaultBindingGroup;
                }
            }
        }
        EnableStackVariableAllocation();

        // Record Body

        {
            PushRecordContext(result.recordContext);
            RTRC_SCOPE_EXIT{ PopRecordContext(); };

            GetThreadGroupSizeStack().push(&result.threadGroupSize);
            RTRC_SCOPE_EXIT{ GetThreadGroupSizeStack().pop(); };

            std::apply(std::move(func), args);
        }

        if(result.threadGroupSize == Vector3u(0))
        {
            throw Exception("Thread group size is unspecified");
        }
    }

} // namespace ComputeEntryDetail

template <RtrcDSLBindingGroup T>
RC<BindingGroup> ComputeEntryDetail::BindingGroupArgumentWrapper<T>::GetBindingGroup(Ref<Device> device) const
{
    return bindingGroup.Match(
        [&](const T &data) { return device->CreateBindingGroupWithCachedLayout(data); },
        [&](const RC<BindingGroup> &group) { return group; });
}

template<typename Func>
RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
    BuildComputeEntry(Ref<Device> device, const Func &func)
{
    using Entry = ComputeEntryDetail::Entry<Func>;

    ComputeEntryDetail::ComputeEntryIntermediates intermediates;
    ComputeEntryDetail::RecordComputeKernel(
        device->GetBackendType(), func, intermediates, static_cast<Entry *>(nullptr));

    RC<Shader> shader;
    int defaultBindingGroupIndex = -1;
    std::vector<ComputeEntryDetail::ValueItem> defaultBindingGroupValueItems;
    ComputeEntryDetail::BuildComputeEntry(
        device,
        intermediates.resourceDefinitions,
        intermediates.threadGroupSize,
        intermediates.recordContext,
        intermediates.defaultBindingGroupLayoutDesc,
        intermediates.defaultUniformTypes,
        intermediates.bindingGroupLayoutDescs,
        shader, defaultBindingGroupIndex, defaultBindingGroupValueItems);

    Entry ret;
    ret.untypedComputeEntry_ = MakeRC<UntypedComputeEntry>();
    ret.untypedComputeEntry_->shader_                        = std::move(shader);
    ret.untypedComputeEntry_->defaultBindingGroupIndex_      = defaultBindingGroupIndex;
    ret.untypedComputeEntry_->defaultBindingGroupValueItems_ = std::move(defaultBindingGroupValueItems);
    return ret;
}

template<typename...KernelArgs>
void DeclareRenderGraphResourceUses(
    RGPass                                              pass,
    const ComputeEntry<KernelArgs...>                  &entry,
    const ComputeEntryDetail::InvokeType<KernelArgs>&...args)
{
    auto HandleArg = [&]<typename Arg>(const Arg &arg)
    {
        arg.DeclareRenderGraphResourceUsage(pass, RHI::PipelineStageFlag::ComputeShader);
    };
    (HandleArg(args), ...);
}

template<typename...KernelArgs>
void SetupComputeEntry(
    CommandBuffer                                      &commandBuffer,
    const ComputeEntry<KernelArgs...>                  &entry,
    const ComputeEntryDetail::InvokeType<KernelArgs>&...args)
{
    auto device = commandBuffer.GetDevice();

    // Bind pipeline & binding groups

    commandBuffer.BindComputePipeline(entry.GetShader()->GetComputePipeline());

    int bindingGroupIndex = 0;
    auto BindCustomBindingGroup = [&]<typename Arg>(const Arg &arg)
    {
        if constexpr(requires{ arg.GetBindingGroup(device); })
        {
            commandBuffer.BindComputeGroup(bindingGroupIndex++, arg.GetBindingGroup(device));
        }
    };
    (BindCustomBindingGroup(args), ...);

    // Create default binding group

    RC<BindingGroup> defaultBindingGroup;
    if(const int defaultBindingGroupIndex = entry.GetDefaultBindingGroupIndex(); defaultBindingGroupIndex >= 0)
    {
        RC<BindingGroup> group =
            entry.GetShader()->GetBindingGroupLayoutByIndex(defaultBindingGroupIndex)->CreateBindingGroup();

        RHI::BindingGroupUpdateBatch batch;
        int slot = 0;

        // Resources

        auto BindResourceInDefaultBindingGroup = [&]<typename Arg>(const Arg &arg)
        {
            if constexpr(requires { Arg::Proxy::BindingType; })
            {
                if constexpr(Arg::Proxy::BindingType == RHI::BindingType::ConstantBuffer)
                {
                    RC<SubBuffer> constantBuffer = arg.GetRtrcObject();
                    if(!constantBuffer)
                    {
                        using CBufferStruct = typename Arg::Proxy::Struct;
                        constantBuffer = device->CreateConstantBuffer(static_cast<const CBufferStruct &>(arg.proxy));
                    }
                    batch.Append(*group->GetRHIObject(), slot, RHI::ConstantBufferUpdate
                    {
                        constantBuffer->GetFullBufferRHIObject().Get(),
                        constantBuffer->GetSubBufferOffset(),
                        constantBuffer->GetSubBufferSize()
                    });
                    group->SetHeldElement(slot, 0, std::move(constantBuffer));
                    ++slot;
                }
                else if constexpr(IsRC<std::remove_cvref_t<decltype(arg.GetRtrcObject())>>)
                {
                    if(auto obj = arg.GetRtrcObject())
                    {
                        batch.Append(*group->GetRHIObject(), slot, obj->GetRHIObject());
                        group->SetHeldElement(slot, 0, std::move(obj));
                    }
                    ++slot;
                }
                else
                {
                    auto rtrcObj = arg.GetRtrcObject();
                    if(auto rhiObj = rtrcObj.GetRHIObject())
                    {
                        batch.Append(*group->GetRHIObject(), slot, std::move(rhiObj));
                        group->SetHeldElement(slot, 0, std::move(rtrcObj));
                    }
                    ++slot;
                }
            }
        };
        (BindResourceInDefaultBindingGroup(args), ...);

        // Uniforms

        if(auto items = entry.GetValueItems(); !items.IsEmpty())
        {
            const size_t size = UpAlignTo<size_t>(items.last().size + items.last().offset, 16);
            std::vector<unsigned char> data(size);

            int valueIndex = 0;
            auto FlattenUniformValue = [&]<typename Arg>(const Arg &arg)
            {
                if constexpr(requires{ typename Arg::NativeType; })
                {
                    const ComputeEntryDetail::ValueItem &item = items[valueIndex];
                    std::memcpy(data.data() + item.offset, &arg.value, sizeof(arg.value));
                    valueIndex++;
                }
            };
            (FlattenUniformValue(args), ...);

            auto uniformBuffer = device->CreateConstantBuffer(data.data(), data.size());
            batch.Append(*group->GetRHIObject(), slot, RHI::ConstantBufferUpdate
            {
                uniformBuffer->GetFullBufferRHIObject().Get(),
                uniformBuffer->GetSubBufferOffset(),
                uniformBuffer->GetSubBufferSize()
            });
            group->SetHeldElement(slot, 0, std::move(uniformBuffer));
            ++slot;
        }

        if(slot > 0)
        {
            device->GetRawDevice()->UpdateBindingGroups(batch);
        }

        commandBuffer.BindComputeGroup(defaultBindingGroupIndex, group);
    }
}

inline void SetThreadGroupSize(const Vector3u &size)
{
    auto &stack = ComputeEntryDetail::GetThreadGroupSizeStack();
    *stack.top() = size;
}

namespace SV
{

    inline u32x3 GetGroupID()
    {
        GetCurrentRecordContext().SetBuiltinValueRead(RecordContext::BuiltinValue::GroupID);
        return CreateTemporaryVariableForExpression<u32x3>("_rtrcSV_GroupID");
    }

    inline u32x3 GetGroupThreadID()
    {
        GetCurrentRecordContext().SetBuiltinValueRead(RecordContext::BuiltinValue::GroupThreadID);
        return CreateTemporaryVariableForExpression<u32x3>("_rtrcSV_GroupThreadID");
    }

    inline u32x3 GetDispatchThreadID()
    {
        GetCurrentRecordContext().SetBuiltinValueRead(RecordContext::BuiltinValue::DispatchThreadID);
        return CreateTemporaryVariableForExpression<u32x3>("_rtrcSV_DispatchThreadID");
    }

} // namespace SV

RTRC_EDSL_END
