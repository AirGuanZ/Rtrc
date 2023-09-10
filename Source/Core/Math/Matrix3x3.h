#pragma once

#include <Core/Math/Common.h>
#include <Core/Math/Vector3.h>

RTRC_BEGIN

class Matrix3x3f
{
public:

    Vector3f rows[3];

    static const Matrix3x3f &Zero();
    static const Matrix3x3f &One();
    static const Matrix3x3f &Identity();

    static Matrix3x3f Diag(float v);
    static Matrix3x3f Fill(float v);

    static Matrix3x3f FromCols(
        const Vector3f &col1,
        const Vector3f &col2,
        const Vector3f &col3);
    static Matrix3x3f FromRows(
        const Vector3f &row1,
        const Vector3f &row2,
        const Vector3f &row3);

    static Matrix3x3f Rotate(const Vector3f &axis, float rad);
    static Matrix3x3f RotateX(float rad);
    static Matrix3x3f RotateY(float rad);
    static Matrix3x3f RotateZ(float rad);

    Matrix3x3f();

    Matrix3x3f(
        float m11, float m12, float m13,
        float m21, float m22, float m23,
        float m31, float m32, float m33);

    const Vector3f &GetRow(int rowMinusOne) const;
          Vector3f  GetCol(int colMinusOne) const;

    const Vector3f &operator[](int rowMinusOne) const;
          Vector3f &operator[](int rowMinusOne);

    Matrix3x3f &operator+=(const Matrix3x3f &b);
    Matrix3x3f &operator-=(const Matrix3x3f &b);
    Matrix3x3f &operator*=(const Matrix3x3f &b);
};

Matrix3x3f operator+(const Matrix3x3f &a, const Matrix3x3f &b);
Matrix3x3f operator-(const Matrix3x3f &a, const Matrix3x3f &b);
Matrix3x3f operator*(const Matrix3x3f &a, const Matrix3x3f &b);

Matrix3x3f operator*(const Matrix3x3f &a, float b);
Matrix3x3f operator*(float a, const Matrix3x3f &b);
Matrix3x3f operator/(const Matrix3x3f &a, float b);

// b: column vector
Vector3f operator*(const Matrix3x3f &a, const Vector3f &b);
// a: row vector
Vector3f operator*(const Vector3f &a, const Matrix3x3f &b);

Matrix3x3f ElemenetWiseMultiply(const Matrix3x3f &a, const Matrix3x3f &b);

Matrix3x3f Transpose(const Matrix3x3f &m);
Matrix3x3f Inverse(const Matrix3x3f &m);
Matrix3x3f Adjoint(const Matrix3x3f &m);

inline const Matrix3x3f &Matrix3x3f::Zero()
{
    static const Matrix3x3f ret = Fill(0);
    return ret;
}

inline const Matrix3x3f &Matrix3x3f::One()
{
    static const Matrix3x3f ret = Fill(1);
    return ret;
}

inline const Matrix3x3f &Matrix3x3f::Identity()
{
    static const Matrix3x3f ret = Diag(1);
    return ret;
}

inline Matrix3x3f Matrix3x3f::Diag(float v)
{
    return {
        v, 0, 0,
        0, v, 0,
        0, 0, v
    };
}

inline Matrix3x3f Matrix3x3f::Fill(float v)
{
    return {
        v, v, v,
        v, v, v,
        v, v, v
    };
}

inline Matrix3x3f Matrix3x3f::FromCols(const Vector3f &col1, const Vector3f &col2, const Vector3f &col3)
{
    return {
        col1.x, col2.x, col3.x,
        col1.y, col2.y, col3.y,
        col1.z, col2.z, col3.z
    };
}

inline Matrix3x3f Matrix3x3f::FromRows(const Vector3f &row1, const Vector3f &row2, const Vector3f &row3)
{
    return {
        row1.x, row1.y, row1.z,
        row2.x, row2.y, row2.z,
        row3.x, row3.y, row3.z
    };
}

inline Matrix3x3f Matrix3x3f::Rotate(const Vector3f &_axis, float rad)
{
    const Vector3f axis = Normalize(_axis);
    const float sinv = std::sin(rad), cosv = std::cos(rad);
    Matrix3x3f ret;
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

inline Matrix3x3f Matrix3x3f::RotateX(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        1, 0, 0,
        0, C, -S,
        0, S, C
    };
}

inline Matrix3x3f Matrix3x3f::RotateY(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, 0, S,
        0, 1, 0,
        -S, 0, C
    };
}

inline Matrix3x3f Matrix3x3f::RotateZ(float rad)
{
    const auto S = std::sin(rad), C = std::cos(rad);
    return {
        C, -S, 0,
        S, C, 0,
        0, 0, 1
    };
}

inline Matrix3x3f::Matrix3x3f()
{
    *this = Zero();
}

inline Matrix3x3f::Matrix3x3f(
    float m11, float m12, float m13,
    float m21, float m22, float m23,
    float m31, float m32, float m33)
{
    rows[0] = { m11, m12, m13 };
    rows[1] = { m21, m22, m23 };
    rows[2] = { m31, m32, m33 };
}

inline const Vector3f &Matrix3x3f::GetRow(int rowMinusOne) const
{
    return rows[rowMinusOne];
}

inline Vector3f Matrix3x3f::GetCol(int colMinusOne) const
{
    return { rows[0][colMinusOne], rows[1][colMinusOne], rows[2][colMinusOne] };
}

inline const Vector3f &Matrix3x3f::operator[](int rowMinusOne) const
{
    return rows[rowMinusOne];
}

inline Vector3f &Matrix3x3f::operator[](int rowMinusOne)
{
    return rows[rowMinusOne];
}

inline Matrix3x3f &Matrix3x3f::operator+=(const Matrix3x3f &b)
{
    return *this = *this + b;
}

inline Matrix3x3f &Matrix3x3f::operator-=(const Matrix3x3f &b)
{
    return *this = *this - b;
}

inline Matrix3x3f &Matrix3x3f::operator*=(const Matrix3x3f &b)
{
    return *this = *this * b;
}

#define RTRC_CREATE_MATRIX_FROM_ELEM(F) \
    {                                   \
        F(0, 0), F(0, 1), F(0, 2),      \
        F(1, 0), F(1, 1), F(1, 2),      \
        F(2, 0), F(2, 1), F(2, 2)       \
    }

inline Matrix3x3f operator+(const Matrix3x3f &a, const Matrix3x3f &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] + b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix3x3f operator-(const Matrix3x3f &a, const Matrix3x3f &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] - b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix3x3f operator*(const Matrix3x3f &a, const Matrix3x3f &b)
{
    Matrix3x3f ret;
    for(int r = 0; r < 3; ++r)
    {
        for(int c = 0; c < 3; ++c)
        {
            ret[r][c] = a[r][0] * b[0][c] + a[r][1] * b[1][c] + a[r][2] * b[2][c];
        }
    }
    return ret;
}

inline Matrix3x3f operator*(const Matrix3x3f &a, float b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] * b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix3x3f operator*(float a, const Matrix3x3f &b)
{
#define RTRC_MAT_ELEM(r, c) (a * b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix3x3f operator/(const Matrix3x3f &a, float b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] / b)
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

// b: column vector
inline Vector3f operator*(const Matrix3x3f &a, const Vector3f &b)
{
    return {
        a[0][0] * b.x + a[0][1] * b.y + a[0][2] * b.z,
        a[1][0] * b.x + a[1][1] * b.y + a[1][2] * b.z,
        a[2][0] * b.x + a[2][1] * b.y + a[2][2] * b.z
    };
}

// a: row vector
inline Vector3f operator*(const Vector3f &a, const Matrix3x3f &b)
{
    return {
        a.x * b[0][0] + a.y * b[1][0] + a.z * b[2][0],
        a.x * b[0][1] + a.y * b[1][1] + a.z * b[2][1],
        a.x * b[0][2] + a.y * b[1][2] + a.z * b[2][2]
    };
}

inline Matrix3x3f ElemenetWiseMultiply(const Matrix3x3f &a, const Matrix3x3f &b)
{
#define RTRC_MAT_ELEM(r, c) (a[r][c] * b[r][c])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

inline Matrix3x3f Transpose(const Matrix3x3f &m)
{
#define RTRC_MAT_ELEM(r, c) (m[c][r])
    return RTRC_CREATE_MATRIX_FROM_ELEM(RTRC_MAT_ELEM);
#undef RTRC_MAT_ELEM
}

#undef RTRC_CREATE_MATRIX_FROM_ELEM

inline Matrix3x3f Inverse(const Matrix3x3f &m)
{
    const Matrix3x3f adj = Adjoint(m);
    const float dem = Dot(m.GetCol(0), adj.GetRow(0));
    return adj / dem;
}

inline Matrix3x3f Adjoint(const Matrix3x3f &m)
{
    auto det = [](float a, float b, float c, float d) { return a * d - b * c; };
    Matrix3x3f adj;
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
