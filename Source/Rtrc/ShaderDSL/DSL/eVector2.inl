#pragma once

#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/ShaderDSL/DSL/eVector2.h>
#include <Rtrc/ShaderDSL/DSL/RecordContext.h>

RTRC_EDSL_BEGIN

namespace Detail
{

    template<typename T>
    struct RawComponentTypeToVector2TypeName;

    template<>
    struct RawComponentTypeToVector2TypeName<uint32_t>
    {
        static constexpr const char *GetName() { return "uint2"; }
    };

    template<>
    struct RawComponentTypeToVector2TypeName<int32_t>
    {
        static constexpr const char *GetName() { return "int2"; }
    };

    template<>
    struct RawComponentTypeToVector2TypeName<float>
    {
        static constexpr const char *GetName() { return "float2"; }
    };

    template<>
    struct RawComponentTypeToVector2TypeName<bool>
    {
        static constexpr const char *GetName() { return "bool2"; }
    };

    template<typename T>
    struct IsVector2Impl : std::false_type
    {

    };

    template<typename T>
    struct IsVector2Impl<eVector2<T>> : std::true_type
    {

    };

} // namespace Detail

template <typename T>
const char* eVector2<T>::GetStaticTypeName()
{
    return Detail::RawComponentTypeToVector2TypeName<T>::GetName();
}

template <typename T>
eVector2<T>::eVector2(const Vector2<T> &value)
    : eVector2(eNumber<T>(value.x), eNumber<T>(value.y))
{

}

template <typename T>
eVector2<T>::eVector2(const ComponentType& x, const ComponentType& y)
    : eVector2()
{
    this->x = x;
    this->y = y;
}

template <typename T>
eVector2<T>::eVector2(const ComponentType& xy)
    : eVector2(xy, xy)
{
    
}

template <typename T>
eVector2<T>::eVector2(const eVector2& other)
    : eVector2(other.x, other.y)
{
    
}

template <typename T>
eVector2<T>& eVector2<T>::operator=(const eVector2& rhs) &
{
    x = rhs.x;
    y = rhs.y;
    return *this;
}

template <typename T>
eVector2<T>& eVector2<T>::operator=(const ComponentType& xy)
{
    *this = eVector2(xy);
    return *this;
}

template <typename T>
template <typename U>
eVector2<T>::eVector2(const eVector2<U>& other)
    : eVector2()
{
    x = eNumber<T>(other.x);
    y = eNumber<T>(other.y);
    return *this;
}

template <typename T>
const std::string& eVector2<T>::Compile() const
{
    return eVariableCommonBase::eVariable_GetFullName();
}

inline eNumber<bool> any(const eVector2<bool> &v)
{
    return v.x | v.y;
}

inline eNumber<bool> all(const eVector2<bool> &v)
{
    return v.x & v.y;
}

template<typename T>
eVector2<T> operator+(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template<typename T>
eVector2<T> operator-(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template<typename T>
eVector2<T> operator*(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template<typename T>
eVector2<T> operator/(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template<typename T>
eVector2<T> operator&(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x & rhs.x, lhs.y & rhs.y);
}

template<typename T>
eVector2<T> operator|(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x | rhs.x, lhs.y | rhs.y);
}

template<typename T>
eVector2<T> operator^(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<T>(lhs.x ^ rhs.x, lhs.y ^ rhs.y);
}

template<typename T>
eVector2<bool> operator==(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x == rhs.x, lhs.y == rhs.y);
}

template<typename T>
eVector2<bool> operator!=(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x != rhs.x, lhs.y != rhs.y);
}

template<typename T>
eVector2<bool> operator<(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x < rhs.x, lhs.y < rhs.y);
}

template<typename T>
eVector2<bool> operator<=(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x <= rhs.x, lhs.y <= rhs.y);
}

template<typename T>
eVector2<bool> operator>(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x > rhs.x, lhs.y > rhs.y);
}

template<typename T>
eVector2<bool> operator>=(const eVector2<T> &lhs, const eVector2<T> &rhs)
{
    return eVector2<bool>(lhs.x >= rhs.x, lhs.y >= rhs.y);
}

RTRC_EDSL_END
