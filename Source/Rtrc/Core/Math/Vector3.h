#pragma once

#include <Rtrc/Core/Math/Vector2.h>

RTRC_BEGIN

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

    size_t Hash() const { return Rtrc::Hash(x, y, z); }

    Vector3 &operator+=(const Vector3 &rhs) { return *this = *this + rhs; }
    Vector3 &operator-=(const Vector3 &rhs) { return *this = *this - rhs; }

    T x, y, z;
};

using Vector3f = Vector3<float>;
using Vector3d = Vector3<double>;
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
auto operator+(const Vector3<T> &v);
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
T Cos(const Vector3<T> &a, const Vector3<T> &b);

template<typename T>
Vector3<T> Min(const Vector3<T> &lhs, const Vector3<T> &rhs);
template<typename T>
Vector3<T> Max(const Vector3<T> &lhs, const Vector3<T> &rhs);

template<typename T>
Vector3<T> Floor(const Vector3<T> &v);
template<typename T>
Vector3<T> Ceil(const Vector3<T> &v);

template<typename T>
Vector3<T> Abs(const Vector3<T> &v);

template<typename T>
auto AddReduce(const Vector3<T> &v);
template<typename T>
T MaxReduce(const Vector3<T> &v);
template<typename T>
T MinReduce(const Vector3<T> &v);

template<std::floating_point T>
Vector3<T> Lerp(const Vector3<T> &lhs, const Vector3<T> &rhs, const Vector3<T> &t);

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
auto operator+(const Vector3<T> &v)
{
    return v;
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
T Cos(const Vector3<T> &a, const Vector3<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
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

template<typename T>
Vector3<T> Floor(const Vector3<T> &v)
{
    return Vector3<T>(std::floor(v.x), std::floor(v.y), std::floor(v.z));
}

template<typename T>
Vector3<T> Ceil(const Vector3<T> &v)
{
    return Vector3<T>(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
}

template<typename T>
Vector3<T> Abs(const Vector3<T> &v)
{
    return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template<typename T>
auto AddReduce(const Vector3<T> &v)
{
    return v.x + v.y + v.z;
}

template<typename T>
T MaxReduce(const Vector3<T> &v)
{
    return (std::max)({ v.x, v.y, v.z });
}

template<typename T>
T MinReduce(const Vector3<T> &v)
{
    return (std::min)({ v.x, v.y, v.z });
}

template<std::floating_point T>
Vector3<T> Lerp(const Vector3<T> &lhs, const Vector3<T> &rhs, const Vector3<T> &t)
{
    return Vector3<T>(Rtrc::Lerp(lhs.x, rhs.x, t.x), Rtrc::Lerp(lhs.y, rhs.y, t.y), Rtrc::Lerp(lhs.z, rhs.z, t.z));
}

RTRC_END

template <typename T>
struct std::formatter<Rtrc::Vector3<T>> : std::formatter<std::string>
{
    auto format(const Rtrc::Vector3<T> &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {}, {})", p.x, p.y, p.z), ctx);
    }
};
