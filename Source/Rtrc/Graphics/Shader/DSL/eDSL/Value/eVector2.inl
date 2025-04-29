#pragma once

#include <Rtrc/Core/Math/Vector.h>

#include "eVector2.h"

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
eVector2<T>& eVector2<T>::operator=(const eVector2& rhs)
{
    static_cast<eVariable<eVector2> &>(*this) = rhs; PopCopyParentVariable(); return *this;
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
    *this = other;
}

template <typename T>
template <typename U>
eVector2<T>& eVector2<T>::operator=(const eVector2<U>& other)
{
    return *this = CreateTemporaryVariableForExpression<eVector2>(
        std::format("{}({})", GetStaticTypeName(), other.Compile()));
}

template <typename T>
TemporaryValueWrapper<typename eVector2<T>::ComponentType> eVector2<T>::operator[](const u32 &index)
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

template <typename T>
const TemporaryValueWrapper<typename eVector2<T>::ComponentType> eVector2<T>::operator[](const u32 &index) const
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

RTRC_EDSL_END
