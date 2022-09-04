#pragma once

#include <map>
#include <mutex>

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename Tag>
class PooledString
{
public:

    PooledString();
    PooledString(std::string_view str);
    PooledString(const char *c);

    operator bool() const;
    uint32_t GetIndex() const;
    const std::string &GetString() const;

    auto operator<=>(const PooledString &) const = default;
    bool operator==(const PooledString &) const = default;

private:

    int32_t index_;
};

template<typename Tag>
class StringPool
{
    friend class PooledString<Tag>;

    static StringPool &GetInstance()
    {
        static StringPool ret;
        return ret;
    }

    uint32_t GetIndex(std::string_view str)
    {
        std::lock_guard lock(mutex_);
        if(auto it = stringToIndex_.find(str); it != stringToIndex_.end())
        {
            return it->second;
        }
        const uint32_t newIndex = static_cast<int32_t>(stringToIndex_.size());
        stringToIndex_.insert({ std::string(str), newIndex });
        indexToString_.push_back(std::string(str));
        return newIndex;
    }

    const std::string &GetString(const PooledString<Tag> &str)
    {
        return indexToString_[str.GetIndex()];
    }

    std::map<std::string, int32_t, std::less<>> stringToIndex_;
    std::vector<std::string> indexToString_;
    std::mutex mutex_;
};

template <typename Tag>
PooledString<Tag>::PooledString()
    : index_(-1)
{
    
}

template <typename Tag>
PooledString<Tag>::PooledString(std::string_view str)
    : index_(StringPool<Tag>::GetInstance().GetIndex(str))
{
    
}

template <typename Tag>
PooledString<Tag>::PooledString(const char *c)
    : PooledString(std::string_view(c))
{
    
}

template <typename Tag>
PooledString<Tag>::operator bool() const
{
    return index_ >= 0;
}

template <typename Tag>
uint32_t PooledString<Tag>::GetIndex() const
{
    return index_;
}

template <typename Tag>
const std::string &PooledString<Tag>::GetString() const
{
    assert(index_ >= 0);
    return StringPool<Tag>::GetInstance().GetString(*this);
}

RTRC_END

namespace std
{

    template<typename Tag>
    struct hash<Rtrc::PooledString<Tag>>
    {
        size_t operator()(const Rtrc::PooledString<Tag> &s) const noexcept
        {
            return std::hash<uint32_t>{}(s.GetIndex());
        }
    };

} // namespace std
