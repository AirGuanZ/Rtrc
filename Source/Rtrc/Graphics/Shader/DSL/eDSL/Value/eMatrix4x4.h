#pragma once

#include "eVector4.h"

RTRC_BEGIN

class Matrix4x4f;

RTRC_END

RTRC_EDSL_BEGIN

struct float4x4 : eVariable<float4x4>
{
    static const char *GetStaticTypeName() { return "float4x4"; }

    float4x4() { PopConstructParentVariable(); }

    float4x4(const Matrix4x4f &value);
    float4x4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m30, float m31, float m32, float m33,
        float m20, float m21, float m22, float m23);
    float4x4(
        const f32 &m00, const f32 &m01, const f32 &m02, const f32 &m03,
        const f32 &m10, const f32 &m11, const f32 &m12, const f32 &m13,
        const f32 &m30, const f32 &m31, const f32 &m32, const f32 &m33,
        const f32 &m20, const f32 &m21, const f32 &m22, const f32 &m23);
    float4x4(const f32x4 &row0, const f32x4 &row1, const f32x4 &row2, const f32x4 &row3);

    float4x4 &operator=(const float4x4 &other);
    
          TemporaryValueWrapper<f32x4> operator[](const u32 &index);
    const TemporaryValueWrapper<f32x4> operator[](const u32 &index) const;

    std::string Compile() const { return eVariable_GetFullName(); }
};

inline float4x4 mul(const float4x4 &lhs, const float4x4 &rhs);
inline f32x4 mul(const float4x4 &lhs, const f32x4 &rhs);
inline f32x4 mul(const f32x4 &lhs, const float4x4 &rhs);
inline float4x4 transpose(const float4x4 &m);

#define RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR(OPR)                     \
    inline float4x4 operator OPR(const float4x4 &lhs, const float4x4 &rhs)  \
    {                                                                       \
        return CreateTemporaryVariableForExpression<float4x4>(              \
            fmt::format("({} " #OPR " {})", lhs.Compile(), rhs.Compile())); \
    }                                                                       \
    inline float4x4 operator OPR(const float4x4 &lhs, const f32x4 &rhs)    \
    {                                                                       \
        return CreateTemporaryVariableForExpression<float4x4>(              \
            fmt::format("({} " #OPR " {})", lhs.Compile(), rhs.Compile())); \
    }                                                                       \
    inline float4x4 operator OPR(const f32x4 &lhs, const float4x4 &rhs)    \
    {                                                                       \
        return CreateTemporaryVariableForExpression<float4x4>(              \
            fmt::format("({} " #OPR " {})", lhs.Compile(), rhs.Compile())); \
    }

RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR(+)
RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR(-)
RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR(*)
RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR(/)

#undef RTRC_EDSL_ADD_ELEMENT_WISE_MATRIX_OPERATOR

RTRC_EDSL_END
