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
    explicit PooledString(std::string_view str);

    operator bool() const;
    uint32_t GetIndex() const;

    auto operator<=>(const PooledString &) const = default;
    bool operator==(const PooledString &) const = default;

private:

    uint32_t index_;
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
        const uint32_t newIndex = static_cast<uint32_t>(stringToIndex_.size()) + 1;
        stringToIndex_.insert({ std::string(str), newIndex });
        return newIndex;
    }

    std::map<std::string, uint32_t, std::less<>> stringToIndex_;
    std::mutex mutex_;
};

template <typename Tag>
PooledString<Tag>::PooledString()
    : index_(0)
{
    
}

template <typename Tag>
PooledString<Tag>::PooledString(std::string_view str)
    : index_(StringPool<Tag>::GetInstance().GetIndex(str))
{
    
}

template <typename Tag>
PooledString<Tag>::operator bool() const
{
    return index_ > 0;
}

template <typename Tag>
uint32_t PooledString<Tag>::GetIndex() const
{
    return index_;
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
