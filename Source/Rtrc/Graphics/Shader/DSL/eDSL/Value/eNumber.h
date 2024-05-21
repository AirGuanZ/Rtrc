#pragma once

#include "../eVariable.h"

RTRC_EDSL_BEGIN

template<typename T>
struct eNumber : eVariable<eNumber<T>>
{
    using NativeType = T;

    static const char *GetStaticTypeName();

    eNumber() { PopConstructParentVariable(); }

    eNumber(T value);

    eNumber(const eNumber &other) : eVariable<eNumber>(other) { PopConstructParentVariable(); }
    eNumber &operator=(const eNumber &rhs) { static_cast<eVariable<eNumber> &>(*this) = rhs; PopCopyParentVariable(); return *this; }

    template<typename U>
    eNumber(const eNumber<U> &other);
    template<typename U>
    eNumber &operator=(const eNumber<U> &rhs);

    std::string Compile() const;
};

using u32     = eNumber<uint32_t>;
using i32     = eNumber<int32_t>;
using f32     = eNumber<float>;
using boolean = eNumber<bool>;

template<typename T>
eNumber<T> operator+(const eNumber<T> &lhs, const eNumber<T> &rhs);
template<typename T>
eNumber<T> operator-(const eNumber<T> &lhs, const eNumber<T> &rhs);
template<typename T>
eNumber<T> operator*(const eNumber<T> &lhs, const eNumber<T> &rhs);
template<typename T>
eNumber<T> operator/(const eNumber<T> &lhs, const eNumber<T> &rhs);

template<typename T>
eNumber<T> operator&(const eNumber<T> &lhs, const eNumber<T> &rhs);
template<typename T>
eNumber<T> operator|(const eNumber<T> &lhs, const eNumber<T> &rhs);
template<typename T>
eNumber<T> operator^(const eNumber<T> &lhs, const eNumber<T> &rhs);

#define RTRC_BINARY_ARITHMETIC_OPERATOR(SYMBOL)                                                                       \
    template<typename T>                                                                                              \
    eNumber<T> operator SYMBOL(const eNumber<T> &lhs, const eNumber<T> &rhs)                                          \
    {                                                                                                                 \
        eNumber<T> ret;                                                                                               \
        GetCurrentRecordContext().AppendLine("{} = {} " #SYMBOL " {};", ret.Compile(), lhs.Compile(), rhs.Compile()); \
        return ret;                                                                                                   \
    }                                                                                                                 \
    template<typename T>                                                                                              \
    eNumber<T> operator SYMBOL(const eNumber<T> &lhs, T rhs)                                                          \
    {                                                                                                                 \
        return lhs SYMBOL eNumber<T>(rhs);                                                                            \
    }                                                                                                                 \
    template<typename T>                                                                                              \
    eNumber<T> operator SYMBOL(T lhs, const eNumber<T> &rhs)                                                          \
    {                                                                                                                 \
        return eNumber<T>(lhs) SYMBOL rhs;                                                                            \
    }

RTRC_BINARY_ARITHMETIC_OPERATOR(+)
RTRC_BINARY_ARITHMETIC_OPERATOR(-)
RTRC_BINARY_ARITHMETIC_OPERATOR(*)
RTRC_BINARY_ARITHMETIC_OPERATOR(/)
RTRC_BINARY_ARITHMETIC_OPERATOR(&)
RTRC_BINARY_ARITHMETIC_OPERATOR(|)
RTRC_BINARY_ARITHMETIC_OPERATOR(^)

#undef RTRC_BINARY_ARITHMETIC_OPERATOR

#define RTRC_BINARY_COMPARE_OPERATOR(SYMBOL)                                                                          \
    template<typename T>                                                                                              \
    eNumber<bool> operator SYMBOL(const eNumber<T> &lhs, const eNumber<T> &rhs)                                       \
    {                                                                                                                 \
        eNumber<bool> ret;                                                                                            \
        GetCurrentRecordContext().AppendLine("{} = {} " #SYMBOL " {};", ret.Compile(), lhs.Compile(), rhs.Compile()); \
        return ret;                                                                                                   \
    }                                                                                                                 \
    template<typename T>                                                                                              \
    eNumber<bool> operator SYMBOL(const eNumber<T> &lhs, T rhs)                                                       \
    {                                                                                                                 \
        return lhs SYMBOL eNumber<T>(rhs);                                                                            \
    }                                                                                                                 \
    template<typename T>                                                                                              \
    eNumber<bool> operator SYMBOL(T lhs, const eNumber<T> &rhs)                                                       \
    {                                                                                                                 \
        return eNumber<T>(lhs) SYMBOL rhs;                                                                            \
    }

RTRC_BINARY_COMPARE_OPERATOR(==)
RTRC_BINARY_COMPARE_OPERATOR(!=)
RTRC_BINARY_COMPARE_OPERATOR(<)
RTRC_BINARY_COMPARE_OPERATOR(<=)
RTRC_BINARY_COMPARE_OPERATOR(>)
RTRC_BINARY_COMPARE_OPERATOR(>=)

#undef RTRC_BINARY_COMPARE_OPERATOR

RTRC_EDSL_END
