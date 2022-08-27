#include <filesystem>
#include <iterator>

#include <fmt/format.h>

#include <Rtrc/Shader/ShaderBindingGroup.h>
#include <Rtrc/Utils/Enumerate.h>

RTRC_BEGIN

void ParsedBindingGroupLayout::AppendSlot(Span<ParsedBindingDescription> aliasedDescs)
{
    const uint32_t offset = static_cast<uint32_t>(allDescs_.size());
    allDescs_.reserve(allDescs_.size() + aliasedDescs.size());
    std::copy(aliasedDescs.begin(), aliasedDescs.end(), std::back_inserter(allDescs_));
    slotRecords_.push_back({ offset, aliasedDescs.GetSize() });
}

int ParsedBindingGroupLayout::GetBindingSlotCount() const
{
    return static_cast<int>(slotRecords_.size());
}

Span<ParsedBindingGroupLayout::ParsedBindingDescription>
    ParsedBindingGroupLayout::GetBindingDescriptionsBySlot(int slot) const
{
    const Record &record = slotRecords_[slot];
    return Span(&allDescs_[record.offset], record.count);
}

BindingGroup::BindingGroup(const BindingGroupLayout *parentLayout, RHI::BindingGroupPtr rhiGroup)
    : parentLayout_(parentLayout), rhiGroup_(std::move(rhiGroup))
{
    
}

void BindingGroup::Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    rhiGroup_->ModifyMember(slot, cbuffer, offset, bytes);
}

void BindingGroup::Set(int slot, const RHI::SamplerPtr &sampler)
{
    rhiGroup_->ModifyMember(slot, sampler);
}

void BindingGroup::Set(int slot, const RHI::BufferSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
}

void BindingGroup::Set(int slot, const RHI::BufferUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
}

void BindingGroup::Set(int slot, const RHI::Texture2DSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
}

void BindingGroup::Set(int slot, const RHI::Texture2DUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, cbuffer, offset, bytes);
}

void BindingGroup::Set(std::string_view name, const RHI::SamplerPtr &sampler)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, sampler);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const RHI::BufferUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

void BindingGroup::Set(std::string_view name, const RHI::Texture2DSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void BindingGroup::Set(std::string_view name, const RHI::Texture2DUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

RHI::BindingGroupPtr BindingGroup::GetRHIBindingGroup()
{
    return rhiGroup_;
}

const std::string &BindingGroupLayout::GetGroupName() const
{
    return groupName_;
}

int BindingGroupLayout::GetBindingSlotByName(std::string_view bindingName) const
{
    const auto it = bindingNameToSlot_.find(bindingName);
    if(it == bindingNameToSlot_.end())
    {
        throw Exception(fmt::format("unknown binding name '{}' in group '{}'", bindingName, groupName_));
    }
    return it->second;
}

RHI::BindingGroupLayoutPtr BindingGroupLayout::GetRHIBindingGroupLayout()
{
    return rhiLayout_;
}

RC<BindingGroup> BindingGroupLayout::CreateBindingGroup() const
{
    RHI::BindingGroupPtr rhiGroup = rhiLayout_->CreateBindingGroup();
    return MakeRC<BindingGroup>(this, std::move(rhiGroup));
}

RTRC_END
