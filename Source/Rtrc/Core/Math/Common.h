#pragma once

#include <Rtrc/Core/Common.h>

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

template<std::floating_point T>
T Saturate(T v)
{
    return std::clamp(v, T(0), T(1));
}

template<typename V, std::floating_point T>
T Lerp(const V &lhs, const V &rhs, const T &t)
{
    return lhs * (1 - t) + rhs * t;
}

template<std::floating_point T>
T InverseLerp(const T &lhs, const T &rhs, const T &val)
{
    if(lhs <= rhs)
    {
        return T(0);
    }
    return (val - lhs) / (rhs - lhs);
}

RTRC_END
