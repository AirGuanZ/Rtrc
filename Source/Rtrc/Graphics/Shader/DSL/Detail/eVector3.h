#pragma once

#include "eVector2.h"

RTRC_BEGIN

template<typename T>
class Vector3;

RTRC_END

RTRC_EDSL_BEGIN

template<typename T>
struct eVector3 : eVariable<eVector3<T>>
{
    using NativeComponentType = T;
    using ComponentType = eNumber<T>;

    static const char *GetStaticTypeName();

    eVector3() { PopConstructParentVariable(); }

    eVector3(const Vector3<T> &value);
    eVector3(const ComponentType &x, const ComponentType &y, const ComponentType &z);
    explicit eVector3(const ComponentType &xyz);

    eVector3(const eVector3 &other) : eVariable<eVector3>(other) { PopConstructParentVariable(); }
    eVector3 &operator=(const eVector3 &rhs) { static_cast<eVariable<eVector3> &>(*this) = rhs; PopCopyParentVariable(); return *this; }

    eVector3 &operator=(const ComponentType &xyz);

    template<typename U>
    explicit eVector3(const eVector3<U> &other);
    template<typename U>
    eVector3 &operator=(const eVector3<U> &other);

          ComponentType operator[](u32 index);
    const ComponentType operator[](u32 index) const;

    std::string Compile() const;

    [[no_unique_address]] MemberVariableNameInitializer<"x"> _rtrc_init_x; eNumber<T> x;
    [[no_unique_address]] MemberVariableNameInitializer<"y"> _rtrc_init_y; eNumber<T> y;
    [[no_unique_address]] MemberVariableNameInitializer<"z"> _rtrc_init_z; eNumber<T> z;

    [[no_unique_address]] MemberVariableNameInitializer<"r"> _rtrc_init_r; eNumber<T> r;
    [[no_unique_address]] MemberVariableNameInitializer<"g"> _rtrc_init_g; eNumber<T> g;
    [[no_unique_address]] MemberVariableNameInitializer<"b"> _rtrc_init_b; eNumber<T> b;
};

using uint3  = eVector3<uint32_t>;
using int3   = eVector3<int32_t>;
using float3 = eVector3<float>;
using bool3  = eVector3<bool>;

eNumber<bool> any(const eVector3<bool> &v);
eNumber<bool> all(const eVector3<bool> &v);

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, +)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, -)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, *)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, /)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, &)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, |)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, T, ^)

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, ==)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, !=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, <)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, <=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, >)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector3, bool, >=)

RTRC_EDSL_END
