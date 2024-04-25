#pragma once

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Archive/ArchiveInterface.h>

RTRC_BEGIN

template<typename T>
class Vector2
{
public:

    using Component = T;

    constexpr Vector2();

    explicit constexpr Vector2(T value);

    constexpr Vector2(T x, T y);

    constexpr const T &operator[](size_t i) const;

    constexpr T &operator[](size_t i);

    constexpr std::tuple<T, T> ToTuple() const;

    template<typename U>
    constexpr Vector2<U> To() const;

    size_t Hash() const;

    Vector2 &operator+=(const Vector2 &rhs) { return *this = *this + rhs; }
    Vector2 &operator-=(const Vector2 &rhs) { return *this = *this - rhs; }

    T x, y;
};

using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;
using Vector2b = Vector2<unsigned char>;

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
auto operator-(const Vector2<T> &a);

template<typename T>
auto operator*(const Vector2<T> &a, T b);
template<typename T>
auto operator*(T a, const Vector2<T> &b);
template<typename T>
auto operator/(const Vector2<T> &a, T b);

template<typename T>
T Dot(const Vector2<T> &lhs, const Vector2<T> &rhs);
template<typename T>
T LengthSquare(const Vector2<T> &v);
template<typename T>
T Length(const Vector2<T> &v);
template<typename T>
Vector2<T> Normalize(const Vector2<T> &v);

template<typename T>
Vector2<T> Min(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
Vector2<T> Max(const Vector2<T> &a, const Vector2<T> &b);

template<typename T>
Vector2<T> Floor(const Vector2<T> &v);
template<typename T>
Vector2<T> Ceil(const Vector2<T> &v);
template<typename T>
Vector2<T> Round(const Vector2<T> &v);

template<typename T>
struct ArchiveTransferTrait<Vector2<T>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, Vector2<T> &object)
    {
        if(ar.BeginTransferTuple(name, 2))
        {
            ar.Transfer("x", object.x);
            ar.Transfer("y", object.y);
            ar.EndTransferTuple();
        }
    }
};

template<typename T>
constexpr Vector2<T>::Vector2()
    : Vector2(0)
{
    
}

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
constexpr const T &Vector2<T>::operator[](size_t i) const
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
auto operator-(const Vector2<T> &a)
{
    static_assert(std::is_signed_v<T>);
    return Vector2<T>(-a.x, -a.y);
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

template<typename T>
auto operator/(const Vector2<T> &a, T b)
{
    return Vector2<T>(a.x / b, a.y / b);
}

template<typename T>
T Dot(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.x * b.x + a.y * b.y;
}

template<typename T>
T LengthSquare(const Vector2<T> &v)
{
    return Rtrc::Dot(v, v);
}

template<typename T>
T Length(const Vector2<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(Rtrc::LengthSquare(v));
}

template<typename T>
Vector2<T> Normalize(const Vector2<T> &v)
{
    const T invLen = 1 / Rtrc::Length(v);
    return invLen * v;
}

template<typename T>
Vector2<T> Min(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>((std::min)(a.x, b.x), (std::min)(a.y, b.y));
}

template<typename T>
Vector2<T> Max(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>((std::max)(a.x, b.x), (std::max)(a.y, b.y));
}

template<typename T>
Vector2<T> Floor(const Vector2<T> &v)
{
    return Vector2<T>(std::floor(v.x), std::floor(v.y));
}

template<typename T>
Vector2<T> Ceil(const Vector2<T> &v)
{
    return Vector2<T>(std::ceil(v.x), std::ceil(v.y));
}

template<typename T>
Vector2<T> Round(const Vector2<T> &v)
{
    return Vector2<T>(std::round(v.x), std::round(v.y));
}

RTRC_END
