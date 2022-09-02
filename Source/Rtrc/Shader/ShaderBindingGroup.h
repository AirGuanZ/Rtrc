#pragma once

#include <map>

#include <Rtrc/RHI/RHI.h>

RTRC_BEGIN

class BindingGroupLayout;
class ShaderCompiler;

class ParsedBindingGroupLayout : public Uncopyable
{
public:

    using ParsedBindingDescription = RHI::BindingDesc;

    void AppendSlot(Span<ParsedBindingDescription> aliasedDescs);

    int GetBindingSlotCount() const;
    Span<ParsedBindingDescription> GetBindingDescriptionsBySlot(int slot) const;

private:

    struct Record
    {
        uint32_t offset;
        uint32_t count;
    };

    std::vector<Record> slotRecords_;
    std::vector<ParsedBindingDescription> allDescs_;
};

class BindingGroup : public Uncopyable
{
public:

    BindingGroup(const BindingGroupLayout *parentLayout, RHI::BindingGroupPtr rhiGroup);

    void Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(int slot, const RHI::SamplerPtr &sampler);
    void Set(int slot, const RHI::BufferSRVPtr &srv);
    void Set(int slot, const RHI::BufferUAVPtr &uav);
    void Set(int slot, const RHI::Texture2DSRVPtr &srv);
    void Set(int slot, const RHI::Texture2DUAVPtr &uav);

    void Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(std::string_view name, const RHI::SamplerPtr &sampler);
    void Set(std::string_view name, const RHI::BufferSRVPtr &srv);
    void Set(std::string_view name, const RHI::BufferUAVPtr &uav);
    void Set(std::string_view name, const RHI::Texture2DSRVPtr &srv);
    void Set(std::string_view name, const RHI::Texture2DUAVPtr &uav);

    RHI::BindingGroupPtr GetRHIBindingGroup();

private:

    const BindingGroupLayout *parentLayout_;
    RHI::BindingGroupPtr rhiGroup_;
};

class BindingGroupLayout : public Uncopyable
{
public:

    const std::string &GetGroupName() const;
    int GetBindingSlotByName(std::string_view bindingName) const;
    RHI::BindingGroupLayoutPtr GetRHIBindingGroupLayout();

    RC<BindingGroup> CreateBindingGroup() const;

private:

    friend class ShaderCompiler;

    std::string groupName_;
    std::map<std::string, int, std::less<>> bindingNameToSlot_;
    RHI::BindingGroupLayoutPtr rhiLayout_;
};

RTRC_END
