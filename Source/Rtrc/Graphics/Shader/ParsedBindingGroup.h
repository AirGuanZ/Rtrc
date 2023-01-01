#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

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

inline void ParsedBindingGroupLayout::AppendSlot(Span<ParsedBindingDescription> aliasedDescs)
{
    const uint32_t offset = static_cast<uint32_t>(allDescs_.size());
    allDescs_.reserve(allDescs_.size() + aliasedDescs.size());
    std::copy(aliasedDescs.begin(), aliasedDescs.end(), std::back_inserter(allDescs_));
    slotRecords_.push_back({ offset, aliasedDescs.GetSize() });
}

inline int ParsedBindingGroupLayout::GetBindingSlotCount() const
{
    return static_cast<int>(slotRecords_.size());
}

inline Span<ParsedBindingGroupLayout::ParsedBindingDescription>
    ParsedBindingGroupLayout::GetBindingDescriptionsBySlot(int slot) const
{
    const Record &record = slotRecords_[slot];
    return Span(&allDescs_[record.offset], record.count);
}

RTRC_END
