#pragma once

#include <Rtrc/Core/Math/Vector.h>

#include "eVector4.h"

RTRC_EDSL_BEGIN

namespace Detail
{

    template<typename T>
    struct RawComponentTypeToVector4TypeName;

    template<>
    struct RawComponentTypeToVector4TypeName<uint32_t>
    {
        static constexpr const char *GetName() { return "uint4"; }
    };

    template<>
    struct RawComponentTypeToVector4TypeName<int32_t>
    {
        static constexpr const char *GetName() { return "int4"; }
    };

    template<>
    struct RawComponentTypeToVector4TypeName<float>
    {
        static constexpr const char *GetName() { return "float4"; }
    };

    template<>
    struct RawComponentTypeToVector4TypeName<bool>
    {
        static constexpr const char *GetName() { return "bool4"; }
    };

    template<typename T>
    struct IsVector4Impl : std::false_type
    {

    };

    template<typename T>
    struct IsVector4Impl<eVector4<T>> : std::true_type
    {

    };

} // namespace Detail

template <typename T>
const char* eVector4<T>::GetStaticTypeName()
{
    return Detail::RawComponentTypeToVector4TypeName<T>::GetName();
}

template <typename T>
eVector4<T>::eVector4(const Vector4<T> &value)
    : eVector4(eNumber<T>(value.x), eNumber<T>(value.y), eNumber<T>(value.z), eNumber<T>(value.w))
{

}

template <typename T>
eVector4<T>::eVector4(const ComponentType &x, const ComponentType &y, const ComponentType &z, const ComponentType &w)
    : eVector4()
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

template <typename T>
eVector4<T>::eVector4(const ComponentType& xyzw)
    : eVector4(xyzw, xyzw, xyzw, xyzw)
{
    
}

template <typename T>
eVector4<T>& eVector4<T>::operator=(const eVector4& rhs)
{
    static_cast<eVariable<eVector4> &>(*this) = rhs; PopCopyParentVariable(); return *this;
}

template <typename T>
eVector4<T>& eVector4<T>::operator=(const ComponentType &xyzw)
{
    return *this = eVector4(xyzw);
}

template <typename T>
template <typename U>
eVector4<T>::eVector4(const eVector4<U>& other)
    : eVector4()
{
    *this = other;
}

template <typename T>
template <typename U>
eVector4<T>& eVector4<T>::operator=(const eVector4<U>& other)
{
    return *this = CreateTemporaryVariableForExpression<eVector4>(
        std::format("{}({})", GetStaticTypeName(), other.Compile()));
}

template <typename T>
TemporaryValueWrapper<typename eVector4<T>::ComponentType> eVector4<T>::operator[](u32 index)
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

template <typename T>
const TemporaryValueWrapper<typename eVector4<T>::ComponentType> eVector4<T>::operator[](u32 index) const
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

template <typename T>
std::string eVector4<T>::Compile() const
{
    return eVariableCommonBase::eVariable_GetFullName();
}

inline eNumber<bool> any(const eVector4<bool> &v)
{
    return v.x | v.y | v.z | v.w;
}

inline eNumber<bool> all(const eVector4<bool> &v)
{
    return v.x & v.y & v.z & v.w;
}

RTRC_EDSL_END
