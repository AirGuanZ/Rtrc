#pragma once

#include <cassert>

#include <ankerl/unordered_dense.h>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Uncopyable.h>

RTRC_BEGIN

template<typename tK, typename tV, typename tHashOp = HashOperator<>>
class LRUCache : public Uncopyable
{
    using InternalList = std::list<std::pair<const tK, tV>>;
    
public:

    using KeyType = tK;
    using ValueType = tV;
    using HashOp = tHashOp;

    explicit LRUCache(size_t cacheSize)
        : LRUCache(cacheSize, cacheSize)
    {
        
    }

    // After reaching cacheSize, evict elements to get back to targetSize
    LRUCache(size_t cacheSize, size_t targetSize)
        : cacheSize_(cacheSize), targetSize_(targetSize)
    {
        assert(0 < targetSize && targetSize <= cacheSize);
    }

    size_t GetCacheSize() const
    {
        return cacheSize_;
    }

    size_t GetTargetSize() const
    {
        return targetSize_;
    }

    size_t GetSize() const
    {
        return map_.size();
    }

    bool IsEmpty() const
    {
        return map_.empty();
    }

    template<typename Self>
    auto Find(this Self &&self, const KeyType &key) -> std::conditional_t<std::is_const_v<Self>, const ValueType *, ValueType *>
    {
        if(const auto it = self.map_.find(key); it != self.map_.end())
        {
            list_.splice(list_.begin(), list_, it->second); 
            return &it->second->second;
        }
        return nullptr;
    }

    // Returns true when newly inserted; false when replaced.
    bool Insert(const KeyType &key, ValueType value)
    {
        if(const auto it = map_.find(key); it != map_.end())
        {
            it->second->second = std::move(value);
            list_.splice(list_.begin(), list_, it->second);
            return false;
        }

        list_.emplace_front(key, std::move(value));
        try
        {
            map_.insert({ key, list_.begin() });
        }
        catch(...)
        {
            list_.pop_front();
            throw;
        }

        EvictIfNeeded();
        return true;
    }

    template<typename ValueProvider>
    ValueType &FindOrInsert(const KeyType &key, ValueProvider &&valueProvider)
    {
        if(const auto it = map_.find(key); it != map_.end())
        {
            list_.splice(list_.begin(), list_, it->second);
            return it->second->second;
        }

        list_.emplace_front(key, static_cast<ValueType>(std::forward<ValueProvider>(valueProvider)));
        map_.insert({ key, list_.begin() });
        EvictIfNeeded();
        assert(!list_.empty() && list_.begin()->first == key);
        return list_.begin()->second;
    }

    // Returns true when found and removed
    bool Remove(const KeyType &key)
    {
        if(auto it = map_.find(key); it != map_.end())
        {
            list_.erase(it->second);
            map_.erase(it);
            return true;
        }
        return false;
    }

    void Clear()
    {
        map_.clear();
        list_.clear();
    }

    auto begin() { return list_.begin(); }
    auto end() { return list_.end(); }

    auto begin() const { return list_.begin(); }
    auto end() const { return list_.end(); }

private:

    void EvictIfNeeded()
    {
        if(map_.size() > cacheSize_)
        {
            while(map_.size() > targetSize_)
            {
                map_.erase(list_.back().first);
                list_.pop_back();
            }
        }
    }

    using List = InternalList;
    using Map = ankerl::unordered_dense::map<KeyType, typename List::iterator>;

    List list_;
    Map map_;
    size_t cacheSize_;
    size_t targetSize_;
};

RTRC_END
