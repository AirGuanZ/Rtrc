#pragma once

#include <cassert>
#include <vector>

#include <Rtrc/Utility/StringPool.h>
#include <Rtrc/Utility/TemplateStringParameter.h>

RTRC_BEGIN

struct PooledStringTagForKeyword { };
using Keyword = PooledString<PooledStringTagForKeyword, uint32_t>;

class KeywordSet;
class KeywordSetPartialValue;

enum class BuiltinKeyword
{
    EnableInstance, // ENABLE_INSTANCE
    Count
};

inline const Keyword &GetBuiltinKeyword(BuiltinKeyword k)
{
    static const Keyword KEYWORDS[] =
    {
        Keyword("ENABLE_INSTANCE")
    };
    return KEYWORDS[std::to_underlying(k)];
}

inline std::string_view GetBuiltinKeywordString(BuiltinKeyword k)
{
    static constexpr std::string_view STRINGS[] =
    {
        "ENABLE_INSTANCE"
    };
    return STRINGS[std::to_underlying(k)];
}

inline int GetBuiltinKeywordBitCount(BuiltinKeyword k)
{
    static constexpr int COUNTS[] =
    {
        1
    };
    return COUNTS[std::to_underlying(k)];
}

class KeywordContext
{
public:

    void Set(const Keyword &keyword, uint8_t value)
    {
        assert(keyword);
        if(keyword.GetIndex() >= values_.size())
        {
            values_.resize(keyword.GetIndex() + 1);
        }
        values_[keyword.GetIndex()] = value;
    }

private:

    friend class KeywordSet;

    std::vector<uint8_t> values_;
};

class KeywordSet
{
public:

    using ValueMask = uint32_t;
    static constexpr int MAX_TOTAL_VALUE_BIT_COUNT = sizeof(ValueMask) * CHAR_BIT;

    class PartialValueMask
    {
    public:

        ValueMask Apply(ValueMask value) const
        {
            return (value & mask_) | value_;
        }

    private:

        friend class KeywordSet;

        ValueMask mask_ = (std::numeric_limits<ValueMask>::max)();
        ValueMask value_ = 0;
    };

    void AddKeyword(const Keyword &keyword, uint8_t bitCount)
    {
        assert(bitCount <= 8);
        records_.push_back({ keyword, bitCount });
        totalBitCount_ += bitCount;
        assert(totalBitCount_ <= MAX_TOTAL_VALUE_BIT_COUNT && "Number of keyword value bits exceeds max limit (32)");
    }

    ValueMask ExtractValueMask(const KeywordContext &valueContext) const
    {
        ValueMask mask = 0;
        int bitOffset = 0;
        for(auto &r : records_)
        {
            const int value = r.keyword.GetIndex() < valueContext.values_.size() ?
                              valueContext.values_[r.keyword.GetIndex()] : 0;
            assert(value < (1 << r.bitCountInMask));
            const uint8_t localMask = BitCountToMask(r.bitCountInMask);
            mask |= (value & localMask) << bitOffset;
            bitOffset += r.bitCountInMask;
        }
        return mask;
    }

    PartialValueMask GeneratePartialValue(const KeywordContext &valueContext) const
    {
        ValueMask mask = 0, value = 0;
        int bitOffset = 0;
        for(auto &r : records_)
        {
            if(r.keyword.GetIndex() >= valueContext.values_.size())
            {
                bitOffset += r.bitCountInMask;
                continue;
            }

            const int kwValue = valueContext.values_[r.keyword.GetIndex()];
            assert(kwValue < (1 << r.bitCountInMask));

            const uint8_t localMask = BitCountToMask(r.bitCountInMask);
            mask |= localMask << bitOffset;
            value |= (kwValue & localMask) << bitOffset;
            bitOffset += r.bitCountInMask;
        }
        PartialValueMask result;
        result.mask_ = ~mask;
        result.value_ = value;
        return result;
    }

    template<typename Func>
    void ForEachKeywordAndValue(ValueMask valueMask, const Func &func) const
    {
        for(auto &r : records_)
        {
            const uint8_t value = valueMask & BitCountToMask(r.bitCountInMask);
            func(r.keyword, value);
            valueMask >>= r.bitCountInMask;
        }
    }

    int GetTotalBitCount() const
    {
        return totalBitCount_;
    }

private:

    static uint8_t BitCountToMask(uint8_t count)
    {
        static const uint8_t MASKS[9] =
        {
            0b00000000,
            0b00000001,
            0b00000011,
            0b00000111,
            0b00001111,
            0b00011111,
            0b00111111,
            0b01111111,
            0b11111111
        };
        assert(count < 9);
        return MASKS[count];
    }

    struct Record
    {
        Keyword keyword;
        uint8_t bitCountInMask;
    };
    int                 totalBitCount_ = 0;
    std::vector<Record> records_;
};

#define RTRC_KEYWORD(X) RTRC_POOLED_STRING(::Rtrc::Keyword, X)

RTRC_END
