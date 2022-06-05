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
            [&]<typename Binding>(Binding Struct::*)
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

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroupInstance(
    RHI::BindingGroup        *instance,
    Member Struct::          *member,
    const RC<RHI::BufferSRV> &srv)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct:: * p)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, srv);
            }
        }
        ++index;
    };
    auto processCBuffer = [&]<typename CBuffer>(CBuffer Struct:: *, const char *, RHI::ShaderStageFlag)
    {
        ++index;
    };
    Struct::ForEachMember(LayoutMemberProcessor(processBinding, processCBuffer));
}

template<typename T>
RC<RHI::BindingGroupLayout> RHI::Device::CreateBindingGroupLayout()
{
    return this->CreateBindingGroupLayout(GetBindingGroupLayoutDesc<T>());
}

template<typename Struct, typename Member>
void RHI::BindingGroup::ModifyMember(Member Struct::*member, const RC<BufferSRV> &bufferSRV)
{
    ModifyBindingGroupInstance(this, member, bufferSRV);
}

RTRC_END
