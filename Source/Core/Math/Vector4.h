#pragma once

#include <Core/Math/Vector3.h>

RTRC_BEGIN

template<typename T>
class Vector4
{
public:

    using Component = T;

    Vector4();

    explicit Vector4(T value);

    Vector4(T x, T y, T z, T w);

    Vector4(const Vector3<T> &xyz, T w);

    const T &operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T, T> ToTuple() const;

    Vector3<T> xyz() const;

    T x, y, z, w;
};

using Vector4f = Vector4<float>;
using Vector4i = Vector4<int32_t>;
using Vector4u = Vector4<uint32_t>;
using Vector4b = Vector4<uint8_t>;

template<typename T>
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
Vector4<T> operator+(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
Vector4<T> operator-(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
Vector4<T> operator*(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
T Dot(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
T LengthSquare(const Vector4<T> &v);
template<typename T>
T Length(const Vector4<T> &v);
template<typename T>
Vector4<T> Normalize(const Vector4<T> &v);

template <typename T>
Vector4<T>::Vector4()
    : Vector4(0, 0, 0, 0)
{
    
}

template<typename T>
Vector4<T>::Vector4(T value)
    : Vector4(value, value, value, value)
{

}

template<typename T>
Vector4<T>::Vector4(T x, T y, T z, T w)
    : x(x), y(y), z(z), w(w)
{

}

template<typename T>
Vector4<T>::Vector4(const Vector3<T> &xyz, T w)
    : Vector4(xyz.x, xyz.y, xyz.z, w)
{
    
}

template<typename T>
const T &Vector4<T>::operator[](size_t i) const
{
    return *(&x + i);
}

template<typename T>
T &Vector4<T>::operator[](size_t i)
{
    return *(&x + i);
}

template<typename T>
std::tuple<T, T, T, T> Vector4<T>::ToTuple() const
{
    return std::make_tuple(x, y, z, w);
}

template<typename T>
Vector3<T> Vector4<T>::xyz() const
{
    return { x, y, z };
}

template<typename T>
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

#define RTRC_CREATE_FOR_EACH_ELEM(F) { F(x), F(y), F(z), F(w) }

template<typename T>
Vector4<T> operator+(const Vector4<T> &a, const Vector4<T> &b)
{
#define RTRC_ELEM(COMP) (a.COMP + b.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator-(const Vector4<T> &a, const Vector4<T> &b)
{
#define RTRC_ELEM(COMP) (a.COMP - b.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator*(const Vector4<T> &a, const Vector4<T> &b)
{
#define RTRC_ELEM(COMP) (a.COMP * b.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

#undef RTRC_CREATE_FOR_EACH_ELEM

template<typename T>
T Dot(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template<typename T>
T LengthSquare(const Vector4<T> &v)
{
    return Dot(v, v);
}

template<typename T>
T Length(const Vector4<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(LengthSquare(v));
}

template<typename T>
Vector4<T> Normalize(const Vector4<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    const T invLen = 1 / Length(v);
    return Vector4<T>(invLen * v.x, invLen * v.y, invLen * v.z, invLen * v.w);
}

RTRC_END
