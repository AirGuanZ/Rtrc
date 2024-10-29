#pragma once

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Archive/ArchiveInterface.h>
#include <Rtrc/Core/Math/Common.h>

RTRC_BEGIN

template<typename T>
class Vector2
{
public:

    using Component = T;

    constexpr Vector2();

    explicit constexpr Vector2(T value);

    template<typename U> requires !std::is_same_v<T, U>
    explicit constexpr Vector2(const Vector2<U> &other);

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
Vector2<T> NormalizeIfNotZero(const Vector2<T> &v);
template<typename T>
T Distance(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
T Cos(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
T AngleBetween(const Vector2<T> &a, const Vector2<T> &b);

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
Vector2<T> Abs(const Vector2<T> &v);
template<typename T>
auto AddReduce(const Vector2<T> &v);
template<typename T>
T MaxReduce(const Vector2<T> &v);
template<typename T>
T MinReduce(const Vector2<T> &v);

template<std::floating_point T>
Vector2<T> Lerp(const Vector2<T> &lhs, const Vector2<T> &rhs, const Vector2<T> &t);

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
    : Vector2(T{})
{
    
}

template<typename T>
constexpr Vector2<T>::Vector2(T value)
    : Vector2(value, value)
{
    
}

template <typename T>
template <typename U> requires !std::is_same_v<T, U>
constexpr Vector2<T>::Vector2(const Vector2<U> &other)
    : x(T(other.x), T(other.y))
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
Vector2<T> NormalizeIfNotZero(const Vector2<T> &v)
{
    if(v == Vector2<T>())
    {
        return {};
    }
    return Rtrc::Normalize(v);
}

template<typename T>
T Distance(const Vector2<T> &a, const Vector2<T> &b)
{
    return Rtrc::Length(a - b);
}

template<typename T>
T Cos(const Vector2<T> &a, const Vector2<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
}

template<typename T>
T AngleBetween(const Vector2<T> &a, const Vector2<T> &b)
{
    return std::acos(Rtrc::Clamp(Rtrc::Cos(a, b), T(-1), T(1)));
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

template<typename T>
Vector2<T> Abs(const Vector2<T> &v)
{
    return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
auto AddReduce(const Vector2<T> &v)
{
    return v.x + v.y;
}

template<typename T>
T MaxReduce(const Vector2<T> &v)
{
    return (std::max)({ v.x, v.y });
}

template<typename T>
T MinReduce(const Vector2<T> &v)
{
    return (std::min)({ v.x, v.y });
}

template<std::floating_point T>
Vector2<T> Lerp(const Vector2<T> &lhs, const Vector2<T> &rhs, const Vector2<T> &t)
{
    return Vector2<T>(Rtrc::Lerp(lhs.x, rhs.x, t.x), Rtrc::Lerp(lhs.y, rhs.y, t.y));
}

RTRC_END

template <typename T>
struct std::formatter<Rtrc::Vector2<T>> : std::formatter<std::string>
{
    auto format(const Rtrc::Vector2<T> &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {})", p.x, p.y), ctx);
    }
};
