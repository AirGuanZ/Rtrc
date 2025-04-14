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

} // namespace SDF

RTRC_GEO_END
