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

template<typename T>
class Vector3
{
public:

    using Component = T;

    constexpr Vector3();

    explicit constexpr Vector3(T value);

    constexpr Vector3(T x, T y, T z);

    template<typename U> requires !std::is_same_v<T, U>
    explicit constexpr Vector3(const Vector3<U> &other);

    constexpr Vector3(const Vector2<T> &xy, T z);

    const T &operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T> ToTuple() const;

    Vector2<T> xz() const { return Vector2<T>(x, z); }
    Vector2<T> xy() const { return Vector2<T>(x, y); }

    template<typename U>
    constexpr Vector3<U> To() const;

    size_t Hash() const;

    Vector3 &operator+=(const Vector3 &rhs) { return *this = *this + rhs; }
    Vector3 &operator-=(const Vector3 &rhs) { return *this = *this - rhs; }

    T x, y, z;
};

template<typename T>
class Vector4
{
public:

    using Component = T;

    constexpr Vector4();

    explicit constexpr Vector4(T value);

    constexpr Vector4(T x, T y, T z, T w);

    template<typename U> requires !std::is_same_v<T, U>
    explicit constexpr Vector4(const Vector4<U> &other);

    constexpr Vector4(const Vector3<T> &xyz, T w);

    const T &operator[](size_t i) const;

    T &operator[](size_t i);

    std::tuple<T, T, T, T> ToTuple() const;

    Vector3<T> xyz() const;

    Vector4 &operator+=(const Vector4 &rhs);

    T x, y, z, w;
};

using Vector2f = Vector2<float>;
using Vector2d = Vector2<double>;
using Vector2i = Vector2<int>;
using Vector2u = Vector2<unsigned int>;
using Vector2b = Vector2<unsigned char>;

using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
using Vector3i = Vector3<int32_t>;
using Vector3u = Vector3<uint32_t>;
using Vector3b = Vector3<uint8_t>;

using Vector4f = Vector4<float>;
using Vector4d = Vector4<double>;
using Vector4i = Vector4<int32_t>;
using Vector4u = Vector4<uint32_t>;
using Vector4b = Vector4<uint8_t>;

template<typename T>
auto operator<=>(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator<=>(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
bool operator==(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
bool operator==(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
auto operator+(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator+(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator+(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
auto operator-(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator-(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator-(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
auto operator*(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator*(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator*(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
auto operator/(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
auto operator/(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
auto operator/(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
auto operator+(const Vector2<T> &v);
template<typename T>
auto operator+(const Vector3<T> &v);
template<typename T>
auto operator+(const Vector4<T> &v);

template<typename T>
auto operator-(const Vector2<T> &a);
template<typename T>
auto operator-(const Vector3<T> &v);
template<typename T>
auto operator-(const Vector4<T> &v);

template<typename T>
auto operator*(const Vector2<T> &a, T b);
template<typename T>
auto operator*(const Vector3<T> &a, T b);
template<typename T>
auto operator*(const Vector4<T> &a, T b);

template<typename T>
auto operator*(T a, const Vector2<T> &b);
template<typename T>
auto operator*(T a, const Vector3<T> &b);
template<typename T>
auto operator*(T a, const Vector4<T> &b);

template<typename T>
auto operator/(const Vector2<T> &a, T b);
template<typename T>
auto operator/(const Vector3<T> &a, T b);
template<typename T>
auto operator/(const Vector4<T> &a, T b);

template<typename T>
T Dot(const Vector2<T> &lhs, const Vector2<T> &rhs);
template<typename T>
T Dot(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
T Dot(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
Vector3<T> Cross(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
T LengthSquare(const Vector2<T> &v);
template<typename T>
T LengthSquare(const Vector3<T> &v);
template<typename T>
T LengthSquare(const Vector4<T> &v);

template<typename T>
T Length(const Vector2<T> &v);
template<typename T>
T Length(const Vector3<T> &v);
template<typename T>
T Length(const Vector4<T> &v);

template<typename T>
Vector2<T> Normalize(const Vector2<T> &v);
template<typename T>
Vector3<T> Normalize(const Vector3<T> &v);
template<typename T>
Vector4<T> Normalize(const Vector4<T> &v);

template<typename T>
Vector2<T> NormalizeIfNotZero(const Vector2<T> &v);
template<typename T>
Vector3<T> NormalizeIfNotZero(const Vector3<T> &v);
template<typename T>
Vector4<T> NormalizeIfNotZero(const Vector4<T> &v);

template<typename T>
T Distance(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
T Distance(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
T Distance(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
T Cos(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
T Cos(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
T Cos(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
T AngleBetween(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
T AngleBetween(const Vector3<T> &a, const Vector3<T> &b);
template<typename T>
T AngleBetween(const Vector4<T> &a, const Vector4<T> &b);

template<typename T>
Vector2<T> Min(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
Vector3<T> Min(const Vector3<T> &lhs, const Vector3<T> &rhs);
template<typename T>
Vector4<T> Min(const Vector4<T> &lhs, const Vector4<T> &rhs);

template<typename T>
Vector2<T> Max(const Vector2<T> &a, const Vector2<T> &b);
template<typename T>
Vector3<T> Max(const Vector3<T> &lhs, const Vector3<T> &rhs);
template<typename T>
Vector4<T> Max(const Vector4<T> &lhs, const Vector4<T> &rhs);

template<typename T>
Vector2<T> Floor(const Vector2<T> &v);
template<typename T>
Vector3<T> Floor(const Vector3<T> &v);
template<typename T>
Vector4<T> Floor(const Vector4<T> &v);

template<typename T>
Vector2<T> Ceil(const Vector2<T> &v);
template<typename T>
Vector3<T> Ceil(const Vector3<T> &v);
template<typename T>
Vector4<T> Ceil(const Vector4<T> &v);

template<typename T>
Vector2<T> Abs(const Vector2<T> &v);
template<typename T>
Vector3<T> Abs(const Vector3<T> &v);
template<typename T>
Vector4<T> Abs(const Vector4<T> &v);

template<typename T>
Vector2<T> Round(const Vector2<T> &v);
template<typename T>
Vector3<T> Round(const Vector3<T> &v);
template<typename T>
Vector4<T> Round(const Vector4<T> &v);

template<typename T>
auto AddReduce(const Vector2<T> &v);
template<typename T>
auto AddReduce(const Vector3<T> &v);
template<typename T>
auto AddReduce(const Vector4<T> &v);

template<typename T>
T MaxReduce(const Vector2<T> &v);
template<typename T>
T MaxReduce(const Vector3<T> &v);
template<typename T>
T MaxReduce(const Vector4<T> &v);

template<typename T>
T MinReduce(const Vector2<T> &v);
template<typename T>
T MinReduce(const Vector3<T> &v);
template<typename T>
T MinReduce(const Vector4<T> &v);

template<typename T>
T MulReduce(const Vector2<T> &v);
template<typename T>
T MulReduce(const Vector3<T> &v);
template<typename T>
T MulReduce(const Vector4<T> &v);

template<std::floating_point T>
Vector2<T> Lerp(const Vector2<T> &lhs, const Vector2<T> &rhs, const Vector2<T> &t);
template<std::floating_point T>
Vector3<T> Lerp(const Vector3<T> &lhs, const Vector3<T> &rhs, const Vector3<T> &t);
template<std::floating_point T>
Vector4<T> Lerp(const Vector4<T> &lhs, const Vector4<T> &rhs, const Vector4<T> &t);

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
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const Vector2<T> &object)
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

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const Vector3<T> &object)
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
struct ArchiveTransferTrait<Vector4<T>>
{
    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, Vector4<T> &object)
    {
        if(ar.BeginTransferTuple(name, 4))
        {
            ar.Transfer("x", object.x);
            ar.Transfer("y", object.y);
            ar.Transfer("y", object.z);
            ar.Transfer("w", object.w);
            ar.EndTransferTuple();
        }
    }

    template<typename Archive>
    static void Transfer(Archive &ar, std::string_view name, const Vector4<T> &object)
    {
        if(ar.BeginTransferTuple(name, 4))
        {
            ar.Transfer("x", object.x);
            ar.Transfer("y", object.y);
            ar.Transfer("y", object.z);
            ar.Transfer("w", object.w);
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

template <typename T>
template <typename U> requires !std::is_same_v<T, U>
constexpr Vector3<T>::Vector3(const Vector3<U> &other)
    : x(T(other.x)), y(T(other.y)), z(T(other.z))
{

}

template<typename T>
constexpr Vector3<T>::Vector3(const Vector2<T> &xy, T z)
    : Vector3(xy.x, xy.y, z)
{

}

template<typename T>
const T &Vector3<T>::operator[](size_t i) const
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

template <typename T>
template <typename U>
constexpr Vector3<U> Vector3<T>::To() const
{
    return Vector3<U>(
        static_cast<U>(x),
        static_cast<U>(y),
        static_cast<U>(z));
}

template <typename T>
size_t Vector3<T>::Hash() const
{
    return Rtrc::Hash(x, y, z);
}

template <typename T>
constexpr Vector4<T>::Vector4()
    : Vector4(T{})
{

}

template<typename T>
constexpr Vector4<T>::Vector4(T value)
    : Vector4(value, value, value, value)
{

}

template<typename T>
constexpr Vector4<T>::Vector4(T x, T y, T z, T w)
    : x(x), y(y), z(z), w(w)
{

}

template <typename T>
template <typename U> requires !std::is_same_v<T, U>
constexpr Vector4<T>::Vector4(const Vector4<U> &other)
    : x(T(other.x)), y(T(other.y)), z(T(other.z)), w(T(other.w))
{

}

template<typename T>
constexpr Vector4<T>::Vector4(const Vector3<T> &xyz, T w)
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

template <typename T>
Vector4<T> &Vector4<T>::operator+=(const Vector4 &rhs)
{
    *this = *this + rhs;
    return *this;
}

template<typename T>
auto operator<=>(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
auto operator<=>(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
auto operator<=>(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() <=> b.ToTuple();
}

template<typename T>
bool operator==(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

template<typename T>
bool operator==(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

template<typename T>
bool operator==(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.ToTuple() == b.ToTuple();
}

template<typename T>
auto operator+(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
auto operator+(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template<typename T>
auto operator+(const Vector4<T> &a, const Vector4<T> &b)
{
    return Vector4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template<typename T>
auto operator-(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x - b.x, a.y - b.y);
}

template<typename T>
auto operator-(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template<typename T>
auto operator-(const Vector4<T> &a, const Vector4<T> &b)
{
    return Vector4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template<typename T>
auto operator*(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x * b.x, a.y * b.y);
}

template<typename T>
auto operator*(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

template<typename T>
auto operator*(const Vector4<T> &a, const Vector4<T> &b)
{
    return Vector4<T>(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);
}

template<typename T>
auto operator/(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>(a.x / b.x, a.y / b.y);
}

template<typename T>
auto operator/(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.x / b.x, a.y / b.y, a.z / b.z);
}

template<typename T>
auto operator/(const Vector4<T> &a, const Vector4<T> &b)
{
    return Vector4<T>(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);
}

template<typename T>
auto operator+(const Vector2<T> &v)
{
    return v;
}

template<typename T>
auto operator+(const Vector3<T> &v)
{
    return v;
}

template<typename T>
auto operator+(const Vector4<T> &v)
{
    return v;
}

template<typename T>
auto operator-(const Vector2<T> &a)
{
    static_assert(std::is_signed_v<T>);
    return Vector2<T>(-a.x, -a.y);
}

template<typename T>
auto operator-(const Vector3<T> &v)
{
    return Vector3<T>(-v.x, -v.y, -v.z);
}

template<typename T>
auto operator-(const Vector4<T> &v)
{
    return Vector4<T>(-v.x, -v.y, -v.z, -v.w);
}

template<typename T>
auto operator*(const Vector2<T> &a, T b)
{
    return Vector2<T>(a.x * b, a.y * b);
}

template<typename T>
auto operator*(const Vector3<T> &a, T b)
{
    return Vector3<T>(a.x * b, a.y * b, a.z * b);
}

template<typename T>
auto operator*(const Vector4<T> &a, T b)
{
    return Vector4<T>(a.x * b, a.y * b, a.z * b, a.w * b);
}

template<typename T>
auto operator*(T a, const Vector2<T> &b)
{
    return b * a;
}

template<typename T>
auto operator*(T a, const Vector3<T> &b)
{
    return b * a;
}

template<typename T>
auto operator*(T a, const Vector4<T> &b)
{
    return b * a;
}

template<typename T>
auto operator/(const Vector2<T> &a, T b)
{
    return Vector2<T>(a.x / b, a.y / b);
}

template<typename T>
auto operator/(const Vector3<T> &a, T b)
{
    return a * (T(1) / b);
}

template<typename T>
auto operator/(const Vector4<T> &a, T b)
{
    return a * (T(1) / b);
}

template<typename T>
T Dot(const Vector2<T> &a, const Vector2<T> &b)
{
    return a.x * b.x + a.y * b.y;
}

template<typename T>
T Dot(const Vector3<T> &a, const Vector3<T> &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<typename T>
T Dot(const Vector4<T> &a, const Vector4<T> &b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template<typename T>
Vector3<T> Cross(const Vector3<T> &a, const Vector3<T> &b)
{
    return Vector3<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

template<typename T>
T LengthSquare(const Vector2<T> &v)
{
    return Rtrc::Dot(v, v);
}

template<typename T>
T LengthSquare(const Vector3<T> &v)
{
    return Dot(v, v);
}

template<typename T>
T LengthSquare(const Vector4<T> &v)
{
    return Dot(v, v);
}

template<typename T>
T Length(const Vector2<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(Rtrc::LengthSquare(v));
}

template<typename T>
T Length(const Vector3<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(LengthSquare(v));
}

template<typename T>
T Length(const Vector4<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    return std::sqrt(LengthSquare(v));
}

template<typename T>
Vector2<T> Normalize(const Vector2<T> &v)
{
    const T invLen = 1 / Rtrc::Length(v);
    return invLen * v;
}

template<typename T>
Vector3<T> Normalize(const Vector3<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    const T invLen = 1 / Length(v);
    return Vector3<T>(invLen * v.x, invLen * v.y, invLen * v.z);
}

template<typename T>
Vector4<T> Normalize(const Vector4<T> &v)
{
    static_assert(std::is_floating_point_v<T>);
    const T invLen = 1 / Length(v);
    return Vector4<T>(invLen * v.x, invLen * v.y, invLen * v.z, invLen * v.w);
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
Vector3<T> NormalizeIfNotZero(const Vector3<T> &v)
{
    if(v == Vector3<T>())
    {
        return {};
    }
    return Rtrc::Normalize(v);
}

template<typename T>
Vector4<T> NormalizeIfNotZero(const Vector4<T> &v)
{
    if(v == Vector4<T>())
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
T Distance(const Vector3<T> &a, const Vector3<T> &b)
{
    return Rtrc::Length(a - b);
}

template<typename T>
T Distance(const Vector4<T> &a, const Vector4<T> &b)
{
    return Rtrc::Length(a - b);
}

template<typename T>
T Cos(const Vector2<T> &a, const Vector2<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
}

template<typename T>
T Cos(const Vector3<T> &a, const Vector3<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
}

template<typename T>
T Cos(const Vector4<T> &a, const Vector4<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
}

template<typename T>
T AngleBetween(const Vector2<T> &a, const Vector2<T> &b)
{
    return std::acos(Rtrc::Clamp(Rtrc::Cos(a, b), T(-1), T(1)));
}

template<typename T>
T AngleBetween(const Vector3<T> &a, const Vector3<T> &b)
{
    return std::acos(Rtrc::Clamp(Rtrc::Cos(a, b), T(-1), T(1)));
}

template<typename T>
T AngleBetween(const Vector4<T> &a, const Vector4<T> &b)
{
    return std::acos(Rtrc::Clamp(Rtrc::Cos(a, b), T(-1), T(1)));
}

template<typename T>
Vector2<T> Min(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>((std::min)(a.x, b.x), (std::min)(a.y, b.y));
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
Vector4<T> Min(const Vector4<T> &lhs, const Vector4<T> &rhs)
{
    return Vector4<T>(
        (std::min)(lhs.x, rhs.x),
        (std::min)(lhs.y, rhs.y),
        (std::min)(lhs.z, rhs.z),
        (std::min)(lhs.w, rhs.w));
}

template<typename T>
Vector2<T> Max(const Vector2<T> &a, const Vector2<T> &b)
{
    return Vector2<T>((std::max)(a.x, b.x), (std::max)(a.y, b.y));
}

template<typename T>
Vector3<T> Max(const Vector3<T> &lhs, const Vector3<T> &rhs)
{
    return Vector3<T>(
        (std::max)(lhs.x, rhs.x),
        (std::max)(lhs.y, rhs.y),
        (std::max)(lhs.z, rhs.z));
}

template<typename T>
Vector4<T> Max(const Vector4<T> &lhs, const Vector4<T> &rhs)
{
    return Vector4<T>(
        (std::max)(lhs.x, rhs.x),
        (std::max)(lhs.y, rhs.y),
        (std::max)(lhs.z, rhs.z),
        (std::max)(lhs.w, rhs.w));
}

template<typename T>
Vector2<T> Floor(const Vector2<T> &v)
{
    return Vector2<T>(std::floor(v.x), std::floor(v.y));
}

template<typename T>
Vector3<T> Floor(const Vector3<T> &v)
{
    return Vector3<T>(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

template<typename T>
Vector4<T> Floor(const Vector4<T> &v)
{
    return Vector4<T>(std::floor(v.x), std::floor(v.y), std::floor(v.z), std::floor(v.w));
}

template<typename T>
Vector2<T> Ceil(const Vector2<T> &v)
{
    return Vector2<T>(std::ceil(v.x), std::ceil(v.y));
}

template<typename T>
Vector3<T> Ceil(const Vector3<T> &v)
{
    return Vector3<T>(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
}

template<typename T>
Vector4<T> Ceil(const Vector4<T> &v)
{
    return Vector4<T>(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z), std::ceil(v.w));
}

template<typename T>
Vector2<T> Abs(const Vector2<T> &v)
{
    return Vector2<T>(std::abs(v.x), std::abs(v.y));
}

template<typename T>
Vector3<T> Abs(const Vector3<T> &v)
{
    return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template<typename T>
Vector4<T> Abs(const Vector4<T> &v)
{
    return Vector4<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w));
}

template<typename T>
Vector2<T> Round(const Vector2<T> &v)
{
    return Vector2<T>(std::round(v.x), std::round(v.y));
}

template<typename T>
Vector3<T> Round(const Vector3<T> &v)
{
    return Vector3<T>(std::round(v.x), std::round(v.y), std::round(v.z));
}

template<typename T>
Vector4<T> Round(const Vector4<T> &v)
{
    return Vector4<T>(std::round(v.x), std::round(v.y), std::round(v.z), std::round(v.w));
}

template<typename T>
auto AddReduce(const Vector2<T> &v)
{
    return v.x + v.y;
}

template<typename T>
auto AddReduce(const Vector3<T> &v)
{
    return v.x + v.y + v.z;
}

template<typename T>
auto AddReduce(const Vector4<T> &v)
{
    return v.x + v.y + v.z + v.w;
}

template<typename T>
T MaxReduce(const Vector2<T> &v)
{
    return (std::max)({ v.x, v.y });
}

template<typename T>
T MaxReduce(const Vector3<T> &v)
{
    return (std::max)({ v.x, v.y, v.z });
}

template<typename T>
T MaxReduce(const Vector4<T> &v)
{
    return (std::max)({ v.x, v.y, v.z, v.w });
}

template<typename T>
T MinReduce(const Vector2<T> &v)
{
    return (std::min)({ v.x, v.y });
}

template<typename T>
T MinReduce(const Vector3<T> &v)
{
    return (std::min)({ v.x, v.y, v.z });
}

template<typename T>
T MinReduce(const Vector4<T> &v)
{
    return (std::min)({ v.x, v.y, v.z, v.w });
}

template<typename T>
T MulReduce(const Vector2<T> &v)
{
    return v.x * v.y;
}

template<typename T>
T MulReduce(const Vector3<T> &v)
{
    return v.x * v.y * v.z;
}

template<typename T>
T MulReduce(const Vector4<T> &v)
{
    return v.x * v.y * v.z * v.w;
}

template<std::floating_point T>
Vector2<T> Lerp(const Vector2<T> &lhs, const Vector2<T> &rhs, const Vector2<T> &t)
{
    return Vector2<T>(Rtrc::Lerp(lhs.x, rhs.x, t.x), Rtrc::Lerp(lhs.y, rhs.y, t.y));
}

template<std::floating_point T>
Vector3<T> Lerp(const Vector3<T> &lhs, const Vector3<T> &rhs, const Vector3<T> &t)
{
    return Vector3<T>(Rtrc::Lerp(lhs.x, rhs.x, t.x), Rtrc::Lerp(lhs.y, rhs.y, t.y), Rtrc::Lerp(lhs.z, rhs.z, t.z));
}

template<std::floating_point T>
Vector4<T> Lerp(const Vector4<T> &lhs, const Vector4<T> &rhs, const Vector4<T> &t)
{
    return Vector4<T>(
        Rtrc::Lerp(lhs.x, rhs.x, t.x), Rtrc::Lerp(lhs.y, rhs.y, t.y),
        Rtrc::Lerp(lhs.z, rhs.z, t.z), Rtrc::Lerp(lhs.w, rhs.w, t.w));
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

template <typename T>
struct std::formatter<Rtrc::Vector3<T>> : std::formatter<std::string>
{
    auto format(const Rtrc::Vector3<T> &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {}, {})", p.x, p.y, p.z), ctx);
    }
};

template <typename T>
struct std::formatter<Rtrc::Vector4<T>> : std::formatter<std::string>
{
    auto format(const Rtrc::Vector4<T> &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {}, {}, {})", p.x, p.y, p.z, p.w), ctx);
    }
};
