#pragma once

#include <numbers>

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

namespace DistributionTransform
{

    template<typename T>
    Vector2<T> UniformDisk(const Vector2<T> &u)
    {
        const T phi = 2 * std::numbers::pi_v<T> * u.x;
        const T r = std::sqrt(u.y);
        return { r * std::cos(phi), r * std::sin(phi) };
    }

    template<typename T>
    Vector2<T> UniformDiskConcentric(const Vector2<T> &u)
    {
        const Vector2<T> uoffset = T(2) * u - Vector2<T>(1);
        if(uoffset.x == 0 && uoffset.y == 0)
            return Vector2<T>(0);
        T theta, r;
        if(std::abs(uoffset.x) > std::abs(uoffset.y))
        {
            r = uoffset.x;
            theta = (std::numbers::pi_v<T> / 4) * (uoffset.y / uoffset.x);
        }
        else
        {
            r = uoffset.y;
            theta = (std::numbers::pi_v<T> / 4) * (uoffset.x / uoffset.y);
        }
        return r * Vector2<T>(std::cos(theta), std::sin(theta));
    }

    template<typename T>
    Vector3<T> UniformSphere(const Vector2<T> &u)
    {
        const T z = 1 - 2 * u.x;
        const T phi = 2 * std::numbers::pi_v<T> * u.y;
        const T r = std::sqrt((std::max<T>)(T(0), 1 - z * z));
        const T x = r * std::cos(phi);
        const T y = r * std::sin(phi);
        return { x, y, z };
    }

} // namespace DistributionTransform

RTRC_END
