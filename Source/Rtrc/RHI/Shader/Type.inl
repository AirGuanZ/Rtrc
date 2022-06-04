#pragma once

RTRC_BEGIN

template<BindingGroupStruct Struct>
const RHI::BindingGroupLayoutDesc &GetBindingGroupLayoutDesc()
{
    static const RHI::BindingGroupLayoutDesc result = []
    {
        RHI::BindingGroupLayoutDesc groupLayoutDesc;

        // bindings / cbuffers

        Struct::template ForEachMember<EnumFlags(ShaderDetail::MemberTag::Binding)>(
            [&]<typename Binding>
        {
            const RHI::BindingDesc desc = {
                .type         = Binding::BindingType,
                .shaderStages = Binding::ShaderStages,
                .isArray      = Binding::IsArray,
                .arraySize    = static_cast<uint32_t>(Binding::ArraySize)
            };
            groupLayoutDesc.bindings.push_back({ desc });
        });

        // cbuffers

        Struct::template ForEachMember<EnumFlags(ShaderDetail::MemberTag::CBuffer)>(
            [&]<typename CBuffer>(CBuffer Struct::*, const char *, RHI::ShaderStageFlag stages)
        {
            const RHI::BindingDesc desc = {
                .type         = RHI::BindingType::ConstantBuffer,
                .shaderStages = stages,
                .isArray      = false,
                .arraySize    = 0
            };
            groupLayoutDesc.bindings.push_back({ desc });
        });

        return groupLayoutDesc;
    }();

    return result;
}

RTRC_END
