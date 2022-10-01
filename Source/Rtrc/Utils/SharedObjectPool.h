#pragma once

#include <map>
#include <shared_mutex>

#include <Rtrc/Common.h>

RTRC_BEGIN

// TODO IMPROVE: thread-local object cache

namespace SharedObjectPoolDetail
{

    template<bool ThreadSafe>
    class SharedObjectPoolMutex
    {
        
    };

    template<>
    class SharedObjectPoolMutex<true>
    {
    protected:
        std::shared_mutex mutex_;
    };

} // namespace SharedObjectPoolDetail

template<typename Key, typename Value, bool ThreadSafe>
class SharedObjectPool : public SharedObjectPoolDetail::SharedObjectPoolMutex<ThreadSafe>
{
public:

    // returns nullptr if not found
    template<typename TKey>
    std::shared_ptr<Value> Get(const TKey &key) const
    {
        if constexpr(ThreadSafe)
        {
            std::shared_lock lock(this->mutex_);
            auto it = keyToValue_.find(key);
            return it != keyToValue_.end() ? it->second.lock() : nullptr;
        }
        else
        {
            auto it = keyToValue_.find(key);
            return it != keyToValue_.end() ? it->second.lock() : nullptr;
        }
    }

    template<typename TKey, typename CreateFunc>
    std::shared_ptr<Value> GetOrCreate(const TKey &key, CreateFunc &&createFunc)
    {
        if constexpr(ThreadSafe)
        {
            {
                std::shared_lock lock(this->mutex_);
                if(auto it = keyToValue_.find(key); it != keyToValue_.end())
                {
                    if(auto rc = it->second.lock())
                    {
                        return rc;
                    }
                }
            }
            std::unique_lock lock(this->mutex_);
            auto newObject = OptionalRCWrapper(std::forward<CreateFunc>(createFunc));
            if(newObject)
            {
                keyToValue_.insert(std::make_pair(key, newObject));
            }
            return newObject;
        }
        else
        {
            if(auto it = keyToValue_.find(key); it != keyToValue_.end())
            {
                if(auto rc = it->second.lock())
                {
                    return rc;
                }
            }
            auto newObject = OptionalRCWrapper(std::forward<CreateFunc>(createFunc));
            if(newObject)
            {
                keyToValue_.insert(std::make_pair(key, newObject));
            }
            return newObject;
        }
    }

    void GC()
    {
        if constexpr(ThreadSafe)
        {
            std::unique_lock lock(this->mutex_);
            for(auto it = keyToValue_.begin(); it != keyToValue_.end();)
            {
                if(it->second.expired())
                {
                    it = keyToValue_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
        else
        {
            for(auto it = keyToValue_.begin(); it != keyToValue_.end();)
            {
                if(it->second.expired())
                {
                    it = keyToValue_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    }

private:

    using KeyToValue = std::map<Key, std::weak_ptr<Value>, std::less<>>;

    template<typename Func>
    auto OptionalRCWrapper(Func &&func)
    {
        using RT = decltype(std::invoke(std::forward<Func>(func)));
        if constexpr(std::is_same_v<std::remove_reference_t<RT>, Value>)
        {
            return MakeRC<Value>(std::invoke(std::forward<Func>(func)));
        }
        else
        {
            return std::invoke(std::forward<Func>(func));
        }
    }

    KeyToValue keyToValue_;
};

RTRC_END
