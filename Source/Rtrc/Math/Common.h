#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
T Square(T x)
{
    return x * x;
}

template<typename T>
int ArgMax(T v0, T v1, T v2)
{
    return v0 > v1 ? (v0 > v2 ? 0 : 2) : (v1 > v2 ? 1 : 2);
}

RTRC_END
