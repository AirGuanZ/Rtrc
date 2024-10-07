#pragma once

#include <Rtrc/Core/Math/Matrix3x3.h>

RTRC_BEGIN

template<typename T>
class Quaternion
{
    template<int Axis>
    static Quaternion FromRotationAlong(T rad);

public:

    T x, y, z, w;

    // (0, 0, 0) -> forward +x, up +y
    // Apply order: roll, pitch, yaw
    // Roll:  along x
    // Yaw:   along y, look around
    // Pitch: along z, look up/down
    static Quaternion FromClassicalRotation(T roll, T yaw, T pitch);
    static Quaternion FromMatrix(const Matrix3x3f &m);
    static Quaternion FromRotationAlongAxis(const Vector3<T> &axis, T rad);
    static Quaternion FromRotationAlongNormalizedAxis(const Vector3<T> &axis, T rad);
    static Quaternion FromRotationAlongX(T rad);
    static Quaternion FromRotationAlongY(T rad);
    static Quaternion FromRotationAlongZ(T rad);

    Quaternion();
    Quaternion(T x, T y, T z, T w);
    explicit Quaternion(T v);

    Vector3<T> ApplyRotation(const Vector3<T> &v) const;

    Matrix3x3f ToMatrix() const;

    T  operator[](size_t i) const;
    T &operator[](size_t i);
};

using Quaternionf = Quaternion<float>;
using Quaterniond = Quaternion<double>;

template<typename T>
Quaternion<T> Inverse(const Quaternion<T> &q);

template<typename T>
Quaternion<T> operator*(const Quaternion<T> &a, const Quaternion<T> &b);

template<typename T>
Quaternion<T> Quaternion<T>::FromClassicalRotation(T roll, T yaw, T pitch)
{
    const T Sx = std::sin(0.5f * roll),  Cx = std::cos(0.5f * roll);
    const T Sy = std::sin(0.5f * yaw),   Cy = std::cos(0.5f * yaw);
    const T Sz = std::sin(0.5f * pitch), Cz = std::cos(0.5f * pitch);

    Quaternion ret;
    ret.x = Cx * Sy * Sz + Sx * Cy * Cz;
    ret.y = Cx * Sy * Cz + Sx * Cy * Sz;
    ret.z = Cx * Cy * Sz - Sx * Sy * Cz;
    ret.w = Cx * Cy * Cz - Sx * Sy * Sz;
    return ret;
}

// Shepperds¡¯s method. See [Accurate Computation of Quaternions from Rotation Matrices].
template<typename T>
Quaternion<T> Quaternion<T>::FromMatrix(const Matrix3x3f &m)
{
    Quaternion ret;
    const T s = m[0][0] + m[1][1] + m[2][2];
    if(s >= 0.0f)
    {
        const T q1 = 0.5f * std::sqrt(s + 1.0f);
        const T invQ1 = 1 / q1;
        const T q2 = 0.25f * invQ1 * (m[2][1] - m[1][2]);
        const T q3 = 0.25f * invQ1 * (m[0][2] - m[2][0]);
        const T q4 = 0.25f * invQ1 * (m[1][0] - m[0][1]);
        ret.x = q2;
        ret.y = q3;
        ret.z = q4;
        ret.w = q1;
    }
    else
    {
        constexpr int NEXT_LUT[3] = { 1, 2, 0 };
        const int a = ArgMax(m[0][0], m[1][1], m[2][2]);
        const int b = NEXT_LUT[a];
        const int c = NEXT_LUT[b];
        T q[3];
        q[a]  = T(0.5) * std::sqrt(m[a][a] - m[b][b] - m[c][c] + 1.0f);
        const T invQa = 1 / q[a];
        q[b] = T(0.25) * invQa * (m[a][b] + m[b][a]);
        q[c] = T(0.25) * invQa * (m[a][c] + m[c][a]);
        ret.x = q[0];
        ret.y = q[1];
        ret.z = q[2];
        ret.w = T(0.25) * invQa * (m[c][b] - m[b][c]);
    }
    return ret;
}

template<typename T>
Quaternion<T> Quaternion<T>::FromRotationAlongAxis(const Vector3<T> &axis, T rad)
{
    return FromRotationAlongNormalizedAxis(Normalize(axis), rad);
}

template<typename T>
Quaternion<T> Quaternion<T>::FromRotationAlongNormalizedAxis(const Vector3<T> &axis, T rad)
{
    const T C = std::cos(0.5f * rad);
    const T S = std::sin(0.5f * rad);
    return { S * axis.x, S * axis.y, S * axis.z, C };
}

template<typename T>
template<int Axis>
Quaternion<T> Quaternion<T>::FromRotationAlong(T rad)
{
    const T C = std::cos(0.5f * rad);
    const T S = std::sin(0.5f * rad);
    Quaternion ret;
    ret[Axis] = S;
    ret.w = C;
    return ret;
}

template<typename T>
Quaternion<T> Quaternion<T>::FromRotationAlongX(T rad)
{
    return FromRotationAlong<0>(rad);
}

template<typename T>
Quaternion<T> Quaternion<T>::FromRotationAlongY(T rad)
{
    return FromRotationAlong<1>(rad);
}

template<typename T>
Quaternion<T> Quaternion<T>::FromRotationAlongZ(T rad)
{
    return FromRotationAlong<2>(rad);
}

template<typename T>
Quaternion<T>::Quaternion()
    : Quaternion(0.0f)
{
    
}

template<typename T>
Quaternion<T>::Quaternion(T v)
    : x(v), y(v), z(v), w(v)
{
    
}

template<typename T>
Quaternion<T>::Quaternion(T x, T y, T z, T w)
    : x(x), y(y), z(z), w(w)
{
    
}

template<typename T>
Vector3<T> Quaternion<T>::ApplyRotation(const Vector3<T> &v) const
{
    const Vector3<T> q(x, y, z);
    const Vector3<T> t = T(2.0) * Cross(q, v);
    return v + w * t + Cross(q, t);
}

template<typename T>
Matrix3x3f Quaternion<T>::ToMatrix() const
{
    const T q1 = w;
    const T q2 = x;
    const T q3 = y;
    const T q4 = z;
    return {
        q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4, 2 * (q2 * q3 - q1 * q4), 2 * (q2 * q4 + q1 * q3),
        2 * (q2 * q3 + q1 * q4), q1 * q1 - q2 * q2 + q3 * q3 - q4 * q4, 2 * (q2 * q4 - q1 * q2),
        2 * (q2 * q4 - q1 * q3), 2 * (q3 * q4 + q1 * q2), q1 * q1 - q2 * q2 - q3 * q3 + q4 * q4
    };
}

template<typename T>
T Quaternion<T>::operator[](size_t i) const
{
    return *(&x + i);
}

template<typename T>
T &Quaternion<T>::operator[](size_t i)
{
    return *(&x + i);
}

template<typename T>
Quaternion<T> Inverse(const Quaternion<T> &q)
{
    return { -q.x, -q.y, -q.z, q.w };
}

template<typename T>
Quaternion<T> operator*(const Quaternion<T> &a, const Quaternion<T> &b)
{
    const T s = a.w;
    const T t = b.w;
    const Vector3<T> v = { a.x, a.y, a.z };
    const Vector3<T> u = { b.x, b.y, b.z };
    const T w = s * t - Dot(v, u);
    const Vector3<T> xyz = s * u + t * v + Cross(v, u);
    return { xyz.x, xyz.y, xyz.z, w };
}

RTRC_END
