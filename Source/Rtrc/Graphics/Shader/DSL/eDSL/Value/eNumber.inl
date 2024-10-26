#pragma once

#include "../RecordContext.h"
#include "eNumber.h"

RTRC_EDSL_BEGIN

namespace Detail
{

    template<typename T>
    struct RawTypeToArithmeticTypeName;

    template<>
    struct RawTypeToArithmeticTypeName<uint32_t>
    {
        static constexpr const char *GetName() { return "uint"; }
    };

    template<>
    struct RawTypeToArithmeticTypeName<int32_t>
    {
        static constexpr const char *GetName() { return "int"; }
    };

    template<>
    struct RawTypeToArithmeticTypeName<float>
    {
        static constexpr const char *GetName() { return "float"; }
    };

    template<>
    struct RawTypeToArithmeticTypeName<bool>
    {
        static constexpr const char *GetName() { return "bool"; }
    };

    template<typename T>
    struct IsArithmeticImpl : std::false_type
    {
        
    };

    template<typename T>
    struct IsArithmeticImpl<eNumber<T>> : std::true_type
    {
        
    };

} // namespace Detail

template <typename T>
const char* eNumber<T>::GetStaticTypeName()
{
    return Detail::RawTypeToArithmeticTypeName<T>::GetName();
}

template <typename T>
eNumber<T>::eNumber(T value)
    : eNumber()
{
    GetCurrentRecordContext().AppendLine("{} = {};", Compile(), value);
}

template <typename T>
template <typename U>
eNumber<T>::eNumber(const eNumber<U>& other)
    : eNumber()
{
    *this = other;
}

template <typename T>
template <typename U>
eNumber<T>& eNumber<T>::operator=(const eNumber<U> &rhs)
{
    eNumber casted = CreateTemporaryVariableForExpression<eNumber>(
        std::format("{}({})", GetStaticTypeName(), rhs.Compile()));
    return *this = casted;
}

template <typename T>
std::string eNumber<T>::Compile() const
{
    return eVariableCommonBase::eVariable_GetFullName();
}

RTRC_EDSL_END
