#pragma once

#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Geometry/Exact/Expansion.h>

RTRC_GEO_BEGIN

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

RTRC_GEO_END

template <>
struct std::formatter<Rtrc::Geo::Expansion> : std::formatter<std::string>
{
    auto format(const Rtrc::Geo::Expansion &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(p.ToString(), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Geo::Expansion2> : std::formatter<std::string>
{
    auto format(const Rtrc::Geo::Expansion2 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {})", p.x.ToString(), p.y.ToString()), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Geo::Expansion3> : std::formatter<std::string>
{
    auto format(const Rtrc::Geo::Expansion3 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format(
            "({}, {}, {})", p.x.ToString(), p.y.ToString(), p.z.ToString()), ctx);
    }
};

template <>
struct std::formatter<Rtrc::Geo::Expansion4> : std::formatter<std::string>
{
    auto format(const Rtrc::Geo::Expansion4 &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format(
            "({}, {}, {}, {})", p.x.ToString(), p.y.ToString(), p.z.ToString(), p.w.ToString()), ctx);
    }
};