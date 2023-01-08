#pragma once

#include <map>
#include <mutex>
#include <limits>

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename Tag, typename Index>
class PooledString
{
public:

    static constexpr Index INVALID_INDEX = std::numeric_limits<Index>::max();

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
        const Index newIndex = static_cast<Index>(stringToIndex_.size());
        stringToIndex_.insert({ std::string(str), newIndex });
        indexToString_.push_back(std::string(str));
        return newIndex;
    }

    const std::string &GetString(const PooledString<Tag, Index> &str)
    {
        return indexToString_[str.GetIndex()];
    }

    std::map<std::string, Index, std::less<>> stringToIndex_;
    std::vector<std::string> indexToString_;
    std::mutex mutex_;
};

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
