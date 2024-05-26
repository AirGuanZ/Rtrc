#pragma once

#include <cassert>

#include <magic_enum.hpp>

#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Shader/DSL/BindingGroup.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ComputeEntry.h>
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
    struct eResourceTrait<ConstantBuffer<T>>
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
    struct eResourceTrait<RaytracingAccelerationStructure>
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
    struct eResourceTrait<SamplerState>
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

    RHI::BackendType GetBackendType(Ref<Device> device);

    template<typename...Args, typename Func>
    ComputeEntry<Args...> BuildComputeEntryImpl(
        Ref<Device> device, const Func &func, ComputeEntry<Args...> *)
    {
        // Prepare eDSL args
        //    Resource:                          _rtrcBindingGroup{groupIndex}_name
        //    Value:                             _rtrcBindingGroup{groupIndex}.name
        //    Resource in default binding group: _rtrcDefaultBindingGroup_Resource{index}
        //    Value in default binding group:    _rtrcDefaultBindingGroup.Value{index}
        //    Static sampler:                    _rtrcStaticSamplers

        BindingGroupLayout::Desc              defaultBindingGroupLayoutDesc;
        std::vector<ShaderUniformType>        defaultUniformTypes;
        std::vector<BindingGroupLayout::Desc> bindingGroupLayoutDescs;

        std::string resourceDefinitions;

        DisableStackVariableAllocation();
        std::tuple<Args...> args;
        {
            uint32_t bindingGroupIndex = 0;
            uint32_t resourceCountInDefaultBindingGroup = 0;
            uint32_t valueCountInDefaultBindingGroup = 0;

            auto regAlloc = ShaderResourceRegisterAllocator::Create(GetBackendType(device));

            std::vector<std::string> partialResourceDefinitionsInDefaultBindingGroup;
            std::vector<RHI::BindingType> resourceBindingTypesInDefaultBindingGroup;

            auto AssignArgName = [&]<typename Arg>(Arg &arg)
            {
                if constexpr(RtrcDSLBindingGroup<Arg>)
                {
                    using BindingGroupStruct = BindingGroupDetail::RebindSketchBuilder<
                        Arg, BindingGroupDetail::BindingGroupStructBuilder_Default>;
                    bindingGroupLayoutDescs.push_back(BindingGroupDetail::ToBindingGroupLayout<BindingGroupStruct>());

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
                            resourceDefinitions += fmt::format(
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

                        resourceDefinitions += fmt::format(
                            "struct _rtrc_generated_cbuffer_struct_{} {{ {} }};\n",
                            bindingGroupIndex, uniformPropertyStr);

                        resourceDefinitions += fmt::format(
                            "{}ConstantBuffer<_rtrc_generated_cbuffer_struct_{}> _rtrcBindingGroup{}{};\n",
                            regAlloc->GetPrefix(), bindingGroupIndex, bindingGroupIndex, regAlloc->GetSuffix());

                        ++resourceCountInBindingGroup;
                    }

                    ++bindingGroupIndex;
                }
                else if constexpr(eResourceTrait<Arg>::IsResource)
                {
                    defaultBindingGroupLayoutDesc.bindings.push_back(eResourceTrait<Arg>::GetBindingDesc());

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

                    defaultUniformTypes.push_back(eValueTrait<Arg>::Type);

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

                    resourceDefinitions += fmt::format(
                        "{}{}{};\n",
                        regAlloc->GetPrefix(),
                        partialResourceDefinitionsInDefaultBindingGroup[i],
                        regAlloc->GetSuffix());
                }

                if(valueCountInDefaultBindingGroup)
                {
                    regAlloc->NewBinding(resourceCountInDefaultBindingGroup++, RHI::BindingType::ConstantBuffer);

                    std::string uniformPropertyStr;
                    for(auto&& [index, type] : std::ranges::enumerate_view(defaultUniformTypes))
                    {
                        uniformPropertyStr += fmt::format("{} Value{};", GetShaderUniformTypeName(type), index);
                    }

                    resourceDefinitions += fmt::format(
                        "struct _rtrc_generated_cbuffer_struct_default {{ {} }};\n", uniformPropertyStr);

                    resourceDefinitions += fmt::format(
                        "{}ConstantBuffer<_rtrc_generated_cbuffer_struct_default> _rtrcDefaultBindingGroup{};\n",
                        regAlloc->GetPrefix(), regAlloc->GetSuffix());

                    ++resourceCountInDefaultBindingGroup;
                }
            }
        }
        EnableStackVariableAllocation();

        // Record Body

        RecordContext context;
        Vector3u threadGroupSize;

        {
            PushRecordContext(context);
            RTRC_SCOPE_EXIT{ PopRecordContext(); };

            GetThreadGroupSizeStack().push(&threadGroupSize);
            RTRC_SCOPE_EXIT{ GetThreadGroupSizeStack().pop(); };

            std::apply(std::move(func), args);
        }

        if(threadGroupSize == Vector3u(0))
        {
            throw Exception("Thread group size is not specified");
        }

        // Default binding group

        UniformBufferLayout defaultUniformBufferLayout;
        size_t defaultUniformBufferSize = 0;
        {
            defaultUniformBufferLayout.variables.reserve(defaultUniformTypes.size());
            for(ShaderUniformType type : defaultUniformTypes)
            {
                auto &variable = defaultUniformBufferLayout.variables.emplace_back();
                variable.type = type;

                variable.size = GetShaderUniformSize(type);
                assert(variable.size % 4 == 0);

                const size_t lineOffset = defaultUniformBufferSize % 16;
                if(lineOffset && lineOffset + variable.size > 16)
                {
                    defaultUniformBufferSize = (defaultUniformBufferSize + 15) / 16 * 16;
                }
                variable.offset = defaultUniformBufferSize;
                defaultUniformBufferSize += variable.size;
            }
            defaultUniformBufferSize = (defaultUniformBufferSize + 15) / 16 * 16;
        }

        if(defaultUniformBufferSize)
        {
            auto &binding = defaultBindingGroupLayoutDesc.bindings.emplace_back();
            binding.type = RHI::BindingType::ConstantBuffer;
        }

        int defaultBindingGroupIndex = -1;
        if(!defaultBindingGroupLayoutDesc.bindings.empty())
        {
            defaultBindingGroupIndex = static_cast<int>(bindingGroupLayoutDescs.size());
            bindingGroupLayoutDescs.push_back(defaultBindingGroupLayoutDesc);
        }

        // Static samplers

        int staticSamplerBindingGroupIndex = -1;
        if(auto &staticSamplerMap = context.GetAllStaticSamplers(); !staticSamplerMap.empty())
        {
            staticSamplerBindingGroupIndex = static_cast<int>(bindingGroupLayoutDescs.size());
            auto &groupLayoutDesc = bindingGroupLayoutDescs.emplace_back();
            auto &binding = groupLayoutDesc.bindings.emplace_back();
            binding.type = RHI::BindingType::Sampler;
            binding.arraySize = static_cast<uint32_t>(staticSamplerMap.size());

            auto &samplers = binding.immutableSamplers;
            samplers.resize(staticSamplerMap.size());
            for(auto &[desc, index] : staticSamplerMap)
            {
                samplers[index] = CreateSampler(device, desc);
            }

            resourceDefinitions += fmt::format(
                "SamplerState _rtrcStaticSamplers[{}];\n", context.GetAllStaticSamplers().size());
        }

        // Generate type definitions

        const std::string typeDefinitions = context.BuildTypeDefinitions();

        // Generate final source

        std::string source;
        source += "#define RTRC_DEFINE_SV_GroupID(NAME)          uint3 NAME : SV_GroupID\n";
        source += "#define RTRC_DEFINE_SV_GroupThreadID(NAME)    uint3 NAME : SV_GroupThreadID\n";
        source += "#define RTRC_DEFINE_SV_DispatchThreadID(NAME) uint3 NAME : SV_DispatchThreadID\n";

        source += typeDefinitions;
        source += resourceDefinitions;

        source += fmt::format("[numthreads({}, {}, {})]\n", threadGroupSize.x, threadGroupSize.y, threadGroupSize.z);

        std::string systemValueDefinitions;
        for(int i = 0; i < std::to_underlying(RecordContext::BuiltinValue::Count); ++i)
        {
            const auto e = static_cast<RecordContext::BuiltinValue>(i);
            if(context.GetBuiltinValueRead(e))
            {
                if(!systemValueDefinitions.empty())
                {
                    systemValueDefinitions += ", ";
                }
                systemValueDefinitions += fmt::format(
                    "RTRC_DEFINE_{}(_rtrc{})",
                    RecordContext::GetBuiltinValueName(e),
                    RecordContext::GetBuiltinValueName(e));
            }
        }
        source += fmt::format("void CSMain({})\n", systemValueDefinitions);

        source += "{\n";
        source += context.BuildRootScope("    ");
        source += "}\n";

        LogInfo(source);

        return {};
    }

} // namespace ComputeEntryDetail

template<typename Func>
auto BuildComputeEntry(Ref<Device> device, const Func &func)
{
    using Entry = typename ComputeEntryDetail::FunctionTrait<Func>::Entry;
    return ComputeEntryDetail::BuildComputeEntryImpl(device, func, static_cast<Entry *>(nullptr));
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
