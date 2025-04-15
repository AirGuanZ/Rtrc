#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

namespace SDF
{

    template<typename T>
    T Sign(T v)
    {
        return v > 0 ? 1 : (v < 0 ? -1 : 0);
    }

    // See https://iquilezles.org/articles/distfunctions/
    template<typename T>
    T UDTriangle(const Vector3<T> &p, const Vector3<T> &a, const Vector3<T> &b, const Vector3<T> &c)
    {
        const Vector3<T> ba = b - a; const Vector3<T> pa = p - a;
        const Vector3<T> cb = c - b; const Vector3<T> pb = p - b;
        const Vector3<T> ac = a - c; const Vector3<T> pc = p - c;
        const Vector3<T> nor = Rtrc::Cross(ba, ac);

        if(SDF::Sign(Rtrc::Dot(Rtrc::Cross(ba, nor), pa)) +
           SDF::Sign(Rtrc::Dot(Rtrc::Cross(cb, nor), pb)) +
           SDF::Sign(Rtrc::Dot(Rtrc::Cross(ac, nor), pc)) < T(2))
        {
            return std::sqrt((std::min)((std::min)(
                Rtrc::LengthSquare(ba * Rtrc::Clamp(Rtrc::Dot(ba, pa) / Rtrc::LengthSquare(ba), 0.0, 1.0) - pa),
                Rtrc::LengthSquare(cb * Rtrc::Clamp(Rtrc::Dot(cb, pb) / Rtrc::LengthSquare(cb), 0.0, 1.0) - pb)),
                Rtrc::LengthSquare(ac * Rtrc::Clamp(Rtrc::Dot(ac, pc) / Rtrc::LengthSquare(ac), 0.0, 1.0) - pc)));
        }

        return std::sqrt(Rtrc::Dot(nor, pa) * Rtrc::Dot(nor, pa) / Rtrc::LengthSquare(nor));
    }

    template<typename T>
    T UDLine(const Vector3<T> &p, const Vector3<T> &a, const Vector3<T> &b)
    {
        const Vector3<T> pa = p - a, ba = b - a;
        const T h = Rtrc::Clamp(Dot(pa, ba) / Dot(ba, ba), 0.0, 1.0);
        return Length(pa - ba * h);
    }

    // c.x: sin(angle), c.y: cos(angle), h: height
    // corner is put at the origin and height line points to -y
    template<typename T>
    T SDCone(const Vector3<T> &p, const Vector2<T> &c, double h)
    {
        const Vector2<T> q = h * Vector2<T>(c.x / c.y, -1.0);
        const Vector2<T> w = Vector2<T>(Rtrc::Length(Vector2<T>(p.x, p.z)), p.y);
        const Vector2<T> a = w - q * Rtrc::Clamp<T>(Rtrc::Dot(w, q) / Rtrc::Dot(q, q), 0, 1);
        const Vector2<T> b = w - q * Vector2<T>(Rtrc::Clamp<T>(w.x / q.x, 0, 1), 1);
        const T k = SDF::Sign(q.y);
        const T d = (std::min)(Rtrc::Dot(a, a), Rtrc::Dot(b, b));
        const T s = (std::max)(k * (w.x * q.y - w.y * q.x), k * (w.y - q.y));
        return std::sqrt(d) * SDF::Sign(s);
    }

    // Similar to the previous one except height is infinite
    template<typename T>
    T SDCone(const Vector3<T> &p, const Vector2<T> &c)
    {
        const Vector2<T> q = Vector2<T>(Rtrc::Length(Vector2<T>(p.x, p.z)), -p.y);
        const T  d = Rtrc::Length(q - c * (std::max<T>)(Rtrc::Dot(q, c), 0));
        return d * ((q.x * c.y - q.y * c.x < T(0)) ? T(-1.0) : T(1.0));
    }

    // c is the sin/cos of the angle
    template<typename T>
    T SDSolidAngle(const Vector3<T> p, const Vector2<T> &c, T radius)
    {
        const Vector2<T> q = Vector2<T>(Rtrc::Length(Vector2<T>(p.x, p.z)), p.y);
        const T l = Rtrc::Length(q) - radius;
        const T m = Rtrc::Length(q - c * Rtrc::Clamp<T>(Rtrc::Dot(q, c), 0, radius));
        return (std::max)(l, m * SDF::Sign(c.y * q.x - c.x * q.y));
    }

} // namespace SDF

RTRC_GEO_END
