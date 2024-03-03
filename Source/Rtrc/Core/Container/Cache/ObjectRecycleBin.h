#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Swap.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

namespace ObjectRecycleBinDetail
{

    template<bool ThreadSafe>
    struct Mutex { };

    template<>
    struct Mutex<true> { std::mutex mutex_; };

    template<bool HashMap, typename Key, typename Value>
    struct Map
    {
        using Type = std::multimap<Key, Value, std::less<>>;
    };

    template<typename Key, typename Value>
    struct Map<true, Key, Value>
    {
        using Type = std::unordered_multimap<Key, Value, HashOperator<>>;
    };

} // namespace ObjectRecycleBinDetail

/**
    ObjectRecycleBin: A thread-safe pool for managing object lifetimes with explicit user control over object destruction.

    Purpose:
    - Provides temporary storage for objects that are not currently needed, but may be reused in the future.
    - Associates stored objects with keys, allowing a one-to-many relationship between keys and objects.

    Behaviors:
    - `Insert`: Adds an object to the pool and associates it with a specified key. Returns a `RemoveHandle`.
        - The `RemoveHandle` allows the user to explicitly destroy the in-pool object before its natural eviction.
    - `Get`: Retrieves and removes an object from the pool that matches the given key, if available.
        - The corresponding `RemoveHandle` for the retrieved object is invalidated upon calling `Get`.

    Usage Example:
    ```
    ObjectRecycleBin pool;
    auto objectA = ...; // Create your object
    auto removeHandleA = pool.Insert(key1, std::move(objectA));
    // `removeHandleA` can be used to explicitly remove `objectA` from the pool.

    auto retrievedObject = pool.Get(key1);
    // `retrievedObject` now holds the original `objectA`.
    // The `removeHandleA` is now invalidated and has no effect.
    ```

    Note:
    - The destruction of `removeHandleA` will destruct `objectA` only if `objectA` is still in the pool.
    - If `objectA` has been retrieved with `Get`, the handle's destructor has no effect on the now-external `objectA`.
 */
template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class ObjectRecycleBin : public Uncopyable, protected ObjectRecycleBinDetail::Mutex<ThreadSafe>
{
    class RemoveHandle;

    struct Record
    {
        RemoveHandle *removeHandle = nullptr;
        Box<Value> value;
    };

    using Map = typename ObjectRecycleBinDetail::Map<HashMap, Key, Record>::Type;

    Map map_;

public:

    class RemoveHandle : public Uncopyable
    {
        friend class ObjectRecycleBin;
        ObjectRecycleBin *pool_ = nullptr;
        std::optional<typename Map::iterator> iterator_;

    public:

        RemoveHandle() = default;

        RemoveHandle(RemoveHandle &&other) noexcept
        {
            this->Swap(other);
        }

        RemoveHandle &operator=(RemoveHandle &&other) noexcept
        {
            this->Swap(other);
            return *this;
        }

        void Swap(RemoveHandle &other) noexcept
        {
            RTRC_SWAP_MEMBERS(*this, other, pool_, iterator_);
        }

        ~RemoveHandle()
        {
            if(pool_)
            {
                pool_->_internalRemove(iterator_);
            }
        }
    };

    /** Returns nullptr if not found */
    Box<Value> Get(const Key &key)
    {
        if constexpr(ThreadSafe)
        {
            std::lock_guard lock(this->mutex_);
            if(auto it = map_.find(key); it != map_.end())
            {
                auto &record = it->second;
                record.removeHandle->iterator_ = std::nullopt;
                auto ret = std::move(record.value);
                map_.erase(it);
                return ret;
            }
        }
        else
        {
            if(auto it = map_.find(key); it != map_.end())
            {
                auto &record = it->second;
                record.removeHandle->iterator_ = std::nullopt;
                auto ret = std::move(record.value);
                map_.erase(it);
                return ret;
            }
        }
        return nullptr;
    }

    Box<RemoveHandle> Insert(const Key &key, Box<Value> value)
    {
        auto handle = MakeBox<RemoveHandle>();
        handle->pool_ = this;
        Record record;
        record.removeHandle = handle.get();
        record.value = std::move(value);
        if constexpr(ThreadSafe)
        {
            std::lock_guard lock(this->mutex_);
            handle->iterator_ = map_.insert({ key, std::move(record) });
        }
        else
        {
            handle->iterator_ = map_.insert({ key, std::move(record) });
        }
        return handle;
    }

    void _internalRemove(const std::optional<typename Map::iterator> &iterator)
    {
        if constexpr(ThreadSafe)
        {
            std::lock_guard lock(this->mutex_);
            if(iterator)
            {
                map_.erase(*iterator);
            }
        }
        else
        {
            if(iterator)
            {
                map_.erase(*iterator);
            }
        }
    }
};

RTRC_END
