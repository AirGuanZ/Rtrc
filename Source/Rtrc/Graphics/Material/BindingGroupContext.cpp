#include <ranges>

#include <Rtrc/Graphics/Material/BindingGroupContext.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

void BindingGroupContext::Clear()
{
    lastCommandBuffer_ = nullptr;
    nameToRecord_.clear();
}

void BindingGroupContext::Set(std::string name, RC<BindingGroup> bindingGroup)
{
    nameToRecord_[std::move(name)] = { std::move(bindingGroup) };
}

const RC<BindingGroup> &BindingGroupContext::Get(std::string_view name) const
{
    static const RC<BindingGroup> NIL;
    auto it = nameToRecord_.find(name);
    return it != nameToRecord_.end() ? it->second.group : NIL;
}

void BindingGroupContext::ClearBindingRecords()
{
    lastCommandBuffer_ = nullptr;
    lastGraphicsShader_ = nullptr;
    lastComputeShader_ = nullptr;
}

void BindingGroupContext::BindForGraphicsPipeline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader)
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

void BindingGroupContext::BindForComputePipeline(const RHI::CommandBufferPtr &commandBuffer, const RC<Shader> &shader)
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
