#pragma once

#include <numbers>

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

namespace DistributionTransform
{

    template<typename T>
    Vector2<T> UniformOnDisk(const Vector2<T> &u)
    {
        const T phi = 2 * std::numbers::pi_v<T> * u.x;
        const T r = std::sqrt(u.y);
        return { r * std::cos(phi), r * std::sin(phi) };
    }

    template<typename T>
    Vector2<T> UniformOnDiskConcentric(const Vector2<T> &u)
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
    Vector3<T> UniformOnSphere(const Vector2<T> &u)
    {
        const T z = 1 - 2 * u.x;
        const T phi = 2 * std::numbers::pi_v<T> * u.y;
        const T r = std::sqrt((std::max<T>)(T(0), 1 - z * z));
        const T x = r * std::cos(phi);
        const T y = r * std::sin(phi);
        return { x, y, z };
    }

    template<typename F>
    std::pair<Vector3<F>, F> UniformOnHemisphere(F u1, F u2) noexcept
    {
        static_assert(std::is_floating_point_v<F>);

        const F z = u1;
        const F phi = 2 * std::numbers::pi_v<F> * u2;
        const F r = std::sqrt((std::max)(F(0), 1 - z * z));
        const F x = r * std::cos(phi);
        const F y = r * std::sin(phi);

        return { { x, y, z }, F(1.0) / (F(2.0) * std::numbers::pi_v<F>) };
    }

    template<typename F>
    std::pair<Vector3<F>, F> UniformOnSphericalCap(F maxCosTheta, F u1, F u2)
    {
        const F cosTheta = (1 - u1) + u1 * maxCosTheta;
        const F sinTheta = std::sqrt((std::max)(F(0), 1 - cosTheta * cosTheta));
        const F phi = 2 * std::numbers::pi_v<F> *u2;
        const F pdf = 1 / (2 * std::numbers::pi_v<F> *(1 - maxCosTheta));
        return { { std::cos(phi) * sinTheta, std::sin(phi) * sinTheta, cosTheta }, pdf };
    }

    template<typename F>
    F UniformOnSphericalCapPDF(F maxCosTheta)
    {
        return 1 / (2 * std::numbers::pi_v<F> *(1 - maxCosTheta));
    }

    template<typename F>
    std::pair<Vector3<F>, F> ZWeightedOnHemisphere(F u1, F u2)
    {
        Vector2<F> sam;
        u1 = 2 * u1 - 1;
        u2 = 2 * u2 - 1;
        if(u1 || u2)
        {
            F theta, r;
            if(std::abs(u1) > std::abs(u2))
            {
                r = u1;
                theta = F(0.25) * std::numbers::pi_v<F> *(u2 / u1);
            }
            else
            {
                r = u2;
                theta = F(0.5) * std::numbers::pi_v<F> - F(0.25) * std::numbers::pi_v<F> * (u1 / u2);
            }
            sam = r * Vector2<F>(std::cos(theta), std::sin(theta));
        }

        const F z = std::sqrt((std::max)(F(0), 1 - Rtrc::LengthSquare(sam)));
        return { { sam.x, sam.y, z }, z * (1 / std::numbers::pi_v<F>) };
    }

    template<typename F>
    F ZWeightedOnHemispherePDF(F z)
    {
        return z > 0 ? z * (1 / std::numbers::pi_v<F>) : 0;
    }

    // Returns barycentric coordinate
    template<typename F>
    Vector2<F> UniformOnTriangle(F u1, F u2) noexcept
    {
        const F t = std::sqrt(u1);
        return { 1 - t, t * u2 };
    }

} // namespace DistributionTransform

RTRC_END
