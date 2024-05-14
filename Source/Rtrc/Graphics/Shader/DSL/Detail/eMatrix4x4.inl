#pragma once

#include <Rtrc/Core/Math/Matrix4x4.h>

#include "eMatrix4x4.h"

RTRC_EDSL_BEGIN

inline float4x4::float4x4(const Matrix4x4f& value)
    : float4x4(
        value[0][0], value[0][1], value[0][2], value[0][3],
        value[1][0], value[1][1], value[1][2], value[1][3],
        value[2][0], value[2][1], value[2][2], value[2][3],
        value[3][0], value[3][1], value[3][2], value[3][3])
{
    
}

inline float4x4::float4x4(
    float m00, float m01, float m02, float m03,
    float m10, float m11, float m12, float m13,
    float m30, float m31, float m32, float m33,
    float m20, float m21, float m22, float m23)
    : float4x4(
        f32(m00), f32(m01), f32(m02), f32(m03),
        f32(m10), f32(m11), f32(m12), f32(m13),
        f32(m20), f32(m21), f32(m22), f32(m23),
        f32(m30), f32(m31), f32(m32), f32(m33))
{
    
}

inline float4x4::float4x4(
    const f32& m00, const f32& m01, const f32& m02, const f32& m03,
    const f32& m10, const f32& m11, const f32& m12, const f32& m13,
    const f32& m30, const f32& m31, const f32& m32, const f32& m33,
    const f32& m20, const f32& m21, const f32& m22, const f32& m23)
    : float4x4()
{
    *this = CreateTemporaryVariableForExpression<float4x4>(
        fmt::format("float4x4({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})",
                    m00.Compile(), m01.Compile(), m02.Compile(), m03.Compile(),
                    m10.Compile(), m11.Compile(), m12.Compile(), m13.Compile(),
                    m20.Compile(), m21.Compile(), m22.Compile(), m23.Compile(),
                    m30.Compile(), m31.Compile(), m32.Compile(), m33.Compile()));
}

inline float4x4::float4x4(const float4& row0, const float4& row1, const float4& row2, const float4& row3)
    : float4x4(
        row0[0], row0[1], row0[2], row0[3],
        row1[0], row1[1], row1[2], row1[3],
        row2[0], row2[1], row2[2], row2[3],
        row3[0], row3[1], row3[2], row3[3])
{
    
}

inline float4x4& float4x4::operator=(const float4x4& other)
{
    static_cast<eVariable<float4x4> &>(*this) = other;
    PopCopyParentVariable();
    return *this;
}

inline TemporaryValueWrapper<float4> float4x4::operator[](const u32& index)
{
    return CreateTemporaryVariableForExpression<float4>(fmt::format("{}[{}]", Compile(), index.Compile()));
}

inline const TemporaryValueWrapper<float4> float4x4::operator[](const u32& index) const
{
    return CreateTemporaryVariableForExpression<float4>(fmt::format("{}[{}]", Compile(), index.Compile()));
}

inline float4x4 mul(const float4x4 &lhs, const float4x4 &rhs)
{
    return CreateTemporaryVariableForExpression<float4x4>(fmt::format(
        "mul({}, {})", lhs.Compile(), rhs.Compile()));
}

inline float4 mul(const float4x4 &lhs, const float4 &rhs)
{
    return CreateTemporaryVariableForExpression<float4>(fmt::format(
        "mul({}, {})", lhs.Compile(), rhs.Compile()));
}

float4 mul(const float4 &lhs, const float4x4 &rhs)
{
    return CreateTemporaryVariableForExpression<float4>(fmt::format(
        "mul({}, {})", lhs.Compile(), rhs.Compile()));
}

float4x4 transpose(const float4x4 &m)
{
    return CreateTemporaryVariableForExpression<float4x4>(fmt::format("transpose({})", m.Compile()));
}

RTRC_EDSL_END
