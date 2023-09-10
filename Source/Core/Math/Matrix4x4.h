#pragma once

#include <cassert>

#include <Core/Math/Matrix3x3.h>
#include <Core/Math/Vector3.h>
#include <Core/Math/Vector4.h>

RTRC_BEGIN

// Row-major 4x4 matrix
class Matrix4x4f
{
public:

    Vector4f rows[4];

    static const Matrix4x4f &Zero();
    static const Matrix4x4f &One();
    static const Matrix4x4f &Identity();

    static Matrix4x4f Diag(float v);
    static Matrix4x4f Fill(float v);

    static Matrix4x4f FromCols(
        const Vector4f &col1,
        const Vector4f &col2,
        const Vector4f &col3,
        const Vector4f &col4);
    static Matrix4x4f FromRows(
        const Vector4f &row1,
        const Vector4f &row2,
        const Vector4f &row3,
        const Vector4f &row4);

    static Matrix4x4f Translate(float x, float y, float z);
    static Matrix4x4f Translate(const Vector3f &offset);

    static Matrix4x4f Rotate(const Vector3f &axis, float rad);
    static Matrix4x4f RotateX(float rad);
    static Matrix4x4f RotateY(float rad);
    static Matrix4x4f RotateZ(float rad);

    static Matrix4x4f Scale(float x, float y, float z);
    static Matrix4x4f Scale(const Vector3f &scale);

    static Matrix4x4f LookAt(const Vector3f &eye, const Vector3f &dst, const Vector3f &up);
    static Matrix4x4f Perspective(float fovYRad, float wOverH, float nearPlane, float farPlane);

    Matrix4x4f();

    explicit Matrix4x4f(const Matrix3x3f &m3x3);

    Matrix4x4f(
        float m11, float m12, float m13, float m14,
        float m21, float m22, float m23, float m24,
        float m31, float m32, float m33, float m34,
        float m41, float m42, float m43, float m44);
    
    float  At(int row, int col) const; // row and col start from 1
    float &At(int row, int col);      // row and col start from 1

    const Vector4f &GetRow(int rowMinusOne) const;
    Vector4f        GetCol(int colMinusOne) const;

    const Vector4f &operator[](int rowMinusOne) const;
    Vector4f       &operator[](int rowMinusOne);

    Matrix4x4f &operator+=(const Matrix4x4f &b);
    Matrix4x4f &operator-=(const Matrix4x4f &b);
    Matrix4x4f &operator*=(const Matrix4x4f &b);
};

Matrix4x4f operator+(const Matrix4x4f &a, const Matrix4x4f &b);
Matrix4x4f operator-(const Matrix4x4f &a, const Matrix4x4f &b);
Matrix4x4f operator*(const Matrix4x4f &a, const Matrix4x4f &b);

Matrix4x4f operator*(float a, const Matrix4x4f &b);
Matrix4x4f operator*(const Matrix4x4f &a, float b);
Matrix4x4f operator/(const Matrix4x4f &a, float b);

// b: column vector
Vector4f operator*(const Matrix4x4f &a, const Vector4f &b);
// a: row vector
Vector4f operator*(const Vector4f &a, const Matrix4x4f &b);

Matrix4x4f ElementWiseMultiply(const Matrix4x4f &a, const Matrix4x4f &b);

Matrix4x4f Transpose(const Matrix4x4f &m);
Matrix4x4f Inverse(const Matrix4x4f &m);
Matrix4x4f Adjoint(const Matrix4x4f &m);

inline const Matrix4x4f &Matrix4x4f::Zero()
{
    static const Matrix4x4f ret = Fill(0);
    return ret;
}

inline const Matrix4x4f &Matrix4x4f::One()
{
    static const Matrix4x4f ret = Fill(1);
    return ret;
}

inline const Matrix4x4f &Matrix4x4f::Identity()
{
    static const Matrix4x4f ret = Diag(1);
    return ret;
}

inline Matrix4x4f Matrix4x4f::Diag(float v)
{
    return {
        v, 0, 0, 0,
        0, v, 0, 0,
        0, 0, v, 0,
        0, 0, 0, v
    };
}

inline Matrix4x4f Matrix4x4f::Fill(float v)
{
    return { v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

inline Matrix4x4f Matrix4x4f::FromCols(
    const Vector4f &col1, const Vector4f &col2, const Vector4f &col3, const Vector4f &col4)
{
    return {
        col1.x, col2.x, col3.x, col4.x,
        col1.y, col2.y, col3.y, col4.y,
        col1.z, col2.z, col3.z, col4.z,
        col1.w, col2.w, col3.w, col4.w
    };
}

inline Matrix4x4f Matrix4x4f::FromRows(
    const Vector4f &row1, const Vector4f &row2, const Vector4f &row3, const Vector4f &row4)
{
    Matrix4x4f ret;
    ret.rows[0] = row1;
    ret.rows[1] = row2;
    ret.rows[2] = row3;
    ret.rows[3] = row4;
    return ret;
}

inline Matrix4x4f Matrix4x4f::Translate(float x, float y, float z)
{
    return {
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Matrix4x4f::Translate(const Vector3f &offset)
{
    return Translate(offset.x, offset.y, offset.z);
}

inline Matrix4x4f Matrix4x4f::Rotate(const Vector3f &_axis, float rad)
{
    const auto axis = Normalize(_axis);
    const float sinv = std::sin(rad), cosv = std::cos(rad);

    Matrix4x4f ret;

    ret.rows[0][0] = axis.x * axis.x + (1 - axis.x * axis.x) * cosv;
    ret.rows[1][0] = axis.x * axis.y * (1 - cosv) - axis.z * sinv;
    ret.rows[2][0] = axis.x * axis.z * (1 - cosv) + axis.y * sinv;
    ret.rows[3][0] = 0;

    ret.rows[0][1] = axis.x * axis.y * (1 - cosv) + axis.z * sinv;
    ret.rows[1][1] = axis.y * axis.y + (1 - axis.y * axis.y) * cosv;
    ret.rows[2][1] = axis.y * axis.z * (1 - cosv) - axis.x * sinv;
    ret.rows[3][1] = 0;

    ret.rows[0][2] = axis.x * axis.z * (1 - cosv) - axis.y * sinv;
    ret.rows[1][2] = axis.y * axis.z * (1 - cosv) + axis.x * sinv;
    ret.rows[2][2] = axis.z * axis.z + (1 - axis.z * axis.z) * cosv;
    ret.rows[3][2] = 0;

    ret.rows[0][3] = 0;
    ret.rows[1][3] = 0;
    ret.rows[2][3] = 0;
    ret.rows[3][3] = 1;

    return ret;
}

inline Matrix4x4f Matrix4x4f::RotateX(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        1, 0, 0, 0,
        0, C, -S, 0,
        0, S, C, 0,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Matrix4x4f::RotateY(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, 0, S, 0,
        0, 1, 0, 0,
        -S, 0, C, 0,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Matrix4x4f::RotateZ(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, -S, 0, 0,
        S, C, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Matrix4x4f::Scale(float x, float y, float z)
{
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Matrix4x4f::Scale(const Vector3f &scale)
{
    return Scale(scale.x, scale.y, scale.z);
}

inline Matrix4x4f Matrix4x4f::LookAt(const Vector3f &eye, const Vector3f &dst, const Vector3f &up)
{
    const Vector3f D = Normalize(dst - eye);
    const Vector3f R = Normalize(Cross(up, D));
    const Vector3f U = Cross(D, R);
    return {
        R.x, R.y, R.z, -Dot(R, eye),
        U.x, U.y, U.z, -Dot(U, eye),
        D.x, D.y, D.z, -Dot(D, eye),
        0,   0,   0,   1
    };
}

inline Matrix4x4f Matrix4x4f::Perspective(float fovYRad, float wOverH, float nearPlane, float farPlane)
{
    const float invDis = 1.0f / (farPlane - nearPlane);
    const float y_rad = 0.5f * fovYRad;
    const float cot = std::cos(y_rad) / std::sin(y_rad);
    return {
        -cot / wOverH, 0,   0,                 0,
        0,             cot, 0,                 0,
        0,             0,   farPlane * invDis, -farPlane * nearPlane * invDis,
        0,             0,   1,                 0
    };
}

inline Matrix4x4f::Matrix4x4f()
{
    *this = Zero();
}

inline Matrix4x4f::Matrix4x4f(const Matrix3x3f &m3x3)
    : Matrix4x4f(m3x3[0][0], m3x3[0][1], m3x3[0][2], 0,
                 m3x3[1][0], m3x3[1][1], m3x3[1][2], 0,
                 m3x3[2][0], m3x3[2][1], m3x3[2][2], 0,
                 0,          0,          0,          1)
{

}

inline Matrix4x4f::Matrix4x4f(
    float m11, float m12, float m13, float m14,
    float m21, float m22, float m23, float m24,
    float m31, float m32, float m33, float m34,
    float m41, float m42, float m43, float m44)
{
    rows[0] = { m11, m12, m13, m14 };
    rows[1] = { m21, m22, m23, m24 };
    rows[2] = { m31, m32, m33, m34 };
    rows[3] = { m41, m42, m43, m44 };
}

inline float Matrix4x4f::At(int row, int col) const
{
    assert(1 <= row && row <= 4);
    assert(1 <= col && col <= 4);
    return rows[row - 1][col - 1];
}

inline float &Matrix4x4f::At(int row, int col)
{
    assert(1 <= row && row <= 4);
    assert(1 <= col && col <= 4);
    return rows[row - 1][col - 1];
}

inline const Vector4f &Matrix4x4f::operator[](int rowMinusOne) const
{
    return rows[rowMinusOne];
}

inline Vector4f &Matrix4x4f::operator[](int rowMinusOne)
{
    return rows[rowMinusOne];
}

inline const Vector4f &Matrix4x4f::GetRow(int rowMinusOne) const
{
    return rows[rowMinusOne];
}

inline Vector4f Matrix4x4f::GetCol(int colMinusOne) const
{
    return { rows[0][colMinusOne], rows[1][colMinusOne], rows[2][colMinusOne], rows[3][colMinusOne] };
}

inline Matrix4x4f &Matrix4x4f::operator+=(const Matrix4x4f &b)
{
    *this = *this + b;
    return *this;
}

inline Matrix4x4f &Matrix4x4f::operator-=(const Matrix4x4f &b)
{
    *this = *this - b;
    return *this;
}

inline Matrix4x4f &Matrix4x4f::operator*=(const Matrix4x4f &b)
{
    *this = *this * b;
    return *this;
}

#define RTRC_CREATE_MATRIX_FROM_ELEM(F)     \
    {                                       \
        F(0, 0), F(0, 1), F(0, 2), F(0, 3), \
        F(1, 0), F(1, 1), F(1, 2), F(1, 3), \
        F(2, 0), F(2, 1), F(2, 2), F(2, 3), \
        F(3, 0), F(3, 1), F(3, 2), F(3, 3)  \
    }

inline Matrix4x4f operator+(const Matrix4x4f &a, const Matrix4x4f &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] + b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix4x4f operator-(const Matrix4x4f &a, const Matrix4x4f &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] - b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix4x4f operator*(const Matrix4x4f &a, const Matrix4x4f &b)
{
    Matrix4x4f ret;
    for(int y = 0; y < 4; ++y)
    {
        for(int x = 0; x < 4; ++x)
        {
            ret[y][x] = a[y][0] * b[0][x]
                      + a[y][1] * b[1][x]
                      + a[y][2] * b[2][x]
                      + a[y][3] * b[3][x];
        }
    }
    return ret;
}

inline Matrix4x4f operator*(float a, const Matrix4x4f &b)
{
    return b * a;
}

inline Matrix4x4f operator*(const Matrix4x4f &a, float b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] * b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix4x4f operator/(const Matrix4x4f &a, float b)
{
    return a * (1.0f / b);
}

inline Vector4f operator*(const Matrix4x4f &a, const Vector4f &b)
{
    Vector4f ret;
    for(int y = 0; y < 4; ++y)
    {
        ret[y] = a[y][0] * b[0]
               + a[y][1] * b[1]
               + a[y][2] * b[2]
               + a[y][3] * b[3];
    }
    return ret;
}

inline Vector4f operator*(const Vector4f &a, const Matrix4x4f &b)
{
    Vector4f ret;
    for(int x = 0; x < 4; ++x)
    {
        ret[x] = a[0] * b[0][x]
               + a[1] * b[1][x]
               + a[2] * b[2][x]
               + a[3] * b[3][x];
    }
    return ret;
}

inline Matrix4x4f ElementWiseMultiply(const Matrix4x4f &a, const Matrix4x4f &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] * b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix4x4f Transpose(const Matrix4x4f &m)
{
#define RTRC_MAT_ELEM(R, C) (m[C][R])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

#undef RTRC_CREATE_MATRIX_FROM_ELEM

inline Matrix4x4f Inverse(const Matrix4x4f &m)
{
    const Matrix4x4f adj = Adjoint(m);
    const float dem = Dot(m.GetCol(0), adj.GetRow(0));
    return adj / dem;
}

inline Matrix4x4f Adjoint(const Matrix4x4f &m)
{
    const float blk00 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    const float blk02 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    const float blk03 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    const float blk04 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
    const float blk06 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
    const float blk07 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
    const float blk08 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
    const float blk10 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
    const float blk11 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    const float blk12 = m[0][2] * m[3][3] - m[0][3] * m[3][2];
    const float blk14 = m[0][1] * m[3][3] - m[0][3] * m[3][1];
    const float blk15 = m[0][1] * m[3][2] - m[0][2] * m[3][1];
    const float blk16 = m[0][2] * m[2][3] - m[0][3] * m[2][2];
    const float blk18 = m[0][1] * m[2][3] - m[0][3] * m[2][1];
    const float blk19 = m[0][1] * m[2][2] - m[0][2] * m[2][1];
    const float blk20 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    const float blk22 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    const float blk23 = m[0][1] * m[1][2] - m[0][2] * m[1][1];

    const Vector4f fac0(blk00, blk00, blk02, blk03);
    const Vector4f fac1(blk04, blk04, blk06, blk07);
    const Vector4f fac2(blk08, blk08, blk10, blk11);
    const Vector4f fac3(blk12, blk12, blk14, blk15);
    const Vector4f fac4(blk16, blk16, blk18, blk19);
    const Vector4f fac5(blk20, blk20, blk22, blk23);

    const Vector4f vec0(m[0][1], m[0][0], m[0][0], m[0][0]);
    const Vector4f vec1(m[1][1], m[1][0], m[1][0], m[1][0]);
    const Vector4f vec2(m[2][1], m[2][0], m[2][0], m[2][0]);
    const Vector4f vec3(m[3][1], m[3][0], m[3][0], m[3][0]);

    const Vector4f inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
    const Vector4f inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
    const Vector4f inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
    const Vector4f inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

    const Vector4f sign_a(+1, -1, +1, -1);
    const Vector4f sign_b(-1, +1, -1, +1);

    return Matrix4x4f::FromCols(inv0 * sign_a, inv1 * sign_b, inv2 * sign_a, inv3 * sign_b);
}

RTRC_END
