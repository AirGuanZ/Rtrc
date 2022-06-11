#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

template<typename T>
class Vector2
{
public:

    using Component = T;

    explicit Vector2(T value = T());

    Vector2(T x, T y);

    T operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T> ToTuple() const;

    T x, y;
};

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int32_t>;
using Vector2u = Vector2<uint32_t>;

template<typename T>
auto operator<=>(const Vector2<T> &a, const Vector2<T> &b);

template<typename T>
bool operator==(const Vector2<T> &a, const Vector2<T> &b);

template<typename T>
Vector2<T>::Vector2(T value)
    : Vector2(value, value)
{
    
}

template<typename T>
Vector2<T>::Vector2(T x, T y)
    : x(x), y(y)
{
    
}

template<typename T>
T &Vector2<T>::operator[](size_t i)
{
    return *(&x + i);
}

template<typename T>
T Vector2<T>::operator[](size_t i) const
{
    return *(&x + i);
}

template<typename T>
std::tuple<T, T> Vector2<T>::ToTuple() const
{
    return std::make_tuple(x, y);
}

template<typename T>
auto operator<=>(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
bool operator==(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

RTRC_END
