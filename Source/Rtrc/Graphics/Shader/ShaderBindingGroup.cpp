#include <filesystem>
#include <iterator>

#include <Rtrc/Graphics/Shader/ShaderBindingGroup.h>
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

TempBindingGroup::TempBindingGroup(RC<const TempBindingGroupLayout> parentLayout, RHI::BindingGroupPtr rhiGroup)
    : parentLayout_(std::move(parentLayout)), rhiGroup_(std::move(rhiGroup))
{
    boundObjects_.resize(rhiGroup_->GetLayout()->GetDesc().bindings.size());
}

const RC<const TempBindingGroupLayout> &TempBindingGroup::GetBindingGroupLayout()
{
    return parentLayout_;
}

void TempBindingGroup::Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    rhiGroup_->ModifyMember(slot, cbuffer, offset, bytes);
    boundObjects_[slot] = cbuffer;
}

void TempBindingGroup::Set(int slot, const RHI::SamplerPtr &sampler)
{
    rhiGroup_->ModifyMember(slot, sampler);
    boundObjects_[slot] = sampler;
}

void TempBindingGroup::Set(int slot, const RHI::BufferSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
    boundObjects_[slot] = srv;
}

void TempBindingGroup::Set(int slot, const RHI::BufferUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
    boundObjects_[slot] = uav;
}

void TempBindingGroup::Set(int slot, const RHI::TextureSRVPtr &srv)
{
    rhiGroup_->ModifyMember(slot, srv);
    boundObjects_[slot] = srv;
}

void TempBindingGroup::Set(int slot, const RHI::TextureUAVPtr &uav)
{
    rhiGroup_->ModifyMember(slot, uav);
    boundObjects_[slot] = uav;
}

void TempBindingGroup::Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, cbuffer, offset, bytes);
}

void TempBindingGroup::Set(std::string_view name, const RHI::SamplerPtr &sampler)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, sampler);
}

void TempBindingGroup::Set(std::string_view name, const RHI::BufferSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void TempBindingGroup::Set(std::string_view name, const RHI::BufferUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

void TempBindingGroup::Set(std::string_view name, const RHI::TextureSRVPtr &srv)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, srv);
}

void TempBindingGroup::Set(std::string_view name, const RHI::TextureUAVPtr &uav)
{
    const int slot = parentLayout_->GetBindingSlotByName(name);
    Set(slot, uav);
}

RHI::BindingGroupPtr TempBindingGroup::GetRHIBindingGroup()
{
    return rhiGroup_;
}

int TempBindingGroupLayout::GetBindingSlotByName(std::string_view bindingName) const
{
    const auto it = bindingNameToSlot_.find(bindingName);
    if(it == bindingNameToSlot_.end())
    {
        throw Exception(fmt::format("unknown binding name '{}' in group", bindingName));
    }
    return it->second;
}

RHI::BindingGroupLayoutPtr TempBindingGroupLayout::GetRHIBindingGroupLayout()
{
    return rhiLayout_;
}

RC<TempBindingGroup> TempBindingGroupLayout::CreateBindingGroup() const
{
    RHI::BindingGroupPtr rhiGroup = device_->CreateBindingGroup(rhiLayout_);
    return MakeRC<TempBindingGroup>(shared_from_this(), std::move(rhiGroup));
}

RTRC_END
