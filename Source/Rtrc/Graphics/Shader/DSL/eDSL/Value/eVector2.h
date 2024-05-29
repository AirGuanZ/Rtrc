#pragma once

#include "eNumber.h"
#include "eVectorSwizzle.h"

RTRC_BEGIN

template<typename T>
class Vector2;

RTRC_END

RTRC_EDSL_BEGIN

template<typename T>
struct eVector2 : eVariable<eVector2<T>>
{
    using NativeComponentType = T;
    using ComponentType = eNumber<T>;

    static const char *GetStaticTypeName();

    eVector2() { PopConstructParentVariable(); }

    eVector2(const Vector2<T> &value);
    eVector2(const ComponentType &x, const ComponentType &y);
    explicit eVector2(const ComponentType &xy);

    eVector2(const eVector2 &other) : eVariable<eVector2>(other) { PopConstructParentVariable(); }
    eVector2 &operator=(const eVector2 &rhs);

    eVector2 &operator=(const ComponentType &xy);

    template<typename U>
    explicit eVector2(const eVector2<U> &other);
    template<typename U>
    eVector2 &operator=(const eVector2<U> &other);

          TemporaryValueWrapper<ComponentType> operator[](const u32 &index);
    const TemporaryValueWrapper<ComponentType> operator[](const u32 &index) const;
    
    std::string Compile() const { return eVariableCommonBase::eVariable_GetFullName(); }

    union
    {
        eVector2 *_vec = this;

#define VECTOR_SWIZZLE_PERMUTATION_2_2(I0, I1, NAME) \
    eVectorSwizzle<eVector2<T>, eVector2<T>, I0, I1, -1, -1> NAME;
#define VECTOR_SWIZZLE_PERMUTATION_2_3(I0, I1, I2, NAME) \
    eVectorSwizzle<eVector2<T>, eVector3<T>, I0, I1, I2, -1> NAME;
#define VECTOR_SWIZZLE_PERMUTATION_2_4(I0, I1, I2, I3, NAME) \
    eVectorSwizzle<eVector2<T>, eVector4<T>, I0, I1, I2, I3> NAME;

#include "eVectorSwizzlePermutation.txt"

#undef VECTOR_SWIZZLE_PERMUTATION_2_2
#undef VECTOR_SWIZZLE_PERMUTATION_2_3
#undef VECTOR_SWIZZLE_PERMUTATION_2_4
    };

    [[no_unique_address]] MemberVariableNameInitializer<"x"> _rtrc_init_x; eNumber<T> x;
    [[no_unique_address]] MemberVariableNameInitializer<"y"> _rtrc_init_y; eNumber<T> y;

    [[no_unique_address]] MemberVariableNameInitializer<"r"> _rtrc_init_r; eNumber<T> r;
    [[no_unique_address]] MemberVariableNameInitializer<"g"> _rtrc_init_g; eNumber<T> g;
};

using u32x2  = eVector2<uint32_t>;
using i32x2  = eVector2<int32_t>;
using f32x2  = eVector2<float>;
using boolx2 = eVector2<bool>;

using euint2  = u32x2;
using eint2   = i32x2;
using efloat2 = f32x2;
using ebool2  = boolx2;

inline eNumber<bool> any(const eVector2<bool> &v) { return v.x | v.y; }
inline eNumber<bool> all(const eVector2<bool> &v) { return v.x & v.y; }

#define RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(VEC, RET, OPR)              \
    template<typename T>                                                       \
    VEC<RET> operator OPR(const VEC<T> &lhs, const VEC<T> &rhs)                \
    {                                                                          \
        return CreateTemporaryVariableForExpression<VEC<RET>>(                 \
            fmt::format("({} " #OPR " {})", lhs.Compile(), rhs.Compile()));    \
    }                                                                          \
    template<typename T>                                                       \
    VEC<RET> operator OPR(const VEC<T> &lhs, const eNumber<T> &rhs)            \
    {                                                                          \
        return lhs OPR VEC<T>(rhs);                                            \
    }                                                                          \
    template<typename T>                                                       \
    VEC<RET> operator OPR(const eNumber<T> &lhs, const VEC<T> &rhs)            \
    {                                                                          \
        return VEC<T>(lhs) OPR rhs;                                            \
    }

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, +)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, -)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, *)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, /)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, &)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, |)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, T, ^)

RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, ==)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, !=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, <)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, <=)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, >)
RTRC_EDSL_ADD_ELEMENT_WISE_VECTOR_OPERATOR(eVector2, bool, >=)

RTRC_EDSL_END
