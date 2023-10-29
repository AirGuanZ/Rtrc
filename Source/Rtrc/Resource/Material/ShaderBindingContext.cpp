#include <ranges>

#include <Graphics/Device/CommandBuffer.h>
#include <Graphics/Shader/Shader.h>
#include <Rtrc/Resource/Material/ShaderBindingContext.h>

RTRC_BEGIN

void ShaderBindingContext::Clear()
{
    nameToRecord_.clear();
}

void ShaderBindingContext::Set(std::string name, RC<BindingGroup> bindingGroup)
{
    nameToRecord_[std::move(name)] = { std::move(bindingGroup) };
}

const RC<BindingGroup> &ShaderBindingContext::Get(std::string_view name) const
{
    static const RC<BindingGroup> NIL;
    auto it = nameToRecord_.find(name);
    return it != nameToRecord_.end() ? it->second.group : NIL;
}

void ShaderBindingContext::ApplyGroupsToGraphicsPipeline(const CommandBuffer &commandBuffer, const RC<Shader> &shader)
{
    ApplyGroupsToGraphicsPipeline(commandBuffer.GetRHIObject(), shader);
}

void ShaderBindingContext::ApplyGroupsToComputePipieline(const CommandBuffer &commandBuffer, const RC<Shader> &shader)
{
    ApplyGroupsToComputePipieline(commandBuffer.GetRHIObject(), shader);
}

void ShaderBindingContext::ApplyGroupsToGraphicsPipeline(
    const RHI::CommandBufferRPtr &commandBuffer, const RC<Shader> &shader)
{
    // TODO: local cache for already bound groups

    const int bindingGroupCount = shader->GetBindingGroupCount();
    for(int i = 0; i < bindingGroupCount; ++i)
    {
        const std::string &groupName = shader->GetBindingGroupNameByIndex(i);

        auto it = nameToRecord_.find(groupName);
        if(it == nameToRecord_.end())
        {
            continue;
        }

        auto &record = it->second;
        if(record.group->GetLayout() != shader->GetBindingGroupLayoutByIndex(i))
        {
            throw Exception(fmt::format(
                "Layouts of binding group {} in shader/bindingGroupContext must be exactly same", groupName));
        }

        commandBuffer->BindGroupToGraphicsPipeline(i, record.group->GetRHIObject());
    }
}

void ShaderBindingContext::ApplyGroupsToComputePipieline(
    const RHI::CommandBufferRPtr &commandBuffer, const RC<Shader> &shader)
{
    // TODO: local cache for already bound groups

    const int bindingGroupCount = shader->GetBindingGroupCount();
    for(int i = 0; i < bindingGroupCount; ++i)
    {
        const std::string &groupName = shader->GetBindingGroupNameByIndex(i);

        auto it = nameToRecord_.find(groupName);
        if(it == nameToRecord_.end())
        {
            continue;
        }

        auto &record = it->second;
        if(record.group->GetLayout() != shader->GetBindingGroupLayoutByIndex(i))
        {
            throw Exception(fmt::format(
                "Layouts of binding group {} in shader/bindingGroupContext must be exactly same", groupName));
        }

        commandBuffer->BindGroupToComputePipeline(i, record.group->GetRHIObject());
    }
}

RTRC_END
