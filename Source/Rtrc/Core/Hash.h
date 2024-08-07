#pragma once

#include <vector>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

inline size_t HashCombine(size_t a, size_t b)
{
    if constexpr(sizeof(size_t) == 4)
    {
        return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
    }
    else
    {
        return a ^ (b + 0x9e3779b97f4a7c15 + (a << 6) + (a >> 2));
    }
}

namespace HashImpl
{

    template<typename T>
    concept UseStdHash = requires(const T & v) { std::hash<T>{}(v); };

    template<typename T>
    concept UseMemberHash = !UseStdHash<T> && requires(const T & v) { v.Hash(); };

} // namespace HashImpl

template<HashImpl::UseStdHash T>
size_t Hash(const T &v)
{
    return std::hash<T>{}(v);
}

template<HashImpl::UseMemberHash T>
size_t Hash(const T &v)
{
    return v.Hash();
}

template<typename It>
size_t HashRange(It begin, It end)
{
    if(begin == end)
    {
        return 0;
    }
    size_t ret = ::Rtrc::Hash(*begin);
    while(++begin != end)
    {
        ret = ::Rtrc::HashCombine(ret, ::Rtrc::Hash(*begin));
    }
    return ret;
}

template<typename T>
size_t Hash(const std::vector<T> &v)
{
    return ::Rtrc::HashRange(v.begin(), v.end());
}

template<typename A, typename B, typename...Others>
size_t Hash(const A &a, const B &b, const Others&...others)
{
    return ::Rtrc::HashCombine(::Rtrc::Hash(a), ::Rtrc::Hash(b, others...));
}

template<typename A, typename B>
size_t Hash(const std::pair<A, B> &pair)
{
    return ::Rtrc::HashCombine(::Rtrc::Hash(pair.first), ::Rtrc::Hash(pair.second));
}

inline size_t HashMemory(const void *data, size_t size)
{
    void MurmurHash3_x64_128(const void *key, int len, uint32_t seed, void *out);
    uint64_t out[2] = { 0, 0 };
    MurmurHash3_x64_128(data, static_cast<int>(size), 42, out);
    return HashCombine(out[0], out[1]);
}

template<typename T = void>
struct HashOperator
{
    size_t operator()(const T &v) const
    {
        return ::Rtrc::Hash(v);
    }
};

template<>
struct HashOperator<void>
{
    using is_transparent = void;

    template<typename T>
    size_t operator()(const T &v) const
    {
        return ::Rtrc::Hash(v);
    }
};

template<typename T1, typename T2>
struct HashOperator<std::pair<T1, T2>>
{
    size_t operator()(const std::pair<T1, T2> &pair) const
    {
        return ::Rtrc::Hash(pair.first, pair.second);
    }
};

RTRC_END
