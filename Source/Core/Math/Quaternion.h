#pragma once

#include <Core/Math/Matrix3x3.h>

RTRC_BEGIN

class Quaternion
{
    template<int Axis>
    static Quaternion FromRotationAlong(float rad);

public:

    float x, y, z, w;

    // (0, 0, 0) -> forward +x, up +y
    // Apply order: roll, pitch, yaw
    // Roll:  along x
    // Yaw:   along y, look around
    // Pitch: along z, look up/down
    static Quaternion FromClassicalRotation(float roll, float yaw, float pitch);
    static Quaternion FromMatrix(const Matrix3x3f &m);
    static Quaternion FromRotationAlongAxis(const Vector3f &axis, float rad);
    static Quaternion FromRotationAlongNormalizedAxis(const Vector3f &axis, float rad);
    static Quaternion FromRotationAlongX(float rad);
    static Quaternion FromRotationAlongY(float rad);
    static Quaternion FromRotationAlongZ(float rad);

    Quaternion();
    Quaternion(float x, float y, float z, float w);
    explicit Quaternion(float v);

    Vector3f ApplyRotation(const Vector3f &v) const;

    Matrix3x3f ToMatrix() const;

    float  operator[](size_t i) const;
    float &operator[](size_t i);
};

Quaternion Inverse(const Quaternion &q);

Quaternion operator*(const Quaternion &a, const Quaternion &b);

inline Quaternion Quaternion::FromClassicalRotation(float roll, float yaw, float pitch)
{
    const float Sx = std::sin(0.5f * roll),  Cx = std::cos(0.5f * roll);
    const float Sy = std::sin(0.5f * yaw),   Cy = std::cos(0.5f * yaw);
    const float Sz = std::sin(0.5f * pitch), Cz = std::cos(0.5f * pitch);

    Quaternion ret;
    ret.x = Cx * Sy * Sz + Sx * Cy * Cz;
    ret.y = Cx * Sy * Cz + Sx * Cy * Sz;
    ret.z = Cx * Cy * Sz - Sx * Sy * Cz;
    ret.w = Cx * Cy * Cz - Sx * Sy * Sz;
    return ret;
}

// Shepperds¡¯s method
// See [Accurate Computation of Quaternions from Rotation Matrices]
inline Quaternion Quaternion::FromMatrix(const Matrix3x3f &m)
{
    Quaternion ret;
    const float s = m[0][0] + m[1][1] + m[2][2];
    if(s >= 0.0f)
    {
        const float q1 = 0.5f * std::sqrt(s + 1.0f);
        const float invQ1 = 1 / q1;
        const float q2 = 0.25f * invQ1 * (m[2][1] - m[1][2]);
        const float q3 = 0.25f * invQ1 * (m[0][2] - m[2][0]);
        const float q4 = 0.25f * invQ1 * (m[1][0] - m[0][1]);
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
        float q[3];
        q[a]  = 0.5f * std::sqrt(m[a][a] - m[b][b] - m[c][c] + 1.0f);
        const float invQa = 1 / q[a];
        q[b] = 0.25f * invQa * (m[a][b] + m[b][a]);
        q[c] = 0.25f * invQa * (m[a][c] + m[c][a]);
        ret.x = q[0];
        ret.y = q[1];
        ret.z = q[2];
        ret.w = 0.25f * invQa * (m[c][b] - m[b][c]);
    }
    return ret;
}

inline Quaternion Quaternion::FromRotationAlongAxis(const Vector3f &axis, float rad)
{
    return FromRotationAlongNormalizedAxis(Normalize(axis), rad);
}

inline Quaternion Quaternion::FromRotationAlongNormalizedAxis(const Vector3f &axis, float rad)
{
    const float C = std::cos(0.5f * rad);
    const float S = std::sin(0.5f * rad);
    return { S * axis.x, S * axis.y, S * axis.z, C };
}

template<int Axis>
Quaternion Quaternion::FromRotationAlong(float rad)
{
    const float C = std::cos(0.5f * rad);
    const float S = std::sin(0.5f * rad);
    Quaternion ret;
    ret[Axis] = S;
    ret.w = C;
    return ret;
}

inline Quaternion Quaternion::FromRotationAlongX(float rad)
{
    return FromRotationAlong<0>(rad);
}

inline Quaternion Quaternion::FromRotationAlongY(float rad)
{
    return FromRotationAlong<1>(rad);
}

inline Quaternion Quaternion::FromRotationAlongZ(float rad)
{
    return FromRotationAlong<2>(rad);
}

inline Quaternion::Quaternion()
    : Quaternion(0.0f)
{
    
}

inline Quaternion::Quaternion(float v)
    : x(v), y(v), z(v), w(v)
{
    
}

inline Quaternion::Quaternion(float x, float y, float z, float w)
    : x(x), y(y), z(z), w(w)
{
    
}

inline Vector3f Quaternion::ApplyRotation(const Vector3f &v) const
{
    const Vector3f q(x, y, z);
    const Vector3f t = 2.0f * Cross(q, v);
    return v + w * t + Cross(q, t);
}

inline Matrix3x3f Quaternion::ToMatrix() const
{
    const float q1 = w;
    const float q2 = x;
    const float q3 = y;
    const float q4 = z;
    return {
        q1 * q1 + q2 * q2 + q3 * q3 + q4 * q4, 2 * (q2 * q3 - q1 * q4), 2 * (q2 * q4 + q1 * q3),
        2 * (q2 * q3 + q1 * q4), q1 * q1 - q2 * q2 + q3 * q3 - q4 * q4, 2 * (q2 * q4 - q1 * q2),
        2 * (q2 * q4 - q1 * q3), 2 * (q3 * q4 + q1 * q2), q1 * q1 - q2 * q2 - q3 * q3 + q4 * q4
    };
}

inline float Quaternion::operator[](size_t i) const
{
    return *(&x + i);
}

inline float &Quaternion::operator[](size_t i)
{
    return *(&x + i);
}

inline Quaternion Inverse(const Quaternion &q)
{
    return { -q.x, -q.y, -q.z, q.w };
}

inline Quaternion operator*(const Quaternion &a, const Quaternion &b)
{
    const float s = a.w;
    const float t = b.w;
    const Vector3f v = { a.x, a.y, a.z };
    const Vector3f u = { b.x, b.y, b.z };
    const float w = s * t - Dot(v, u);
    const Vector3f xyz = s * u + t * v + Cross(v, u);
    return { xyz.x, xyz.y, xyz.z, w };
}

RTRC_END
