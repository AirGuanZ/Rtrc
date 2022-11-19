#pragma once

#include <Rtrc/Utility/Hash.h>

RTRC_BEGIN

template<typename T>
class Vector2
{
public:

    using Component = T;

    explicit constexpr Vector2(T value = T());

    constexpr Vector2(T x, T y);

    constexpr T operator[](size_t i) const;

    constexpr T &operator[](size_t i);

    constexpr std::tuple<T, T> ToTuple() const;

    template<typename U>
    constexpr Vector2<U> To() const;

    size_t Hash() const;

    T x, y;
};

using Vector2f = Vector2<float>;
using Vector2i = Vector2<int32_t>;
using Vector2u = Vector2<uint32_t>;
using Vector2b = Vector2<uint8_t>;

template<typename T>
auto operator<=>(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
bool operator==(const Vector2<T> &a, const Vector2<T> &b);

template<typename T>
auto operator+(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator-(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator*(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator/(const Vector2<T> &a, const Vector2<T> &b);

template<typename T>
auto operator*(const Vector2<T> &a, T b);
template<typename T>
auto operator*(T a, const Vector2<T> &b);

template<typename T>
constexpr Vector2<T>::Vector2(T value)
    : Vector2(value, value)
{
    
}

template<typename T>
constexpr Vector2<T>::Vector2(T x, T y)
    : x(x), y(y)
{
    
}

template<typename T>
constexpr T &Vector2<T>::operator[](size_t i)
{
    return *(&x + i);
}

template<typename T>
constexpr T Vector2<T>::operator[](size_t i) const
{
    return *(&x + i);
}

template<typename T>
constexpr std::tuple<T, T> Vector2<T>::ToTuple() const
{
    return std::make_tuple(x, y);
}

template <typename T>
template <typename U>
constexpr Vector2<U> Vector2<T>::To() const
{
    return Vector2<U>(static_cast<U>(x), static_cast<U>(y));
}

template<typename T>
size_t Vector2<T>::Hash() const
{
    return ::Rtrc::Hash(x, y);
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

template<typename T>
auto operator+(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
auto operator-(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x - b.x, a.y - b.y);
}

template<typename T>
auto operator*(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x * b.x, a.y * b.y);
}

template<typename T>
auto operator/(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x / b.x, a.y / b.y);
}

template<typename T>
auto operator*(const Vector2<T> &a, T b)
{
    return Vector2<T>(a.x * b, a.y * b);
}

template<typename T>
auto operator*(T a, const Vector2<T> &b)
{
    return b * a;
}

RTRC_END
