#pragma once

#include <cassert>
#include <map>
#include <unordered_map>
#include <shared_mutex>

#include <Rtrc/Utility/Hash.h>
#include <Rtrc/Utility/Uncopyable.h>

RTRC_BEGIN

class InObjectCache;

namespace ObjectCacheDetail
{

    template<bool ThreadSafe>
    struct ObjectCacheMutex { };

    template<>
    struct ObjectCacheMutex<true> { std::shared_mutex mutex_; };

    template<bool ThreadSafe>
    struct ObjectCacheRecordCounter { };

    template<>
    struct ObjectCacheRecordCounter<true> { int iteratorRefCount = 0; };

    template<bool UseHashMap, typename Key, typename Value>
    struct ObjectCacheMap
    {
        using Type = std::map<Key, Value>;
    };

    template<typename Key, typename Value>
    struct ObjectCacheMap<true, Key, Value>
    {
        using Type = std::unordered_map<Key, Value, HashOperator<Key>>;
    };

    class ObjectCacheInterface : public Uncopyable
    {
    public:

        virtual ~ObjectCacheInterface() = default;
        virtual void _internalRelease(InObjectCache &object) = 0;
    };

} // namespace ObjectCacheDetail

template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class ObjectCache;

class InObjectCache : public Uncopyable
{
    template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
    friend class ObjectCache;

    ObjectCacheDetail::ObjectCacheInterface *cache_;
    void *iterator_;

public:

    ~InObjectCache()
    {
        cache_->_internalRelease(*this);
    }
};

template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class ObjectCache :
    protected ObjectCacheDetail::ObjectCacheMutex<ThreadSafe>,
    public ObjectCacheDetail::ObjectCacheInterface
{
    static_assert(std::is_base_of_v<InObjectCache, Value>);

    struct Record;

    using Map = typename ObjectCacheDetail::ObjectCacheMap<HashMap, Key, Record>::Type;

    struct Record : ObjectCacheDetail::ObjectCacheRecordCounter<ThreadSafe>
    {
        std::weak_ptr<Value> valueWeakRef;
        typename Map::iterator selfIterator;
    };

    Map map_;

    void _internalRelease(InObjectCache &object) override
    {
        auto iter = *static_cast<typename Map::iterator *>(object.iterator_);
        if constexpr(ThreadSafe)
        {
            std::unique_lock lock(this->mutex_);
            if(!--iter->second.iteratorRefCount)
            {
                map_.erase(iter);
            }
        }
        else
        {
            map_.erase(iter);
        }
    }

    template<typename CreateFunc>
    static RC<Value> CreateAsRC(CreateFunc &&createFunc)
    {
        auto ret = std::invoke(std::forward<CreateFunc>(createFunc));
        if constexpr(std::is_same_v<RC<Value>, std::remove_cvref_t<decltype(ret)>>)
        {
            return ret;
        }
        else
        {
            return ToRC(std::move(ret));
        }
    }

public:

    template<typename CreateFunc>
    RC<Value> GetOrCreate(const Key &key, CreateFunc &&createFunc)
    {
        if constexpr(ThreadSafe)
        {
            {
                std::shared_lock lock(this->mutex_);
                if(auto it = map_.find(key); it != map_.end())
                {
                    if(auto ret = it->second.valueWeakRef.lock())
                    {
                        return ret;
                    }
                }
            }

            std::unique_lock lock(this->mutex_);
            if(auto it = map_.find(key); it != map_.end())
            {
                if(auto ret = it->second.valueWeakRef.lock())
                {
                    return ret;
                }
                // The cached object has 0 reference counter value, but its destructor hasn't been executed.
                // In this case we create a new object, and increase the iteratorRefCount by 1
                auto ret = CreateAsRC(std::forward<CreateFunc>(createFunc));
                InObjectCache &base = *ret;
                base.cache_ = this;
                base.iterator_ = &it->second.selfIterator;
                ++it->second.iteratorRefCount;
                it->second.valueWeakRef = ret;
                return ret;
            }
            auto ret = CreateAsRC(std::forward<CreateFunc>(createFunc));
            auto it = map_.insert({ key, Record{} }).first;
            it->second.selfIterator = it;
            InObjectCache &base = *ret;
            base.cache_ = this;
            base.iterator_ = &it->second.selfIterator;
            it->second.iteratorRefCount = 1;
            it->second.valueWeakRef = ret;
            return ret;
        }
        else
        {
            if(auto it = map_.find(key); it != map_.end())
            {
                auto ret = it->second.valueWeakRef.lock();
                assert(ret);
                return ret;
            }
            auto ret = CreateAsRC(std::forward<CreateFunc>(createFunc));
            auto it = map_.insert({ key, Record{} }).first;
            it->second.selfIterator = it;
            InObjectCache &base = *ret;
            base.cache_ = this;
            base.iterator_ = &it->second.selfIterator;
            it->second.valueWeakRef = ret;
            return ret;
        }
    }
};

RTRC_END
