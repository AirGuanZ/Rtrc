#pragma once

#include <cassert>
#include <map>
#include <unordered_map>
#include <shared_mutex>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

class InSharedObjectCache;

template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class SharedObjectCache;

namespace SharedObjectCacheDetail
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
        using Type = std::map<Key, Value, std::less<>>;
    };

    template<typename Key, typename Value>
    struct ObjectCacheMap<true, Key, Value>
    {
        using Type = std::unordered_map<Key, Value, HashOperator<>>;
    };

    class ObjectCacheInterface : public Uncopyable
    {
    public:

        virtual ~ObjectCacheInterface() = default;
        virtual void _internalRelease(InSharedObjectCache &object) = 0;
    };

} // namespace SharedObjectCacheDetail

class InSharedObjectCache : public Uncopyable
{
    template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
    friend class SharedObjectCache;

    SharedObjectCacheDetail::ObjectCacheInterface *cache_ = nullptr;
    void *iterator_ = nullptr;

public:

    virtual ~InSharedObjectCache()
    {
        if(cache_)
        {
            cache_->_internalRelease(*this);
        }
    }
};

/*
    SharedObjectCache: A thread-safe cache for managing and reusing instances of objects.

    Behaviors:
    - Object types must inherit from InSharedObjectCache.
    - Instances are accessed via SharedObjectCache::GetOrCreate, which provides a shared_ptr to the requested object.
    - Each key is mapped to a single instance of an object. This mapping persists as long as at least one shared_ptr reference to the object exists.
    - When all references to an object are released, the object is automatically destroyed and its key is removed from the cache.
    - It's guaranteed that each key will map to exactly one 'alive' instance at any given time within this cache.

    Usage Example:
    - Define an object class that inherits from InSharedObjectCache:
        class Object : public InSharedObjectCache { ... };

    - Create an SharedObjectCache instance specifying the key and object types:
        SharedObjectCache<Key, Object, ...> cache;

    - Use GetOrCreate with a key to obtain or create an object:
        Key key1{...}, key2{ ... };
        auto sharedObjectA = cache.GetOrCreate(key1, []{ ... }); // Create or get object for key1
        auto sharedObjectB = cache.GetOrCreate(key1, []{ ... }); // Fetch the existing object for key1
        auto sharedObjectC = cache.GetOrCreate(key2, []{ ... }); // Create or get object for key2

    - Validate that sharedObjectA and sharedObjectB are the same instance:
        assert(sharedObjectA == sharedObjectB);

    - Release the shared_ptr references to the object associated with key1:
        sharedObjectA.reset();
        sharedObjectB.reset(); // The object for key1 is now destroyed and its entry evicted from the cache.
*/
template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class SharedObjectCache :
    protected SharedObjectCacheDetail::ObjectCacheMutex<ThreadSafe>,
    public SharedObjectCacheDetail::ObjectCacheInterface
{
    static_assert(std::is_base_of_v<InSharedObjectCache, Value>);

    struct Record;

    using Map = typename SharedObjectCacheDetail::ObjectCacheMap<HashMap, Key, Record>::Type;

    struct Record : SharedObjectCacheDetail::ObjectCacheRecordCounter<ThreadSafe>
    {
        std::weak_ptr<Value> valueWeakRef;
        typename Map::iterator selfIterator;
    };

    Map map_;

    void _internalRelease(InSharedObjectCache &object) override
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

    template<typename TKey, typename CreateFunc>
    RC<Value> GetOrCreate(const TKey &key, CreateFunc &&createFunc)
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
                InSharedObjectCache &base = *ret;
                base.cache_ = this;
                base.iterator_ = &it->second.selfIterator;
                ++it->second.iteratorRefCount;
                it->second.valueWeakRef = ret;
                return ret;
            }
            auto ret = CreateAsRC(std::forward<CreateFunc>(createFunc));
            auto it = map_.insert({ Key(key), Record{} }).first;
            it->second.selfIterator = it;
            InSharedObjectCache &base = *ret;
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
            auto it = map_.insert({ Key(key), Record{} }).first;
            it->second.selfIterator = it;
            InSharedObjectCache &base = *ret;
            base.cache_ = this;
            base.iterator_ = &it->second.selfIterator;
            it->second.valueWeakRef = ret;
            return ret;
        }
    }
};

RTRC_END
