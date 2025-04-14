#pragma once

#include <map>
#include <mutex>
#include <limits>
#include <shared_mutex>

#include <Rtrc/Core/TemplateStringParameter.h>

RTRC_BEGIN

template<typename Tag, typename Index>
class PooledString
{
public:

    static constexpr Index INVALID_INDEX = (std::numeric_limits<Index>::max)();

    static PooledString FromIndex(Index index);

    PooledString();
    PooledString(std::string_view str);
    PooledString(const char *c);

    operator bool() const;
    Index GetIndex() const;
    const std::string &GetString() const;

    auto operator<=>(const PooledString &) const = default;
    bool operator==(const PooledString &) const = default;

private:

    Index index_;
};

template<typename Tag, typename Index>
class StringPool
{
    friend class PooledString<Tag, Index>;

    static StringPool &GetInstance()
    {
        static StringPool ret;
        return ret;
    }

    Index GetIndex(std::string_view str)
    {
        std::lock_guard lock(mutex_);
        if(auto it = stringToIndex_.find(str); it != stringToIndex_.end())
        {
            return it->second;
        }
        Index newIndex;
        {
            std::lock_guard lock2(indexToStringMutex_);
            newIndex = static_cast<Index>(indexToString_.size());
            indexToString_.push_back(std::string(str));
        }
        stringToIndex_.insert({ std::string(str), newIndex });
        return newIndex;
    }

    const std::string &GetString(const PooledString<Tag, Index> &str)
    {
        std::shared_lock lock(indexToStringMutex_);
        return indexToString_[str.GetIndex()];
    }

    std::map<std::string, Index, std::less<>> stringToIndex_;
    std::mutex mutex_;

    std::shared_mutex indexToStringMutex_;
    std::vector<std::string> indexToString_;
};

struct GeneralPooledStringTag { };
using GeneralPooledString = PooledString<GeneralPooledStringTag, uint32_t>;
#define RTRC_GENERAL_POOLED_STRING(X) RTRC_POOLED_STRING(::Rtrc::GeneralPooledString, X)

template<typename PooledStringInstance, TemplateStringParameter String>
const PooledStringInstance &GetPooledStringInstance()
{
    static PooledStringInstance ret(String.GetString());
    return ret;
}

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_POOLED_STRING(TYPE, X) ([]() -> const TYPE& { static TYPE ret(#X); return ret; }())
#else
#define RTRC_POOLED_STRING(TYPE, X) (::Rtrc::GetPooledStringInstance<TYPE, ::Rtrc::TemplateStringParameter(#X)>())
#endif

template<typename Tag, typename Index>
PooledString<Tag, Index> PooledString<Tag, Index>::FromIndex(Index index)
{
    PooledString ret;
    ret.index_ = index;
    return ret;
}

template <typename Tag, typename Index>
PooledString<Tag, Index>::PooledString()
    : index_(INVALID_INDEX)
{
    
}

template <typename Tag, typename Index>
PooledString<Tag, Index>::PooledString(std::string_view str)
    : index_(StringPool<Tag, Index>::GetInstance().GetIndex(str))
{
    
}

template <typename Tag, typename Index>
PooledString<Tag, Index>::PooledString(const char *c)
    : PooledString(std::string_view(c))
{
    
}

template <typename Tag, typename Index>
PooledString<Tag, Index>::operator bool() const
{
    return index_ != INVALID_INDEX;
}

template <typename Tag, typename Index>
Index PooledString<Tag, Index>::GetIndex() const
{
    return index_;
}

template <typename Tag, typename Index>
const std::string &PooledString<Tag, Index>::GetString() const
{
    assert(index_ != INVALID_INDEX);
    return StringPool<Tag, Index>::GetInstance().GetString(*this);
}

RTRC_END

template<typename Tag, typename Index>
struct std::hash<Rtrc::PooledString<Tag, Index>>
{
    size_t operator()(const Rtrc::PooledString<Tag, Index> &s) const noexcept
    {
        return std::hash<Index>{}(s.GetIndex());
    }
};

template<typename Tag, typename Index>
struct std::formatter<Rtrc::PooledString<Tag, Index>>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext &ctx)
    {
        return ctx.begin();
    }

    template<typename FormatContext>
    auto format(const Rtrc::PooledString<Tag, Index> &value, FormatContext &ctx)
    {
        return std::format_to(ctx.out(), "{}", value.GetString());
    }
};
