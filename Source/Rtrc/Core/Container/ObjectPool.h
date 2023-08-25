#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Swap.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

namespace ObjectPoolDetail
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

} // namespace ObjectPoolDetail

template<typename Key, typename Value, bool ThreadSafe, bool HashMap>
class ObjectPool : public Uncopyable, protected ObjectPoolDetail::Mutex<ThreadSafe>
{
    class RemoveHandle;

    struct Record
    {
        RemoveHandle *removeHandle = nullptr;
        Box<Value> value;
    };

    using Map = typename ObjectPoolDetail::Map<HashMap, Key, Record>::Type;

    Map map_;

public:

    class RemoveHandle : public Uncopyable
    {
        friend class ObjectPool;
        ObjectPool *pool_ = nullptr;
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
