#pragma once

#include <cassert>

#include <Rtrc/Core/Math/Matrix3x3.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>

RTRC_BEGIN

// Row-major 4x4 matrix.
// Note that we use the -Zpr option with DXC,
// which allows matrices to be passed directly to the constant buffer without transposition.
template<typename T>
class Matrix4x4
{
public:

    Vector4<T> rows[4];

    static const Matrix4x4 &Zero();
    static const Matrix4x4 &One();
    static const Matrix4x4 &Identity();

    static Matrix4x4 Diag(T v);
    static Matrix4x4 Fill(T v);

    static Matrix4x4 FromCols(
        const Vector4<T> &col1,
        const Vector4<T> &col2,
        const Vector4<T> &col3,
        const Vector4<T> &col4);
    static Matrix4x4 FromRows(
        const Vector4<T> &row1,
        const Vector4<T> &row2,
        const Vector4<T> &row3,
        const Vector4<T> &row4);

    static Matrix4x4 Translate(T x, T y, T z);
    static Matrix4x4 Translate(const Vector3<T> &offset);

    static Matrix4x4 Rotate(const Vector3<T> &axis, T rad);
    static Matrix4x4 RotateX(T rad);
    static Matrix4x4 RotateY(T rad);
    static Matrix4x4 RotateZ(T rad);

    static Matrix4x4 Scale(T x, T y, T z);
    static Matrix4x4 Scale(const Vector3<T> &scale);

    static Matrix4x4 LookAt(const Vector3<T> &eye, const Vector3<T> &dst, const Vector3<T> &up);
    static Matrix4x4 Perspective(T fovYRad, T wOverH, T nearPlane, T farPlane, bool reverseZ = false);

    Matrix4x4();

    explicit Matrix4x4(const Matrix3x3<T> &m3x3);

    Matrix4x4(
        T m11, T m12, T m13, T m14,
        T m21, T m22, T m23, T m24,
        T m31, T m32, T m33, T m34,
        T m41, T m42, T m43, T m44);
    
    T  At(int row, int col) const; // row and col start from 1
    T &At(int row, int col);       // row and col start from 1

    const Vector4<T> &GetRow(int rowMinusOne) const;
    Vector4<T>        GetCol(int colMinusOne) const;

    const Vector4<T> &operator[](int rowMinusOne) const;
    Vector4<T>       &operator[](int rowMinusOne);

    Matrix4x4 &operator+=(const Matrix4x4 &b);
    Matrix4x4 &operator-=(const Matrix4x4 &b);
    Matrix4x4 &operator*=(const Matrix4x4 &b);
};

using Matrix4x4f = Matrix4x4<float>;
using Matrix4x4d = Matrix4x4<double>;

template<typename T>
Matrix4x4<T> operator+(const Matrix4x4<T> &a, const Matrix4x4<T> &b);
template<typename T>
Matrix4x4<T> operator-(const Matrix4x4<T> &a, const Matrix4x4<T> &b);
template<typename T>
Matrix4x4<T> operator*(const Matrix4x4<T> &a, const Matrix4x4<T> &b);

template<typename T>
Matrix4x4<T> operator*(T a, const Matrix4x4<T> &b);
template<typename T>
Matrix4x4<T> operator*(const Matrix4x4<T> &a, T b);
template<typename T>
Matrix4x4<T> operator/(const Matrix4x4<T> &a, T b);

// b: column vector
template<typename T>
Vector4<T> operator*(const Matrix4x4<T> &a, const Vector4<T> &b);
// a: row vector
template<typename T>
Vector4<T> operator*(const Vector4<T> &a, const Matrix4x4<T> &b);

template<typename T>
Matrix4x4<T> ElementWiseMultiply(const Matrix4x4<T> &a, const Matrix4x4<T> &b);

template<typename T>
Matrix4x4<T> Transpose(const Matrix4x4<T> &m);
template<typename T>
Matrix4x4<T> Inverse(const Matrix4x4<T> &m);
template<typename T>
Matrix4x4<T> Adjoint(const Matrix4x4<T> &m);

template<typename T>
const Matrix4x4<T> &Matrix4x4<T>::Zero()
{
    static const Matrix4x4<T> ret = Fill(0);
    return ret;
}

template<typename T>
const Matrix4x4<T> &Matrix4x4<T>::One()
{
    static const Matrix4x4<T> ret = Fill(1);
    return ret;
}

template<typename T>
const Matrix4x4<T> &Matrix4x4<T>::Identity()
{
    static const Matrix4x4<T> ret = Diag(1);
    return ret;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Diag(T v)
{
    return {
        v, 0, 0, 0,
        0, v, 0, 0,
        0, 0, v, 0,
        0, 0, 0, v
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Fill(T v)
{
    return { v, v, v, v, v, v, v, v, v, v, v, v, v, v, v, v };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::FromCols(
    const Vector4<T> &col1, const Vector4<T> &col2, const Vector4<T> &col3, const Vector4<T> &col4)
{
    return {
        col1.x, col2.x, col3.x, col4.x,
        col1.y, col2.y, col3.y, col4.y,
        col1.z, col2.z, col3.z, col4.z,
        col1.w, col2.w, col3.w, col4.w
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::FromRows(
    const Vector4<T> &row1, const Vector4<T> &row2, const Vector4<T> &row3, const Vector4<T> &row4)
{
    Matrix4x4<T> ret;
    ret.rows[0] = row1;
    ret.rows[1] = row2;
    ret.rows[2] = row3;
    ret.rows[3] = row4;
    return ret;
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Translate(T x, T y, T z)
{
    return {
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Translate(const Vector3<T> &offset)
{
    return Translate(offset.x, offset.y, offset.z);
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Rotate(const Vector3<T> &_axis, T rad)
{
    const auto axis = Normalize(_axis);
    const T sinv = std::sin(rad), cosv = std::cos(rad);

    Matrix4x4<T> ret;

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

template<typename T>
Matrix4x4<T> Matrix4x4<T>::RotateX(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        1, 0, 0, 0,
        0, C, -S, 0,
        0, S, C, 0,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::RotateY(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, 0, S, 0,
        0, 1, 0, 0,
        -S, 0, C, 0,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::RotateZ(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, -S, 0, 0,
        S, C, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Scale(T x, T y, T z)
{
    return {
        x, 0, 0, 0,
        0, y, 0, 0,
        0, 0, z, 0,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Scale(const Vector3<T> &scale)
{
    return Scale(scale.x, scale.y, scale.z);
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::LookAt(const Vector3<T> &eye, const Vector3<T> &dst, const Vector3<T> &up)
{
    const Vector3<T> D = Normalize(dst - eye);
    const Vector3<T> R = Normalize(Cross(up, D));
    const Vector3<T> U = Cross(D, R);
    return {
        R.x, R.y, R.z, -Dot(R, eye),
        U.x, U.y, U.z, -Dot(U, eye),
        D.x, D.y, D.z, -Dot(D, eye),
        0,   0,   0,   1
    };
}

template<typename T>
Matrix4x4<T> Matrix4x4<T>::Perspective(T fovYRad, T wOverH, T nearPlane, T farPlane, bool reverseZ)
{
    const T invDis = 1.0f / (farPlane - nearPlane);
    const T y_rad = 0.5f * fovYRad;
    const T cot = std::cos(y_rad) / std::sin(y_rad);
    const T a = (reverseZ ? -nearPlane : farPlane) * invDis;
    const T b = (reverseZ ? nearPlane : -nearPlane) * farPlane * invDis;
    return {
        -cot / wOverH, 0,   0, 0,
        0,             cot, 0, 0,
        0,             0,   a, b,
        0,             0,   1, 0
    };
}

template<typename T>
Matrix4x4<T>::Matrix4x4()
{
    *this = Zero();
}

template<typename T>
Matrix4x4<T>::Matrix4x4(const Matrix3x3<T> &m3x3)
    : Matrix4x4(m3x3[0][0], m3x3[0][1], m3x3[0][2], 0,
                m3x3[1][0], m3x3[1][1], m3x3[1][2], 0,
                m3x3[2][0], m3x3[2][1], m3x3[2][2], 0,
                0,          0,          0,          1)
{

}

template<typename T>
Matrix4x4<T>::Matrix4x4(
    T m11, T m12, T m13, T m14,
    T m21, T m22, T m23, T m24,
    T m31, T m32, T m33, T m34,
    T m41, T m42, T m43, T m44)
{
    rows[0] = { m11, m12, m13, m14 };
    rows[1] = { m21, m22, m23, m24 };
    rows[2] = { m31, m32, m33, m34 };
    rows[3] = { m41, m42, m43, m44 };
}

template<typename T>
T Matrix4x4<T>::At(int row, int col) const
{
    assert(1 <= row && row <= 4);
    assert(1 <= col && col <= 4);
    return rows[row - 1][col - 1];
}

template<typename T>
T &Matrix4x4<T>::At(int row, int col)
{
    assert(1 <= row && row <= 4);
    assert(1 <= col && col <= 4);
    return rows[row - 1][col - 1];
}

template<typename T>
const Vector4<T> &Matrix4x4<T>::operator[](int rowMinusOne) const
{
    return rows[rowMinusOne];
}

template<typename T>
Vector4<T> &Matrix4x4<T>::operator[](int rowMinusOne)
{
    return rows[rowMinusOne];
}

template<typename T>
const Vector4<T> &Matrix4x4<T>::GetRow(int rowMinusOne) const
{
    return rows[rowMinusOne];
}

template<typename T>
Vector4<T> Matrix4x4<T>::GetCol(int colMinusOne) const
{
    return { rows[0][colMinusOne], rows[1][colMinusOne], rows[2][colMinusOne], rows[3][colMinusOne] };
}

template<typename T>
Matrix4x4<T> &Matrix4x4<T>::operator+=(const Matrix4x4<T> &b)
{
    *this = *this + b;
    return *this;
}

template<typename T>
Matrix4x4<T> &Matrix4x4<T>::operator-=(const Matrix4x4<T> &b)
{
    *this = *this - b;
    return *this;
}

template<typename T>
Matrix4x4<T> &Matrix4x4<T>::operator*=(const Matrix4x4<T> &b)
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

template<typename T>
Matrix4x4<T> operator+(const Matrix4x4<T> &a, const Matrix4x4<T> &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] + b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix4x4<T> operator-(const Matrix4x4<T> &a, const Matrix4x4<T> &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] - b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix4x4<T> operator*(const Matrix4x4<T> &a, const Matrix4x4<T> &b)
{
    Matrix4x4<T> ret;
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

template<typename T>
Matrix4x4<T> operator*(T a, const Matrix4x4<T> &b)
{
    return b * a;
}

template<typename T>
Matrix4x4<T> operator*(const Matrix4x4<T> &a, T b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] * b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix4x4<T> operator/(const Matrix4x4<T> &a, T b)
{
    return a * (1.0f / b);
}

template<typename T>
Vector4<T> operator*(const Matrix4x4<T> &a, const Vector4<T> &b)
{
    Vector4<T> ret;
    for(int y = 0; y < 4; ++y)
    {
        ret[y] = a[y][0] * b[0]
               + a[y][1] * b[1]
               + a[y][2] * b[2]
               + a[y][3] * b[3];
    }
    return ret;
}

template<typename T>
Vector4<T> operator*(const Vector4<T> &a, const Matrix4x4<T> &b)
{
    Vector4<T> ret;
    for(int x = 0; x < 4; ++x)
    {
        ret[x] = a[0] * b[0][x]
               + a[1] * b[1][x]
               + a[2] * b[2][x]
               + a[3] * b[3][x];
    }
    return ret;
}

template<typename T>
Matrix4x4<T> ElementWiseMultiply(const Matrix4x4<T> &a, const Matrix4x4<T> &b)
{
#define RTRC_MAT_ELEM(R, C) (a.rows[R][C] * b.rows[R][C])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix4x4<T> Transpose(const Matrix4x4<T> &m)
{
#define RTRC_MAT_ELEM(R, C) (m[C][R])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

#undef RTRC_CREATE_MATRIX_FROM_ELEM

template<typename T>
Matrix4x4<T> Inverse(const Matrix4x4<T> &m)
{
    const Matrix4x4<T> adj = Adjoint(m);
    const T dem = Dot(m.GetCol(0), adj.GetRow(0));
    return adj / dem;
}

template<typename T>
Matrix4x4<T> Adjoint(const Matrix4x4<T> &m)
{
    const T blk00 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
    const T blk02 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
    const T blk03 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
    const T blk04 = m[1][2] * m[3][3] - m[1][3] * m[3][2];
    const T blk06 = m[1][1] * m[3][3] - m[1][3] * m[3][1];
    const T blk07 = m[1][1] * m[3][2] - m[1][2] * m[3][1];
    const T blk08 = m[1][2] * m[2][3] - m[1][3] * m[2][2];
    const T blk10 = m[1][1] * m[2][3] - m[1][3] * m[2][1];
    const T blk11 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    const T blk12 = m[0][2] * m[3][3] - m[0][3] * m[3][2];
    const T blk14 = m[0][1] * m[3][3] - m[0][3] * m[3][1];
    const T blk15 = m[0][1] * m[3][2] - m[0][2] * m[3][1];
    const T blk16 = m[0][2] * m[2][3] - m[0][3] * m[2][2];
    const T blk18 = m[0][1] * m[2][3] - m[0][3] * m[2][1];
    const T blk19 = m[0][1] * m[2][2] - m[0][2] * m[2][1];
    const T blk20 = m[0][2] * m[1][3] - m[0][3] * m[1][2];
    const T blk22 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
    const T blk23 = m[0][1] * m[1][2] - m[0][2] * m[1][1];

    const Vector4<T> fac0(blk00, blk00, blk02, blk03);
    const Vector4<T> fac1(blk04, blk04, blk06, blk07);
    const Vector4<T> fac2(blk08, blk08, blk10, blk11);
    const Vector4<T> fac3(blk12, blk12, blk14, blk15);
    const Vector4<T> fac4(blk16, blk16, blk18, blk19);
    const Vector4<T> fac5(blk20, blk20, blk22, blk23);

    const Vector4<T> vec0(m[0][1], m[0][0], m[0][0], m[0][0]);
    const Vector4<T> vec1(m[1][1], m[1][0], m[1][0], m[1][0]);
    const Vector4<T> vec2(m[2][1], m[2][0], m[2][0], m[2][0]);
    const Vector4<T> vec3(m[3][1], m[3][0], m[3][0], m[3][0]);

    const Vector4<T> inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
    const Vector4<T> inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
    const Vector4<T> inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
    const Vector4<T> inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

    const Vector4<T> sign_a(+1, -1, +1, -1);
    const Vector4<T> sign_b(-1, +1, -1, +1);

    return Matrix4x4<T>::FromCols(inv0 * sign_a, inv1 * sign_b, inv2 * sign_a, inv3 * sign_b);
}

RTRC_END
