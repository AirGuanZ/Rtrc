#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
class Vector4
{
public:

    using Component = T;

    explicit Vector4(T value = T());

    Vector4(T x, T y, T z, T w);

    T operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T, T> ToTuple() const;

    T x, y, z, w;
};

using Vector4f = Vector4<float>;
using Vector4i = Vector4<int32_t>;
using Vector4u = Vector4<uint32_t>;

template<typename T>
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b);

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
T Vector4<T>::operator[](size_t i) const
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
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

RTRC_END
