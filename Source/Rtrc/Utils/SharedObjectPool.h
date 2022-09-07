#pragma once

#include <map>
#include <mutex>

#include <Rtrc/Common.h>

RTRC_BEGIN

// TODO IMPROVE: thread-local object cache

template<bool ThreadSafe>
class SharedObjectPoolMutex
{
    
};

template<>
class SharedObjectPoolMutex<true>
{
protected:

    std::mutex mutex_;
};

template<typename Key, typename Value, bool ThreadSafe>
class SharedObjectPool : public SharedObjectPoolMutex<ThreadSafe>
{
public:

    // returns nullptr if not found
    template<typename TKey>
    std::shared_ptr<Value> Get(const TKey &key) const
    {
        return DoWithThreadSafeGuard([&]
        {
            auto it = keyToValue_.find(key);
            return it != keyToValue_.end() ? it->second.lock() : nullptr;
        });
    }

    template<typename TKey, typename CreateFunc>
    std::shared_ptr<Value> GetOrCreate(const TKey &key, CreateFunc &&createFunc)
    {
        return DoWithThreadSafeGuard([&]
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
        });
    }

    void GC()
    {
        DoWithThreadSafeGuard([&]
        {
            std::vector<typename KeyToValue::iterator> invalidIters;
            for(auto it = keyToValue_.begin(); it != keyToValue_.end(); ++it)
            {
                if(it->second.expired())
                {
                    invalidIters.push_back(it);
                }
            }
            for(auto &iter : invalidIters)
            {
                keyToValue_.erase(iter);
            }
        });
    }

private:

    using KeyToValue = std::map<Key, std::weak_ptr<Value>, std::less<>>;

    template<typename Func>
    auto DoWithThreadSafeGuard(Func &&func)
    {
        if constexpr(ThreadSafe)
        {
            std::lock_guard lock(this->mutex_);
            return std::invoke(std::forward<Func>(func));
        }
        else
        {
            return std::invoke(std::forward<Func>(func));
        }
    }

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
