#pragma once

#include <cassert>

#include <magic_enum.hpp>

#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Shader/DSL/BindingGroup.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ComputeEntry.h>
#include <Rtrc/Graphics/Shader/ShaderBuilder.h>
#include <Rtrc/ShaderCommon/Preprocess/RegisterAllocator.h>
#include <Rtrc/ShaderCommon/Preprocess/ShaderPreprocessing.h>

RTRC_EDSL_BEGIN

namespace ComputeEntryDetail
{

    inline auto &GetThreadGroupSizeStack()
    {
        static std::stack<Vector3u *> ret;
        return ret;
    }

    template<typename T>
    struct eResourceTrait
    {
        static constexpr bool IsResource = false;
    };

    template<typename T, TemplateBufferType Type>
    struct eResourceTrait<TemplateBuffer<T, Type>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            constexpr RHI::BindingType TYPE_MAP[] =
            {
                RHI::BindingType::Buffer,
                RHI::BindingType::StructuredBuffer,
                RHI::BindingType::ByteAddressBuffer,
                RHI::BindingType::RWBuffer,
                RHI::BindingType::RWStructuredBuffer,
                RHI::BindingType::RWByteAddressBuffer
            };
            static_assert(GetArraySize(TYPE_MAP) == magic_enum::enum_count<TemplateBufferType>());

            BindingGroupLayout::BindingDesc ret;
            ret.type = TYPE_MAP[std::to_underlying(Type)];
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture1D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture2D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture3D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture1DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture2DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture3DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T>
    struct eResourceTrait<eConstantBuffer<T>>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::ConstantBuffer;
            return ret;
        }
    };

    template<>
    struct eResourceTrait<eRaytracingAccelerationStructure>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::AccelerationStructure;
            return ret;
        }
    };

    template<>
    struct eResourceTrait<eSamplerState>
    {
        static constexpr bool IsResource = true;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::Sampler;
            return ret;
        }
    };

    template<typename T>
    struct eValueTrait
    {
        static constexpr bool IsValue = false;
    };

#define ADD_EVALUE_TRAIT(ETYPE, TYPE)                                      \
    template<>                                                             \
    struct eValueTrait<ETYPE>                                              \
    {                                                                      \
        static constexpr bool IsValue = true;                              \
        static constexpr ShaderUniformType Type = ShaderUniformType::TYPE; \
    }

    ADD_EVALUE_TRAIT(i32, Int);
    ADD_EVALUE_TRAIT(i32x2, Int2);
    ADD_EVALUE_TRAIT(i32x3, Int3);
    ADD_EVALUE_TRAIT(i32x4, Int4);

    ADD_EVALUE_TRAIT(u32, UInt);
    ADD_EVALUE_TRAIT(u32x2, UInt2);
    ADD_EVALUE_TRAIT(u32x3, UInt3);
    ADD_EVALUE_TRAIT(u32x4, UInt4);

    ADD_EVALUE_TRAIT(f32, Float);
    ADD_EVALUE_TRAIT(f32x2, Float2);
    ADD_EVALUE_TRAIT(f32x3, Float3);
    ADD_EVALUE_TRAIT(f32x4, Float4);

    ADD_EVALUE_TRAIT(float4x4, Float4x4);

#undef ADD_EVALUE_TRAIT

    RC<Sampler> CreateSampler(Ref<Device> device, const RHI::SamplerDesc &desc);

    RHI::DeviceOPtr GetRHIDevice(Ref<Device> device);

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
        int                                   &outDefaultBindingGroupIndex);

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
                            var.type = eValueTrait<M>::Type;
                            var.name = name;

                            accessor(arg).eVariableName = fmt::format(
                                "_rtrcBindingGroup{}.{}", bindingGroupIndex, name);
                        }
                        else
                        {
                            regAlloc->NewBinding(resourceCountInBindingGroup, eResourceTrait<M>::GetBindingDesc().type);

                            const std::string resourceName = fmt::format(
                                "_rtrcBindingGroup{}_{}", bindingGroupIndex, name);
                            result.resourceDefinitions += fmt::format(
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
                            uniformPropertyStr += fmt::format("{} {};", GetShaderUniformTypeName(var.type), var.name);
                        }

                        result.resourceDefinitions += fmt::format(
                            "struct _rtrc_generated_cbuffer_struct_{} {{ {} }};\n",
                            bindingGroupIndex, uniformPropertyStr);

                        result.resourceDefinitions += fmt::format(
                            "{}ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> _rtrcBindingGroup{}{};\n",
                            regAlloc->GetPrefix(), bindingGroupIndex, bindingGroupIndex, regAlloc->GetSuffix());

                        ++resourceCountInBindingGroup;
                    }

                    ++bindingGroupIndex;
                }
                else if constexpr(eResourceTrait<Arg>::IsResource)
                {
                    result.defaultBindingGroupLayoutDesc.bindings.push_back(eResourceTrait<Arg>::GetBindingDesc());

                    partialResourceDefinitionsInDefaultBindingGroup.push_back(fmt::format(
                        "{} _rtrcDefaultBindingGroup_Resource{}",
                        Arg::GetStaticTypeName(), resourceCountInDefaultBindingGroup));

                    resourceBindingTypesInDefaultBindingGroup.push_back(eResourceTrait<Arg>::GetBindingDesc().type);

                    arg.eVariableName = fmt::format(
                        "_rtrcDefaultBindingGroup_Resource{}", resourceCountInDefaultBindingGroup);

                    ++resourceCountInDefaultBindingGroup;
                }
                else
                {
                    static_assert(eValueTrait<Arg>::IsValue);

                    result.defaultUniformTypes.push_back(eValueTrait<Arg>::Type);

                    arg.eVariableName = fmt::format(
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

                    result.resourceDefinitions += fmt::format(
                        "{}{}{};\n",
                        regAlloc->GetPrefix(),
                        partialResourceDefinitionsInDefaultBindingGroup[i],
                        regAlloc->GetSuffix());
                }

                if(valueCountInDefaultBindingGroup)
                {
                    regAlloc->NewBinding(resourceCountInDefaultBindingGroup++, RHI::BindingType::ConstantBuffer);

                    std::string uniformPropertyStr;
                    for(auto&& [index, type] : std::ranges::enumerate_view(result.defaultUniformTypes))
                    {
                        uniformPropertyStr += fmt::format("{} Value{};", GetShaderUniformTypeName(type), index);
                    }

                    result.resourceDefinitions += fmt::format(
                        "struct _rtrc_generated_cbuffer_struct_default {{ {} }};\n", uniformPropertyStr);

                    result.resourceDefinitions += fmt::format(
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

template<typename Func>
RTRC_INTELLISENSE_SELECT(auto, ComputeEntryDetail::Entry<Func>)
    BuildComputeEntry(Ref<Device> device, const Func &func)
{
    using Entry = ComputeEntryDetail::Entry<Func>;

    ComputeEntryDetail::ComputeEntryIntermediates intermediates;
    ComputeEntryDetail::RecordComputeKernel(
        ComputeEntryDetail::GetRHIDevice(device)->GetBackendType(), func, intermediates, static_cast<Entry *>(nullptr));

    RC<Shader> shader;
    int defaultBindingGroupIndex = -1;
    ComputeEntryDetail::BuildComputeEntry(
        device,
        intermediates.resourceDefinitions,
        intermediates.threadGroupSize,
        intermediates.recordContext,
        intermediates.defaultBindingGroupLayoutDesc,
        intermediates.defaultUniformTypes,
        intermediates.bindingGroupLayoutDescs,
        shader, defaultBindingGroupIndex);

    Entry ret;
    ret.shader_ = std::move(shader);
    ret.defaultBindingGroupIndex_ = defaultBindingGroupIndex;
    return ret;
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
