#pragma once

#include <Rtrc/Core/Math/Vector.h>
#include <Rtrc/Core/Math/Exact/Expansion.h>

RTRC_BEGIN

using Expansion2 = Vector2<Expansion>;
using Expansion3 = Vector3<Expansion>;
using Expansion4 = Vector4<Expansion>;

template<typename Word, int StaticStorage>
using SExpansion2 = Vector2<SExpansion<Word, StaticStorage>>;
template<typename Word, int StaticStorage>
using SExpansion3 = Vector3<SExpansion<Word, StaticStorage>>;
template<typename Word, int StaticStorage>
using SExpansion4 = Vector4<SExpansion<Word, StaticStorage>>;

inline Expansion2 ToDynamicExpansion(const Vector2d &v) { return { Expansion(v.x), Expansion(v.y) }; }
inline Expansion3 ToDynamicExpansion(const Vector3d &v) { return { Expansion(v.x), Expansion(v.y), Expansion(v.z) }; }
inline Expansion4 ToDynamicExpansion(const Vector4d &v) { return { Expansion(v.x), Expansion(v.y), Expansion(v.z), Expansion(v.w) }; }

struct CompareHomogeneousPoint
{
    bool operator()(const Expansion3 &a, const Expansion3 &b) const
    {
        if(const int c0 = CompareHomo(a.x, a.z, b.x, b.z); c0 != 0)
        {
            return c0 < 0;
        }
        return CompareHomo(a.y, a.z, b.y, b.z) < 0;
    }

    bool operator()(const Expansion4 &a, const Expansion4 &b) const
    {
        if(const int c0 = CompareHomo(a.x, a.w, b.x, b.w); c0 != 0)
        {
            return c0 < 0;
        }
        if(const int c1 = CompareHomo(a.y, a.w, b.y, b.w); c1 != 0)
        {
            return c1 < 0;
        }
        return CompareHomo(a.z, a.w, b.z, b.w) < 0;
    }

    static int CompareHomo(const Expansion &a, const Expansion &adown, const Expansion &b, const Expansion &bdown)
    {
        return adown.GetSign() * bdown.GetSign() * (a * bdown).Compare(b * adown);
    }
};

RTRC_END

template <>
struct std::formatter<Rtrc::Expansion> : std::formatter<std::string>
{
    auto format(const Rtrc::Expansion &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(p.ToString(), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Expansion2> : std::formatter<std::string>
{
    auto format(const Rtrc::Expansion2 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {})", p.x.ToString(), p.y.ToString()), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Expansion3> : std::formatter<std::string>
{
    auto format(const Rtrc::Expansion3 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format(
            "({}, {}, {})", p.x.ToString(), p.y.ToString(), p.z.ToString()), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Expansion4> : std::formatter<std::string>
{
    auto format(const Rtrc::Expansion4 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format(
            "({}, {}, {}, {})", p.x.ToString(), p.y.ToString(), p.z.ToString(), p.w.ToString()), ctx);
    }
};