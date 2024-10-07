#pragma once

#include <Rtrc/Core/Math/Common.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_BEGIN

template<typename T>
class Matrix3x3
{
public:

    Vector3<T> rows[3];

    static const Matrix3x3 &Zero();
    static const Matrix3x3 &One();
    static const Matrix3x3 &Identity();

    static Matrix3x3 Diag(T v);
    static Matrix3x3 Fill(T v);

    static Matrix3x3 FromCols(
        const Vector3<T> &col1,
        const Vector3<T> &col2,
        const Vector3<T> &col3);
    static Matrix3x3<T> FromRows(
        const Vector3<T> &row1,
        const Vector3<T> &row2,
        const Vector3<T> &row3);

    static Matrix3x3 Rotate(const Vector3<T> &axis, T rad);
    static Matrix3x3 RotateX(T rad);
    static Matrix3x3 RotateY(T rad);
    static Matrix3x3 RotateZ(T rad);

    Matrix3x3();

    Matrix3x3(
        T m11, T m12, T m13,
        T m21, T m22, T m23,
        T m31, T m32, T m33);

    const Vector3<T> &GetRow(int rowMinusOne) const;
          Vector3<T>  GetCol(int colMinusOne) const;

    const Vector3<T> &operator[](int rowMinusOne) const;
          Vector3<T> &operator[](int rowMinusOne);

    Matrix3x3 &operator+=(const Matrix3x3 &b);
    Matrix3x3 &operator-=(const Matrix3x3 &b);
    Matrix3x3 &operator*=(const Matrix3x3 &b);
};

using Matrix3x3f = Matrix3x3<float>;
using Matrix3x3d = Matrix3x3<double>;

template<typename T>
Matrix3x3<T> operator+(const Matrix3x3<T> &a, const Matrix3x3<T> &b);
template<typename T>
Matrix3x3<T> operator-(const Matrix3x3<T> &a, const Matrix3x3<T> &b);
template<typename T>
Matrix3x3<T> operator*(const Matrix3x3<T> &a, const Matrix3x3<T> &b);

template<typename T>
Matrix3x3<T> operator*(const Matrix3x3<T> &a, T b);
template<typename T>
Matrix3x3<T> operator*(T a, const Matrix3x3<T> &b);
template<typename T>
Matrix3x3<T> operator/(const Matrix3x3<T> &a, T b);

// b: column vector
template<typename T>
Vector3<T> operator*(const Matrix3x3<T> &a, const Vector3<T> &b);
// a: row vector
template<typename T>
Vector3<T> operator*(const Vector3<T> &a, const Matrix3x3<T> &b);

template<typename T>
Matrix3x3<T> ElemenetWiseMultiply(const Matrix3x3<T> &a, const Matrix3x3<T> &b);

template<typename T>
Matrix3x3<T> Transpose(const Matrix3x3<T> &m);
template<typename T>
Matrix3x3<T> Inverse(const Matrix3x3<T> &m);
template<typename T>
Matrix3x3<T> Adjoint(const Matrix3x3<T> &m);

template<typename T>
const Matrix3x3<T> &Matrix3x3<T>::Zero()
{
    static const Matrix3x3<T> ret = Fill(0);
    return ret;
}

template<typename T>
const Matrix3x3<T> &Matrix3x3<T>::One()
{
    static const Matrix3x3<T> ret = Fill(1);
    return ret;
}

template<typename T>
const Matrix3x3<T> &Matrix3x3<T>::Identity()
{
    static const Matrix3x3<T> ret = Diag(1);
    return ret;
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::Diag(T v)
{
    return {
        v, 0, 0,
        0, v, 0,
        0, 0, v
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::Fill(T v)
{
    return {
        v, v, v,
        v, v, v,
        v, v, v
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::FromCols(const Vector3<T> &col1, const Vector3<T> &col2, const Vector3<T> &col3)
{
    return {
        col1.x, col2.x, col3.x,
        col1.y, col2.y, col3.y,
        col1.z, col2.z, col3.z
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::FromRows(const Vector3<T> &row1, const Vector3<T> &row2, const Vector3<T> &row3)
{
    return {
        row1.x, row1.y, row1.z,
        row2.x, row2.y, row2.z,
        row3.x, row3.y, row3.z
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::Rotate(const Vector3<T> &_axis, T rad)
{
    const Vector3<T> axis = Normalize(_axis);
    const T sinv = std::sin(rad), cosv = std::cos(rad);
    Matrix3x3<T> ret;
    ret[0][0] = axis.x * axis.x + (1 - axis.x * axis.x) * cosv;
    ret[1][0] = axis.x * axis.y * (1 - cosv) - axis.z * sinv;
    ret[2][0] = axis.x * axis.z * (1 - cosv) + axis.y * sinv;
    ret[0][1] = axis.x * axis.y * (1 - cosv) + axis.z * sinv;
    ret[1][1] = axis.y * axis.y + (1 - axis.y * axis.y) * cosv;
    ret[2][1] = axis.y * axis.z * (1 - cosv) - axis.x * sinv;
    ret[0][2] = axis.x * axis.z * (1 - cosv) - axis.y * sinv;
    ret[1][2] = axis.y * axis.z * (1 - cosv) + axis.x * sinv;
    ret[2][2] = axis.z * axis.z + (1 - axis.z * axis.z) * cosv;
    return ret;
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::RotateX(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        1, 0, 0,
        0, C, -S,
        0, S, C
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::RotateY(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, 0, S,
        0, 1, 0,
        -S, 0, C
    };
}

template<typename T>
Matrix3x3<T> Matrix3x3<T>::RotateZ(T rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, -S, 0,
        S, C, 0,
        0, 0, 1
    };
}

template<typename T>
Matrix3x3<T>::Matrix3x3()
{
    *this = Zero();
}

template<typename T>
Matrix3x3<T>::Matrix3x3(
    T m11, T m12, T m13,
    T m21, T m22, T m23,
    T m31, T m32, T m33)
{
    rows[0] = { m11, m12, m13 };
    rows[1] = { m21, m22, m23 };
    rows[2] = { m31, m32, m33 };
}

template<typename T>
const Vector3<T> &Matrix3x3<T>::GetRow(int rowMinusOne) const
{
    return rows[rowMinusOne];
}

template<typename T>
Vector3<T> Matrix3x3<T>::GetCol(int colMinusOne) const
{
    return { rows[0][colMinusOne], rows[1][colMinusOne], rows[2][colMinusOne] };
}

template<typename T>
const Vector3<T> &Matrix3x3<T>::operator[](int rowMinusOne) const
{
    return rows[rowMinusOne];
}

template<typename T>
Vector3<T> &Matrix3x3<T>::operator[](int rowMinusOne)
{
    return rows[rowMinusOne];
}

template<typename T>
Matrix3x3<T> &Matrix3x3<T>::operator+=(const Matrix3x3<T> &b)
{
    return *this = *this + b;
}

template<typename T>
Matrix3x3<T> &Matrix3x3<T>::operator-=(const Matrix3x3<T> &b)
{
    return *this = *this - b;
}

template<typename T>
Matrix3x3<T> &Matrix3x3<T>::operator*=(const Matrix3x3<T> &b)
{
    return *this = *this * b;
}

#define RTRC_CREATE_MATRIX_FROM_ELEM(F) \
    {                                   \
        F(0, 0), F(0, 1), F(0, 2),      \
        F(1, 0), F(1, 1), F(1, 2),      \
        F(2, 0), F(2, 1), F(2, 2)       \
    }

template<typename T>
Matrix3x3<T> operator+(const Matrix3x3<T> &a, const Matrix3x3<T> &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] + b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix3x3<T> operator-(const Matrix3x3<T> &a, const Matrix3x3<T> &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] - b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix3x3<T> operator*(const Matrix3x3<T> &a, const Matrix3x3<T> &b)
{
    Matrix3x3<T> ret;
    for(int r = 0; r < 3; ++r)
    {
        for(int c = 0; c < 3; ++c)
        {
            ret[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
        }
    }
    return ret;
}

template<typename T>
Matrix3x3<T> operator*(const Matrix3x3<T> &a, T b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] * b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix3x3<T> operator*(T a, const Matrix3x3<T> &b)
{
#define RTRC_MAT_ELEM(r, c) (a * b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix3x3<T> operator/(const Matrix3x3<T> &a, T b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] / b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

// b: column vector
template<typename T>
Vector3<T> operator*(const Matrix3x3<T> &a, const Vector3<T> &b)
{
    return {
        a[0][0] * b.x + a[0][1] * b.y + a[0][2] * b.z,
        a[1][0] * b.x + a[1][1] * b.y + a[1][2] * b.z,
        a[2][0] * b.x + a[2][1] * b.y + a[2][2] * b.z
    };
}

// a: row vector
template<typename T>
Vector3<T> operator*(const Vector3<T> &a, const Matrix3x3<T> &b)
{
    return {
        a.x * b[0][0] + a.y * b[1][0] + a.z * b[2][0],
        a.x * b[0][1] + a.y * b[1][1] + a.z * b[2][1],
        a.x * b[0][2] + a.y * b[1][2] + a.z * b[2][2]
    };
}

template<typename T>
Matrix3x3<T> ElemenetWiseMultiply(const Matrix3x3<T> &a, const Matrix3x3<T> &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] * b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

template<typename T>
Matrix3x3<T> Transpose(const Matrix3x3<T> &m)
{
#define RTRC_MAT_ELEM(r, c) (m[c][r])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

#undef RTRC_CREATE_MATRIX_FROM_ELEM

template<typename T>
Matrix3x3<T> Inverse(const Matrix3x3<T> &m)
{
    const Matrix3x3<T> adj = Adjoint(m);
    const T dem = Dot(m.GetCol(0), adj.GetRow(0));
    return adj / dem;
}

template<typename T>
Matrix3x3<T> Adjoint(const Matrix3x3<T> &m)
{
    auto det = [](T a, T b, T c, T d) { return a * d - b * c; };
    Matrix3x3<T> adj;
    adj.rows[0][0] = +det(m.rows[1][1], m.rows[1][2], m.rows[2][1], m.rows[2][2]);
    adj.rows[0][1] = -det(m.rows[0][1], m.rows[0][2], m.rows[2][1], m.rows[2][2]);
    adj.rows[0][2] = +det(m.rows[0][1], m.rows[0][2], m.rows[1][1], m.rows[1][2]);
    adj.rows[1][0] = -det(m.rows[1][0], m.rows[1][2], m.rows[2][0], m.rows[2][2]);
    adj.rows[1][1] = +det(m.rows[0][0], m.rows[0][2], m.rows[2][0], m.rows[2][2]);
    adj.rows[1][2] = -det(m.rows[0][0], m.rows[0][2], m.rows[1][0], m.rows[1][2]);
    adj.rows[2][0] = +det(m.rows[1][0], m.rows[1][1], m.rows[2][0], m.rows[2][1]);
    adj.rows[2][1] = -det(m.rows[0][0], m.rows[0][1], m.rows[2][0], m.rows[2][1]);
    adj.rows[2][2] = +det(m.rows[0][0], m.rows[0][1], m.rows[1][0], m.rows[1][1]);
    return adj;
}

RTRC_END
