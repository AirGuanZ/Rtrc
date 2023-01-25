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

class KeywordValueContext
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
    static constexpr int MAX_TOTAL_VALUE_BIT_COUNT = 32;

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
        assert(totalBitCount_ <= MAX_TOTAL_VALUE_BIT_COUNT && "number of keyword value bits exceeds max limit (32)");
    }

    ValueMask ExtractValueMask(const KeywordValueContext &valueContext) const
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

    PartialValueMask GeneratePartialValue(const KeywordValueContext &valueContext) const
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
    int totalBitCount_ = 0;
    std::vector<Record> records_;
};

template<TemplateStringParameter KeywordString>
const Keyword &GetKeyword()
{
    static Keyword ret(KeywordString.GetString());
    return ret;
}

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_KEYWORD(X) ([]() -> const ::Rtrc::Keyword& { static ::Rtrc::Keyword ret(#X); return ret; }())
#else
#define RTRC_KEYWORD(X) (::Rtrc::GetKeyword<::Rtrc::TemplateStringParameter(#X)>())
#endif

RTRC_END
