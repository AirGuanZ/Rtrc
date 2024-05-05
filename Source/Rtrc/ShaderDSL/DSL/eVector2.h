#pragma once

#include <Rtrc/ShaderDSL/DSL/eNumber.h>

RTRC_BEGIN

template<typename T>
class Vector2;

RTRC_END

RTRC_EDSL_BEGIN

template<typename T, int...Is>
struct eVectorSwizzleProxy
{
    
};

template<typename T>
struct eVector2 : eVariable<eVector2<T>>
{
    using NativeComponentType = T;
    using ComponentType = eNumber<T>;

    static const char *GetStaticTypeName();

    eVector2() { PopParentVariable(); }

    eVector2(const Vector2<T> &value);
    eVector2(const ComponentType &x, const ComponentType &y);
    explicit eVector2(const ComponentType &xy);

    eVector2(const eVector2 &other);
    eVector2 &operator=(const eVector2 &rhs) &;
    eVector2 &operator=(const eVector2 &rhs) && = delete;

    eVector2 &operator=(const ComponentType &xy);

    template<typename U>
    explicit eVector2(const eVector2<U> &other);
    
    const std::string &Compile() const;

    [[no_unique_address]] MemberVariableNameInitializer<"x"> _rtrc_init_x; eNumber<T> x;
    [[no_unique_address]] MemberVariableNameInitializer<"y"> _rtrc_init_y; eNumber<T> y;
    [[no_unique_address]] MemberVariableNameInitializer<"z"> _rtrc_init_z; eNumber<T> z;
};

using uint2 = eVector2<uint32_t>;
using int2 = eVector2<int32_t>;
using float2 = eVector2<float>;
using bool2 = eVector2<bool>;

eNumber<bool> any(const eVector2<bool> &v);
eNumber<bool> all(const eVector2<bool> &v);

template<typename T>
eVector2<T> operator+(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<T> operator-(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<T> operator*(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<T> operator/(const eVector2<T> &lhs, const eVector2<T> &rhs);

template<typename T>
eVector2<T> operator&(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<T> operator|(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<T> operator^(const eVector2<T> &lhs, const eVector2<T> &rhs);

template<typename T>
eVector2<bool> operator==(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<bool> operator!=(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<bool> operator<(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<bool> operator<=(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<bool> operator>(const eVector2<T> &lhs, const eVector2<T> &rhs);
template<typename T>
eVector2<bool> operator>=(const eVector2<T> &lhs, const eVector2<T> &rhs);

RTRC_EDSL_END
