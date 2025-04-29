#pragma once

#include <Rtrc/Core/Math/Vector.h>

#include "eVector3.h"

RTRC_EDSL_BEGIN

namespace Detail
{

    template<typename T>
    struct RawComponentTypeToVector3TypeName;

    template<>
    struct RawComponentTypeToVector3TypeName<uint32_t>
    {
        static constexpr const char *GetName() { return "uint3"; }
    };

    template<>
    struct RawComponentTypeToVector3TypeName<int32_t>
    {
        static constexpr const char *GetName() { return "int3"; }
    };

    template<>
    struct RawComponentTypeToVector3TypeName<float>
    {
        static constexpr const char *GetName() { return "float3"; }
    };

    template<>
    struct RawComponentTypeToVector3TypeName<bool>
    {
        static constexpr const char *GetName() { return "bool3"; }
    };

    template<typename T>
    struct IsVector3Impl : std::false_type
    {

    };

    template<typename T>
    struct IsVector3Impl<eVector3<T>> : std::true_type
    {

    };

} // namespace Detail

template <typename T>
const char* eVector3<T>::GetStaticTypeName()
{
    return Detail::RawComponentTypeToVector3TypeName<T>::GetName();
}

template <typename T>
eVector3<T>::eVector3(const Vector3<T> &value)
    : eVector3(eNumber<T>(value.x), eNumber<T>(value.y), eNumber<T>(value.z))
{

}

template <typename T>
eVector3<T>::eVector3(const ComponentType &x, const ComponentType &y, const ComponentType &z)
    : eVector3()
{
    this->x = x;
    this->y = y;
    this->z = z;
}

template <typename T>
eVector3<T>::eVector3(const ComponentType& xyz)
    : eVector3(xyz, xyz, xyz)
{
    
}

template <typename T>
eVector3<T>& eVector3<T>::operator=(const eVector3& rhs)
{
    static_cast<eVariable<eVector3> &>(*this) = rhs; PopCopyParentVariable(); return *this;
}

template <typename T>
eVector3<T>& eVector3<T>::operator=(const ComponentType&xyz)
{
    return *this = eVector3(xyz);
}

template <typename T>
template <typename U>
eVector3<T>::eVector3(const eVector3<U>& other)
    : eVector3()
{
    *this = other;
}

template <typename T>
template <typename U>
eVector3<T>& eVector3<T>::operator=(const eVector3<U>& other)
{
    return *this = CreateTemporaryVariableForExpression<eVector3>(
        std::format("{}({})", GetStaticTypeName(), other.Compile()));
}

template <typename T>
TemporaryValueWrapper<typename eVector3<T>::ComponentType> eVector3<T>::operator[](u32 index)
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

template <typename T>
const TemporaryValueWrapper<typename eVector3<T>::ComponentType> eVector3<T>::operator[](u32 index) const
{
    return CreateTemporaryVariableForExpression<ComponentType>(std::format("{}[{}]", Compile(), index.Compile()));
}

template <typename T>
std::string eVector3<T>::Compile() const
{
    return eVariableCommonBase::eVariable_GetFullName();
}

inline eNumber<bool> any(const eVector3<bool> &v)
{
    return v.x | v.y | v.z;
}

inline eNumber<bool> all(const eVector3<bool> &v)
{
    return v.x & v.y & v.z;
}

RTRC_EDSL_END
