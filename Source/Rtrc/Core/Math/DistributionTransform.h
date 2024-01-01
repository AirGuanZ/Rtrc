#pragma once

#include <Rtrc/Core/Math/Angle.h>
#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

namespace DistributionTransform
{

    inline Vector2f UniformDisk(const Vector2f &u)
    {
        const float phi = 2 * PI * u.x;
        const float r = std::sqrt(u.y);
        return { r * std::cos(phi), r * std::sin(phi) };
    }

    inline Vector2f UniformDiskConcentric(const Vector2f &u)
    {
        const Vector2f uoffset = 2.0f * u - Vector2f(1);
        if(uoffset.x == 0 && uoffset.y == 0)
            return Vector2f(0);
        float theta, r;
        if(std::abs(uoffset.x) > std::abs(uoffset.y))
        {
            r = uoffset.x;
            theta = (PI / 4) * (uoffset.y / uoffset.x);
        }
        else
        {
            r = uoffset.y;
            theta = (PI / 4) * (uoffset.x / uoffset.y);
        }
        return r * Vector2f(std::cos(theta), std::sin(theta));
    }

    inline Vector3f UniformSphere(const Vector3f &u)
    {
        const float z = 1 - 2 * u.x;
        const float phi = 2 * PI * u.y;
        const float r = std::sqrt((std::max)(0.0f, 1 - z * z));
        const float x = r * std::cos(phi);
        const float y = r * std::sin(phi);
        return { x, y, z };
    }

} // namespace DistributionTransform

RTRC_END
