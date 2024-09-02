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

template<typename T>
T Min(T a, T b)
{
    return a < b ? a : b;
}

template<typename T>
T Max(T a, T b)
{
    return a < b ? b : a;
}

template<typename T>
T Clamp(T v, T vmin, T vmax)
{
    return Rtrc::Max(vmin, Rtrc::Min(v, vmax));
}

template<std::floating_point T>
T Saturate(T v)
{
    return Rtrc::Clamp(v, T(0), T(1));
}

template<typename V, std::floating_point T>
V Lerp(const V &lhs, const V &rhs, const T &t)
{
    return lhs * (1 - t) + rhs * t;
}

template<std::floating_point T>
T InverseLerp(const T &lhs, const T &rhs, const T &val)
{
    const T diff = rhs - lhs;
    if(diff == 0)
    {
        return T(0);
    }
    return (val - lhs) / diff;
}

template<std::floating_point T>
T ComputeTriangleAreaFromEdgeLengths(T l0, T l1, T l2)
{
    const T s = T(0.5) * (l0 + l1 + l2);
    return std::sqrt(s * (s - l0) * (s - l1) * (s - l2));
}

// cos<a, b>
template<std::floating_point T>
T ComputeCosAngleFromTriangleEdgeLengths(T a, T b, T c)
{
    return (a * a + b * b - c * c) / (2 * a * b);
}

struct MinOperator
{
    template<typename T>
    const T &operator()(const T &lhs, const T &rhs) const
    {
        return (std::min)(lhs, rhs);
    }
};

struct MaxOperator
{
    template<typename T>
    const T &operator()(const T &lhs, const T &rhs) const
    {
        return (std::max)(lhs, rhs);
    }
};

template<typename T>
auto Det2x2(const T &a, const T &b,
            const T &c, const T &d)
{
    return a * d - b * c;
}

template<typename T>
auto Det3x3(const T &a, const T &b, const T &c,
            const T &d, const T &e, const T &f,
            const T &g, const T &h, const T &i)
{
    const auto A = Det2x2(e, f, h, i);
    const auto B = Det2x2(d, f, g, i);
    const auto C = Det2x2(d, e, g, h);
    return a * A - b * B + c * C;
}

RTRC_END
