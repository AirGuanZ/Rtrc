#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<std::integral T>
T NextPowerOfTwo(T x)
{
    T r = 1;
    while(r < x)
    {
        r <<= 1;
    }
    return r;
}

template<>
inline uint32_t NextPowerOfTwo<uint32_t>(uint32_t x)
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

RTRC_END
