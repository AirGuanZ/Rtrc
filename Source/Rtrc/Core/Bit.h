#pragma once

#include <bit>
#include <cassert>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<std::integral T>
constexpr T NextPowerOfTwo(T x)
{
    T r = 1;
    while(r < x)
    {
        r <<= 1;
    }
    return r;
}

template<typename T>
constexpr uint32_t NextPowerOfTwo_Power(T x)
{
    uint32_t i = 0;
    while((T(1) << i) < x)
    {
        ++i;
    }
    return i;
}

template<typename T>
constexpr uint32_t Log2PowerOfTwo(T x)
{
    const uint32_t result = std::countr_zero(x);
    return result;
}

template<>
constexpr uint32_t NextPowerOfTwo<uint32_t>(uint32_t x)
{
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x++;
    return x;
}

template<std::unsigned_integral T>
constexpr bool IsPowerOfTwo(T x)
{
    return (x != 0) && ((x & (x - 1)) == 0);
}

RTRC_END
