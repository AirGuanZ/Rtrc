#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

class Frame
{
public:

    Vector3f x, y, z;

    Frame();

    Frame(const Vector3f &x, const Vector3f &y, const Vector3f &z);

    static Frame FromX(const Vector3f &x);

    static Frame FromY(const Vector3f &y);

    static Frame FromZ(const Vector3f &z);

    Vector3f LocalToGlobal(const Vector3f &local) const;

    Vector3f GlobalToLocal(const Vector3f &global) const;

    Frame RotateToNewZ(const Vector3f &newZ) const;

    bool InPositiveZHemisphere(const Vector3f &v) const;
};

inline Frame::Frame()
    : Frame(Vector3f(1, 0, 0), Vector3f(0, 1, 0), Vector3f(0, 0, 1))
{

}

inline Frame::Frame(const Vector3f &x, const Vector3f &y, const Vector3f &z)
    : x(Normalize(x)), y(Normalize(y)), z(Normalize(z))
{

}

inline Frame Frame::FromX(const Vector3f &x)
{
    Vector3f newX = Normalize(x), newZ;
    if(1 - std::abs(newX.y) < 0.1f)
    {
        newZ = Cross(newX, Vector3f(0, 0, 1));
    }
    else
    {
        newZ = Cross(newX, Vector3f(0, 1, 0));
    }
    return Frame(newX, Cross(newZ, newX), newZ);
}

inline Frame Frame::FromY(const Vector3f &y)
{
    Vector3f newY = Normalize(y), newX;
    if(1 - std::abs(newY.z) < 0.1f)
    {
        newX = Cross(newY, Vector3f(1, 0, 0));
    }
    else
    {
        newX = Cross(newY, Vector3f(0, 0, 1));
    }
    return Frame(newX, newY, Cross(newX, newY));
}

inline Frame Frame::FromZ(const Vector3f &z)
{
    Vector3f newZ = Normalize(z), newY;
    if(1 - std::abs(newZ.x) < 0.1f)
    {
        newY = Cross(newZ, Vector3f(0, 1, 0));
    }
    else
    {
        newY = Cross(newZ, Vector3f(1, 0, 0));
    }
    return Frame(Cross(newZ, newY), newY, newZ);
}

inline Vector3f Frame::LocalToGlobal(const Vector3f &local) const
{
    return x * local.x + y * local.y + z * local.z;
}

inline Vector3f Frame::GlobalToLocal(const Vector3f &global) const
{
    return Vector3f(Dot(global, x), Dot(global, y), Dot(global, z));
}

inline Frame Frame::RotateToNewZ(const Vector3f &newZ) const
{
    const Vector3f newX = Cross(y, newZ);
    const Vector3f newY = Cross(newZ, newX);
    return Frame(newX, newY, newZ);
}

inline bool Frame::InPositiveZHemisphere(const Vector3f &v) const
{
    return Dot(v, z) > 0;
}

RTRC_END
