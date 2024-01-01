#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Container/StaticVector.h>
#include <Rtrc/Core/StringPool.h>

RTRC_BEGIN

class FastKeywordOverrider;
class FastKeywordSet;

struct FastShaderKeywordTag {};
using FastShaderKeyword = PooledString<FastShaderKeywordTag, uint32_t>;
#define RTRC_FAST_KEYWORD(NAME) RTRC_POOLED_STRING(::Rtrc::FastShaderKeyword, NAME)

class FastKeywordSetValue
{
    friend class FastKeywordOverrider;
    friend class FastKeywordSet;

    uint32_t value_ = 0;

public:

    auto operator<=>(const FastKeywordSetValue &) const = default;

    size_t Hash() const
    {
        return Rtrc::Hash(value_);
    }

    uint32_t GetInternalValue() const
    {
        return value_;
    }
};

class FastKeywordOverrider
{
public:

    FastKeywordSetValue Override(FastKeywordSetValue originalValue) const
    {
        assert((value_ & ~mask_) == 0);
        FastKeywordSetValue ret;
        ret.value_ = (originalValue.value_ & ~mask_) | value_;
        return ret;
    }

private:

    friend class FastKeywordSet;

    uint32_t mask_ = 0;
    uint32_t value_ = 0;
};

class FastKeywordContext
{
public:

    void Set(FastShaderKeyword keyword, unsigned int value)
    {
        if(keyword.GetIndex() >= records_.size())
        {
            records_.resize(keyword.GetIndex() + 1);
        }
        Record &record = records_[keyword.GetIndex()];
        record.value = value;
        record.valid = 1;
    }

    bool Has(FastShaderKeyword keyword) const
    {
        return keyword.GetIndex() < records_.size() ? records_[keyword.GetIndex()].valid : false;
    }

    unsigned int Get(FastShaderKeyword keyword) const
    {
        return keyword.GetIndex() < records_.size() ? records_[keyword.GetIndex()].valid : 0;
    }

private:

    friend class FastKeywordSet;

    struct Record
    {
        uint8_t value : 7 = 0;
        uint8_t valid : 1 = 0;
    };
    static_assert(sizeof(Record) == 1);

    std::vector<Record> records_;
};

class FastKeywordSet
{
public:

    struct InputRecord
    {
        FastShaderKeyword keyword;
        int               bitCount;
    };

    FastKeywordSet() = default;

    explicit FastKeywordSet(Span<InputRecord> records)
    {
        records_.Resize(records.size());
        for(size_t i = 0; i < records.size(); ++i)
        {
            records_[i].keyword = records[i].keyword;
            records_[i].bitCount = records[i].bitCount;
        }
        if(!records_.IsEmpty())
        {
            records_[0].bitOffset = 0;
            for(size_t i = 1; i < records_.GetSize(); ++i)
            {
                records_[i].bitOffset = records_[i - 1].bitOffset + records_[i - 1].bitCount;
            }
        }
    }

    FastKeywordOverrider CreateOverrider(const FastKeywordContext &context) const
    {
        FastKeywordOverrider ret;
        ret.value_ = 0;
        ret.mask_ = 0;
        for(auto &record : records_)
        {
            if(record.keyword.GetIndex() >= context.records_.size())
            {
                break;
            }
            auto &contextRecord = context.records_[record.keyword.GetIndex()];
            if(contextRecord.valid)
            {
                ret.value_ |= contextRecord.valid << record.bitOffset;
                ret.mask_ |= BitCountToBitMask(record.bitCount) << record.bitOffset;
            }
        }
        return ret;
    }

    FastKeywordSetValue ExtractValue(
        const FastKeywordContext  &context,
        const FastKeywordSetValue &defaultValue = {}) const
    {
        FastKeywordSetValue ret;
        ret.value_ = 0;
        for(auto &record : records_)
        {
            if(record.keyword.GetIndex() < context.records_.size())
            {
                auto &contextRecord = context.records_[record.keyword.GetIndex()];
                if(contextRecord.valid)
                {
                    ret.value_ |= contextRecord.value << record.bitOffset;
                    continue;
                }
            }
            ret.value_ |= defaultValue.value_ & (BitCountToBitMask(record.bitCount) << record.bitOffset);
        }
        return ret;
    }

    int GetKeywordCount() const
    {
        return records_.GetSize();
    }

    int ExtractSingleKeywordValue(int index, FastKeywordSetValue value) const
    {
        auto &record = records_[index];
        return (value.GetInternalValue() >> record.bitOffset) & BitCountToBitMask(record.bitCount);
    }

    int GetTotalBitCount() const
    {
        int ret = 0;
        for(auto &record : records_)
        {
            ret += record.bitCount;
        }
        return ret;
    }

    bool HasKeyword(FastShaderKeyword keyword) const
    {
        for(auto &record : records_)
        {
            if(record.keyword == keyword)
            {
                return true;
            }
        }
        return false;
    }

private:

    static uint32_t BitCountToBitMask(int count)
    {
        assert(1 <= count && count < 8);
        constexpr uint32_t ret[] =
        {
            0b0000000u,
            0b0000001u,
            0b0000011u,
            0b0000111u,
            0b0001111u,
            0b0011111u,
            0b0111111u,
            0b1111111u
        };
        return ret[count];
    }

    struct Record
    {
        FastShaderKeyword keyword;
        uint8_t bitCount;
        uint8_t bitOffset; // Exclusive prefix sum of bitCount
    };

    StaticVector<Record, 32> records_;
};

enum class BuiltinKeyword
{
    EnableInstance,
    Count
};

inline FastShaderKeyword GetBuiltinShaderKeyword(BuiltinKeyword keyword)
{
    static const FastShaderKeyword ret[] =
    {
        FastShaderKeyword("ENABLE_INSTANCE")
    };
    return ret[std::to_underlying(keyword)];
}

RTRC_END
