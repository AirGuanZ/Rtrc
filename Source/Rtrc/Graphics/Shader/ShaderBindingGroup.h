#pragma once

#include <any>
#include <map>

#include <Rtrc/Graphics/RHI/RHI.h>

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

    BindingGroup(RC<const BindingGroupLayout> parentLayout, RHI::BindingGroupPtr rhiGroup);

    const RC<const BindingGroupLayout> &GetBindingGroupLayout();

    void Set(int slot, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(int slot, const RHI::SamplerPtr &sampler);
    void Set(int slot, const RHI::BufferSRVPtr &srv);
    void Set(int slot, const RHI::BufferUAVPtr &uav);
    void Set(int slot, const RHI::TextureSRVPtr &srv);
    void Set(int slot, const RHI::TextureUAVPtr &uav);

    void Set(std::string_view name, const RHI::BufferPtr &cbuffer, size_t offset, size_t bytes);
    void Set(std::string_view name, const RHI::SamplerPtr &sampler);
    void Set(std::string_view name, const RHI::BufferSRVPtr &srv);
    void Set(std::string_view name, const RHI::BufferUAVPtr &uav);
    void Set(std::string_view name, const RHI::TextureSRVPtr &srv);
    void Set(std::string_view name, const RHI::TextureUAVPtr &uav);

    RHI::BindingGroupPtr GetRHIBindingGroup();

private:

    RC<const BindingGroupLayout> parentLayout_;
    RHI::BindingGroupPtr rhiGroup_;
    std::vector<RHI::Ptr<RHI::RHIObject>> boundObjects_;
};

class BindingGroupLayout : public Uncopyable, public std::enable_shared_from_this<BindingGroupLayout>
{
public:

    int GetBindingSlotByName(std::string_view bindingName) const;
    RHI::BindingGroupLayoutPtr GetRHIBindingGroupLayout();

    RC<BindingGroup> CreateBindingGroup() const;

private:

    friend class ShaderCompiler;

    std::map<std::string, int, std::less<>> bindingNameToSlot_;
    RHI::DevicePtr device_;
    RHI::BindingGroupLayoutPtr rhiLayout_;
};

RTRC_END
