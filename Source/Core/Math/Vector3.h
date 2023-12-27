#pragma once

#include <Core/Math/Vector2.h>

RTRC_BEGIN

template<typename T>
class Vector3
{
public:

    using Component = T;

    constexpr Vector3();

    explicit constexpr Vector3(T value);

    constexpr Vector3(T x, T y, T z);

    constexpr Vector3(const Vector2<T> &xy, T z);

    T operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T> ToTuple() const;

    size_t Hash() const { return Rtrc::Hash(x, y, z); }

    Vector3 &operator+=(const Vector3 &rhs) { return *this = *this + rhs; }
    Vector3 &operator-=(const Vector3 &rhs) { return *this = *this - rhs; }

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
auto operator+(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator-(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator*(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator/(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
auto operator-(const Vector3<T> &v);

template<typename T>
auto operator*(const Vector3<T> &a, T b);
template<typename T>
auto operator*(T a, const Vector3<T> &b);
template<typename T>
auto operator/(const Vector3<T> &a, T b);

template<typename T>
T Dot(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
Vector3<T> Cross(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
T LengthSquare(const Vector3<T> &v);
template<typename T>
T Length(const Vector3<T> &v);
template<typename T>
Vector3<T> Normalize(const Vector3<T> &v);
template<typename T>
Vector3<T> NormalizeIfNotZero(const Vector3<T> &v);
template<typename T>
T Distance(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
Vector3<T> Min(const Vector3<T> &lhs, const Vector3<T> &rhs);
template<typename T>
Vector3<T> Max(const Vector3<T> &lhs, const Vector3<T> &rhs);

template<typename T>
struct ArchiveTransferTrait<Vector3<T>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, Vector3<T> &object)
    {
        if(ar.BeginTransferTuple(name, 3))
        {
            ar.Transfer("x", object.x);
            ar.Transfer("y", object.y);
            ar.Transfer("z", object.z);
            ar.EndTransferTuple();
        }
    }
};

template<typename T>
constexpr Vector3<T>::Vector3()
    : Vector3(T{})
{
    
}

template<typename T>
constexpr Vector3<T>::Vector3(T value)
    : Vector3(value, value, value)
{
    
}

template<typename T>
constexpr Vector3<T>::Vector3(T x, T y, T z)
    : x(x), y(y), z(z)
{
    
}

template<typename T>
constexpr Vector3<T>::Vector3(const Vector2<T> &xy, T z)
    : Vector3(xy.x, xy.y, z)
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

template<typename T>
auto operator+(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename T>
auto operator-(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename T>
auto operator*(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

template<typename T>
auto operator/(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

template<typename T>
auto operator-(const Vector3<T> &v)
{
    return Vector3<T>(-v.x, -v.y, -v.z);
}

template<typename T>
auto operator*(const Vector3<T> &a, T b)
{
    return Vector3<T>(a.x * b, a.y * b, a.z * b);
}

template<typename T>
auto operator*(T a, const Vector3<T> &b)
{
    return b * a;
}

template<typename T>
auto operator/(const Vector3<T> &a, T b)
{
    return a * (T(1) / b);
}

template<typename T>
T Dot(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<typename T>
Vector3<T> Cross(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

template<typename T>
T LengthSquare(const Vector3<T> &v)
{
    return Dot(v, v);
}

template<typename T>
T Length(const Vector3<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(LengthSquare(v));
}

template<typename T>
Vector3<T> Normalize(const Vector3<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    const T invLen = 1 / Length(v);
    return Vector3<T>(invLen * v.x, invLen * v.y, invLen * v.z);
}

template<typename T>
Vector3<T> NormalizeIfNotZero(const Vector3<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    const T len = Length(v);
    if(len == 0)
    {
        return Vector3<T>();
    }
    const T invLen = 1 / len;
    return Vector3<T>(invLen * v.x, invLen * v.y, invLen * v.z);
}

template<typename T>
T Distance(const Vector3<T> &a, const Vector3<T> &b)
{
    return Rtrc::Length(a - b);
}

template<typename T>
Vector3<T> Min(const Vector3<T> &lhs, const Vector3<T> &rhs)
{
    return Vector3<T>(
        (std::min)(lhs.x, rhs.x),
        (std::min)(lhs.y, rhs.y),
        (std::min)(lhs.z, rhs.z));
}

template<typename T>
Vector3<T> Max(const Vector3<T> &lhs, const Vector3<T> &rhs)
{
    return Vector3<T>(
        (std::max)(lhs.x, rhs.x),
        (std::max)(lhs.y, rhs.y),
        (std::max)(lhs.z, rhs.z));
}

RTRC_END
