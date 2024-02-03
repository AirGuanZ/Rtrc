#pragma once

#include <Rtrc/Core/Math/Vector2.h>

RTRC_BEGIN

// f(x, y, uvx, uvy) or f(x, y)
// (x, y) \in [0, width] x [0, height]
// (uvx, uvy) \in [0, 1]^2
template<typename F>
void GenerateUniformGridVertices(int width, int height, const F &f)
{
    const float rcpW = 1.0f / width;
    const float rcpH = 1.0f / height;
    for(int y = 0; y <= height; ++y)
    {
        for(int x = 0; x <= width; ++x)
        {
            if constexpr(requires{ f(x, y, x *rcpW, y *rcpH); })
            {
                f(x, y, x * rcpW, y * rcpH);
            }
            else
            {
                f(x, y);
            }
        }
    }
}

// f(a, b, c)
// a, b, c \in [0, (width+1)*(height+1)]
template<typename F>
void GenerateUniformGridTriangleIndices(int width, int height, const F &f)
{
    auto ToIndex = [&](int x, int y)  { return y * (width + 1) + x; };
    for(int y = 0; y < height; ++y)
    {
        for(int x = 0; x < width; ++x)
        {
            const int i0 = ToIndex(x + 0, y + 0);
            const int i1 = ToIndex(x + 0, y + 1);
            const int i2 = ToIndex(x + 1, y + 1);
            const int i3 = ToIndex(x + 1, y + 0);
            f(i0, i1, i2);
            f(i0, i2, i3);
        }
    }
}

inline std::vector<Vector2i> GenerateUniformGridVertices(int width, int height)
{
    std::vector<Vector2i> ret;
    GenerateUniformGridVertices(width, height, [&](int x, int y) { ret.emplace_back(x, y); });
    return ret;
}

template<typename T = uint32_t>
std::vector<T> GenerateUniformGridTriangleIndices(int width, int height)
{
    std::vector<T> ret;
    Rtrc::GenerateUniformGridTriangleIndices(width, height, [&](int a, int b, int c)
    {
        ret.push_back(a);
        ret.push_back(b);
        ret.push_back(c);
    });
    return ret;
}

RTRC_END
