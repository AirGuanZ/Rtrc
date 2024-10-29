#pragma once

#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

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

    T x, y, z, w;
};

using Vector4f = Vector4<float>;
using Vector4d = Vector4<double>;
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
Vector4<T> operator/(const Vector4<T>& a, const Vector4<T>& b);

template<typename T>
Vector4<T> operator-(const Vector4<T>& v);

template<typename T>
Vector4<T> operator*(const Vector4<T>& a, T b);
template<typename T>
Vector4<T> operator*(T a, const Vector4<T>& b);
template<typename T>
Vector4<T> operator/(const Vector4<T>& a, T b);

template<typename T>
T Dot(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
T LengthSquare(const Vector4<T> &v);
template<typename T>
T Length(const Vector4<T> &v);
template<typename T>
Vector4<T> Normalize(const Vector4<T> &v);
template<typename T>
Vector4<T> NormalizeIfNotZero(const Vector4<T> &v);
template<typename T>
T Cos(const Vector4<T> &a, const Vector4<T> &b);
template<typename T>
T AngleBetween(const Vector4<T> &a, const Vector4<T> &b);

template<std::floating_point T>
Vector4<T> Lerp(const Vector4<T> &lhs, const Vector4<T> &rhs, const Vector4<T> &t);

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
};

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

template<typename T>
Vector4<T> operator/(const Vector4<T>& a, const Vector4<T>& b)
{
#define RTRC_ELEM(COMP) (a.COMP / b.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator-(const Vector4<T>& v)
{
#define RTRC_ELEM(COMP) (-v.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator*(const Vector4<T>& a, T b)
{
#define RTRC_ELEM(COMP) (a.COMP * b)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator*(T a, const Vector4<T>& b)
{
#define RTRC_ELEM(COMP) (a * b.COMP)
    return RTRC_CREATE_FOR_EACH_ELEM(RTRC_ELEM);
#undef RTRC_ELEM
}

template<typename T>
Vector4<T> operator/(const Vector4<T>& a, T b)
{
#define RTRC_ELEM(COMP) (a.COMP / b)
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
T Cos(const Vector4<T> &a, const Vector4<T> &b)
{
    return Rtrc::Dot(Rtrc::NormalizeIfNotZero(a), Rtrc::NormalizeIfNotZero(b));
}

template<typename T>
T AngleBetween(const Vector4<T> &a, const Vector4<T> &b)
{
    return std::acos(Rtrc::Clamp(Rtrc::Cos(a, b), T(-1), T(1)));
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
struct std::formatter<Rtrc::Vector4<T>> : std::formatter<std::string>
{
    auto format(const Rtrc::Vector4<T> &p, std::format_context &ctx) const
    {
        return formatter<std::string>::format(std::format("({}, {}, {}, {})", p.x, p.y, p.z, p.w), ctx);
    }
};
