#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

inline uint32_t NextPowerOfTwo(uint32_t x)
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
