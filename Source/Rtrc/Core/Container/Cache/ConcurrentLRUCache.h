#pragma once

#include <Rtrc/Core/Container/Cache/hhvm_lru_cache/concurrent-scalable-cache.h>
#include <Rtrc/Core/Hash.h>

RTRC_BEGIN

namespace ConcurrentLRUCacheDetail
{

    template<typename HashOpr, typename EqOpr>
    struct HashCompareImpl
    {
        template<typename Key>
        size_t hash(const Key &key) const
        {
            return HashOpr{}(key);
        }

        template<typename Key>
        bool equal(const Key &lhs, const Key &rhs) const
        {
            return EqOpr{}(lhs, rhs);
        }
    };

} // namespace ConcurrentLRUCacheDetail

template<typename Key, typename Value, typename HashOpr = HashOperator<>, typename EqOpr = std::equal_to<>>
class ConcurrentLRUCache
{
public:

    explicit ConcurrentLRUCache(size_t cacheSize);

    // Returns true if found
    template<typename AccessFunc>
    bool Find(const Key &key, const AccessFunc &func);

    // Returns true if found
    bool Insert(const Key &key, const Value &value);

    // Returns true if inserted
    template<typename AccessFunc, typename CreateFunc>
    bool FindOrInsert(const Key &key, const AccessFunc &accessFunc, const CreateFunc &createFunc);

    // Not thread-safe
    void Clear();

private:

    using Impl = HPHP::ConcurrentLRUCache<Key, Value, ConcurrentLRUCacheDetail::HashCompareImpl<HashOpr, EqOpr>>;

    Impl impl_;
};

template<typename Key, typename Value, typename HashOpr = HashOperator<>, typename EqOpr = std::equal_to<>>
class ScalableConcurrentLRUCache
{
public:

    // Set numShards <- 0 to use the number of hardware concurrency
    explicit ScalableConcurrentLRUCache(size_t cacheSize, size_t numShards = 0);

    // Returns true if found
    template<typename AccessFunc>
    bool Find(const Key &key, const AccessFunc &func);

    // Returns true if found
    bool Insert(const Key &key, const Value &value);

    // Returns true if inserted
    template<typename AccessFunc, typename CreateFunc>
    bool FindOrInsert(const Key &key, const AccessFunc &accessFunc, const CreateFunc &createFunc);

    // Not thread-safe
    void Clear();

private:

    using Impl = HPHP::ConcurrentScalableCache<Key, Value, ConcurrentLRUCacheDetail::HashCompareImpl<HashOpr, EqOpr>>;

    Impl impl_;
};

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
ConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::ConcurrentLRUCache(size_t cacheSize)
    : impl_(cacheSize)
{
    
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
template <typename AccessFunc>
bool ConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Find(const Key& key, const AccessFunc& func)
{
    typename Impl::ConstAccessor accessor;
    if(impl_.find(accessor, key))
    {
        func(*accessor);
        return true;
    }
    return false;
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
bool ConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Insert(const Key& key, const Value& value)
{
    return impl_.insert(key, value);
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
template <typename AccessFunc, typename CreateFunc>
bool ConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::FindOrInsert(
    const Key& key, const AccessFunc& accessFunc, const CreateFunc& createFunc)
{
    return impl_.find_or_insert(key, accessFunc, createFunc);
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
void ConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Clear()
{
    impl_.clear();
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
ScalableConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::ScalableConcurrentLRUCache(size_t cacheSize, size_t numShards)
    : impl_(cacheSize, numShards)
{
    
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
template <typename AccessFunc>
bool ScalableConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Find(const Key& key, const AccessFunc& func)
{
    typename Impl::ConstAccessor accessor;
    if(impl_.find(accessor, key))
    {
        func(*accessor);
        return true;
    }
    return false;
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
bool ScalableConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Insert(const Key& key, const Value& value)
{
    return impl_.insert(key, value);
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
template <typename AccessFunc, typename CreateFunc>
bool ScalableConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::FindOrInsert(
    const Key& key, const AccessFunc& accessFunc, const CreateFunc& createFunc)
{
    return impl_.find_or_insert(key, accessFunc, createFunc);
}

template <typename Key, typename Value, typename HashOpr, typename EqOpr>
void ScalableConcurrentLRUCache<Key, Value, HashOpr, EqOpr>::Clear()
{
    impl_.clear();
}

RTRC_END
