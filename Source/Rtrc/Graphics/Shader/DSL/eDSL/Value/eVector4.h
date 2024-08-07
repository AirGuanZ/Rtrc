#pragma once

#include "eVector3.h"

RTRC_BEGIN

template<typename T>
class Vector3;

RTRC_END

RTRC_EDSL_BEGIN

template<typename T>
struct eVector4 : eVariable<eVector4<T>>
{
    using NativeComponentType = T;
    using ComponentType = eNumber<T>;

    static const char *GetStaticTypeName();

    eVector4() { PopConstructParentVariable(); }

    eVector4(const Vector4<T> &value);
    eVector4(const ComponentType &x, const ComponentType &y, const ComponentType &z, const ComponentType &w);
    explicit eVector4(const ComponentType &xyzw);

    eVector4(const eVector4 &other) : eVariable<eVector4>(other) { PopConstructParentVariable(); }
    eVector4 &operator=(const eVector4 &rhs);

    eVector4 &operator=(const ComponentType &xyzw);

    template<typename U>
    explicit eVector4(const eVector4<U> &other);
    template<typename U>
    eVector4 &operator=(const eVector4<U> &other);

          TemporaryValueWrapper<ComponentType> operator[](u32 index);
    const TemporaryValueWrapper<ComponentType> operator[](u32 index) const;

    std::string Compile() const;

    union
    {
        eVector4 *_vec = this;

#define VECTOR_SWIZZLE_PERMUTATION_4_2(I0, I1, NAME) \
    eVectorSwizzle<eVector4<T>, eVector2<T>, I0, I1, -1, -1> NAME;
#define VECTOR_SWIZZLE_PERMUTATION_4_3(I0, I1, I2, NAME) \
    eVectorSwizzle<eVector4<T>, eVector3<T>, I0, I1, I2, -1> NAME;
#define VECTOR_SWIZZLE_PERMUTATION_4_4(I0, I1, I2, I3, NAME) \
    eVectorSwizzle<eVector4<T>, eVector4<T>, I0, I1, I2, I3> NAME;

#include "eVectorSwizzlePermutation.txt"

#undef VECTOR_SWIZZLE_PERMUTATION_4_2
#undef VECTOR_SWIZZLE_PERMUTATION_4_3
#undef VECTOR_SWIZZLE_PERMUTATION_4_4
    };

    [[no_unique_address]] MemberVariableNameInitializer<"x"> _rtrc_init_x; eNumber<T> x;
    [[no_unique_address]] MemberVariableNameInitializer<"y"> _rtrc_init_y; eNumber<T> y;
    [[no_unique_address]] MemberVariableNameInitializer<"z"> _rtrc_init_z; eNumber<T> z;
    [[no_unique_address]] MemberVariableNameInitializer<"w"> _rtrc_init_w; eNumber<T> w;

    [[no_unique_address]] MemberVariableNameInitializer<"r"> _rtrc_init_r; eNumber<T> r;
    [[no_unique_address]] MemberVariableNameInitializer<"g"> _rtrc_init_g; eNumber<T> g;
    [[no_unique_address]] MemberVariableNameInitializer<"b"> _rtrc_init_b; eNumber<T> b;
    [[no_unique_address]] MemberVariableNameInitializer<"a"> _rtrc_init_a; eNumber<T> a;
};

using u32x4  = eVector4<uint32_t>;
using i32x4  = eVector4<int32_t>;
using f32x4  = eVector4<float>;
using boolx4 = eVector4<bool>;

using euint4  = u32x4;
using eint4   = i32x4;
using efloat4 = f32x4;
using ebool4  = boolx4;

eNumber<bool> any(const eVector4<bool> &v);
eNumber<bool> all(const eVector4<bool> &v);

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, +)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, -)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, *)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, /)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, &)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, |)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, T, ^)

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, ==)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, !=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, <)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, <=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, >)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector4, bool, >=)

RTRC_EDSL_END
