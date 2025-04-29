#pragma once

#include <Rtrc/Core/Math/Vector.h>

RTRC_BEGIN

template<typename T>
class Frame
{
    static_assert(std::is_floating_point_v<T>);

public:

    Vector3<T> x, y, z;

    Frame();

    Frame(const Vector3<T> &x, const Vector3<T> &y, const Vector3<T> &z);

    static Frame FromX(const Vector3<T> &x);

    static Frame FromY(const Vector3<T> &y);

    static Frame FromZ(const Vector3<T> &z);

    Vector3<T> LocalToGlobal(const Vector3<T> &local) const;

    Vector3<T> GlobalToLocal(const Vector3<T> &global) const;

    Frame RotateToNewZ(const Vector3<T> &newZ) const;

    bool InPositiveZHemisphere(const Vector3<T> &v) const;
};

using Framef = Frame<float>;
using Framed = Frame<double>;

template<typename T>
Frame<T>::Frame()
    : Frame(Vector3<T>(1, 0, 0), Vector3<T>(0, 1, 0), Vector3<T>(0, 0, 1))
{

}

template<typename T>
Frame<T>::Frame(const Vector3<T> &x, const Vector3<T> &y, const Vector3<T> &z)
    : x(Normalize(x)), y(Normalize(y)), z(Normalize(z))
{

}

template<typename T>
Frame<T> Frame<T>::FromX(const Vector3<T> &x)
{
    Vector3<T> newX = Normalize(x), newZ;
    if(1 - std::abs(newX.y) < 0.1f)
    {
        newZ = Cross(newX, Vector3<T>(0, 0, 1));
    }
    else
    {
        newZ = Cross(newX, Vector3<T>(0, 1, 0));
    }
    return Frame(newX, Cross(newZ, newX), newZ);
}

template<typename T>
Frame<T> Frame<T>::FromY(const Vector3<T> &y)
{
    Vector3<T> newY = Normalize(y), newX;
    if(1 - std::abs(newY.z) < 0.1f)
    {
        newX = Cross(newY, Vector3<T>(1, 0, 0));
    }
    else
    {
        newX = Cross(newY, Vector3<T>(0, 0, 1));
    }
    return Frame(newX, newY, Cross(newX, newY));
}

template<typename T>
Frame<T> Frame<T>::FromZ(const Vector3<T> &z)
{
    Vector3<T> newZ = Normalize(z), newY;
    if(1 - std::abs(newZ.x) < 0.1f)
    {
        newY = Cross(newZ, Vector3<T>(0, 1, 0));
    }
    else
    {
        newY = Cross(newZ, Vector3<T>(1, 0, 0));
    }
    return Frame(Cross(newY, newZ), newY, newZ);
}

template<typename T>
Vector3<T> Frame<T>::LocalToGlobal(const Vector3<T> &local) const
{
    return x * local.x + y * local.y + z * local.z;
}

template<typename T>
Vector3<T> Frame<T>::GlobalToLocal(const Vector3<T> &global) const
{
    return Vector3<T>(Dot(global, x), Dot(global, y), Dot(global, z));
}

template<typename T>
Frame<T> Frame<T>::RotateToNewZ(const Vector3<T> &newZ) const
{
    const Vector3<T> newX = Cross(y, newZ);
    const Vector3<T> newY = Cross(newZ, newX);
    return Frame(newX, newY, newZ);
}

template<typename T>
bool Frame<T>::InPositiveZHemisphere(const Vector3<T> &v) const
{
    return Dot(v, z) > 0;
}

RTRC_END
