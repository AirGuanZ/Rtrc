#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
class Vector3
{
public:

    using Component = T;

    explicit Vector3(T value = T());

    Vector3(T x, T y, T z);

    T operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T> ToTuple() const;

    T x, y, z;
};

using Vector3f = Vector3<float>;
using Vector3i = Vector3<int32_t>;
using Vector3u = Vector3<uint32_t>;
using Vector3b = Vector3<uint8_t>;

template<typename T>
auto operator<=>(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
bool operator==(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
Vector3<T>::Vector3(T value)
    : Vector3(value, value, value)
{
    
}

template<typename T>
Vector3<T>::Vector3(T x, T y, T z)
    : x(x), y(y), z(z)
{
    
}

template<typename T>
T Vector3<T>::operator[](size_t i) const
{
    return *(&x + i);
}

template<typename T>
T &Vector3<T>::operator[](size_t i)
{
    return *(&x + i);
}

template<typename T>
std::tuple<T, T, T> Vector3<T>::ToTuple() const
{
    return std::make_tuple(x, y, z);
}

template<typename T>
auto operator<=>(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
bool operator==(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

RTRC_END
