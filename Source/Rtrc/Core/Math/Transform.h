#pragma once

#include <Rtrc/Core/Math/AABB.h>
#include <Rtrc/Core/Math/Matrix4x4.h>
#include <Rtrc/Core/Math/Quaternion.h>

RTRC_BEGIN

template<typename T>
class Transform
{
public:

    Transform();
    explicit Transform(const Matrix4x4<T> &m); // Assuming no shearing

    Transform &SetTranslation(const Vector3<T> &position);
    const Vector3<T> &GetTranslation() const;

    Transform &SetRotation(const Quaternion<T> &rotation);
    const Quaternion<T> &GetRotation() const;

    Transform &SetScale(const Vector3<T> &scale);
    const Vector3<T> &GetScale() const;

    Matrix4x4<T> ToMatrix() const;
    Matrix4x4<T> ToInverseMatrix() const;

    Vector3<T> ApplyToPoint(const Vector3<T> &p) const;
    Vector3<T> ApplyToVector(const Vector3<T> &v) const;
    AABB3<T> ApplyToAABB(const AABB3<T> &bbox) const;

    Transform Inverse() const;

private:

    Vector3<T>    translate_;
    Quaternion<T> rotate_;
    Vector3<T>    scale_;
};

using Transformf = Transform<float>;
using Transformd = Transform<double>;

template<typename T>
Transform<T> Inverse(const Transform<T> &t);

template<typename T>
Transform<T> operator*(const Transform<T> &a, const Transform<T> &b);

template<typename T>
Transform<T>::Transform()
{
    rotate_ = { 0, 0, 0, 1 };
    scale_ = { 1, 1, 1 };
}

template<typename T>
Transform<T>::Transform(const Matrix4x4<T> &m)
{
          Vector3<T> ex = { m[0][0], m[1][0], m[2][0] };
    const Vector3<T> ey = { m[0][1], m[1][1], m[2][1] };
    const Vector3<T> ez = { m[0][2], m[1][2], m[2][2] };

    const T det = Det3x3(
        m[0][0], m[0][1], m[0][2],
        m[1][0], m[1][1], m[1][2],
        m[2][0], m[2][1], m[2][2]);
    translate_ = { m[0][3], m[1][3], m[2][3] };
    scale_.x = Length(ex);
    scale_.y = Length(ey);
    scale_.z = Length(ez);
    if(det < 0)
    {
        scale_.x = -scale_.x;
        ex = -ex;
    }
    rotate_ = Quaternion<T>::FromMatrix(Matrix3x3<T>::FromCols(ex / scale_.x, ey / scale_.y, ez / scale_.z));
}

template<typename T>
Transform<T> &Transform<T>::SetTranslation(const Vector3<T> &position)
{
    translate_ = position;
    return *this;
}

template<typename T>
const Vector3<T> &Transform<T>::GetTranslation() const
{
    return translate_;
}

template<typename T>
Transform<T> &Transform<T>::SetRotation(const Quaternion<T> &rotation)
{
    rotate_ = rotation;
    return *this;
}

template<typename T>
const Quaternion<T> &Transform<T>::GetRotation() const
{
    return rotate_;
}

template<typename T>
Transform<T> &Transform<T>::SetScale(const Vector3<T> &scale)
{
    scale_ = scale;
    return *this;
}

template<typename T>
const Vector3<T> &Transform<T>::GetScale() const
{
    return scale_;
}

template<typename T>
Matrix4x4<T> Transform<T>::ToMatrix() const
{
    const Matrix3x3<T> rot = rotate_.ToMatrix();
    return {
        scale_.x * rot[0][0], scale_.y * rot[0][1], scale_.z * rot[0][2], translate_.x,
        scale_.x * rot[1][0], scale_.y * rot[1][1], scale_.z * rot[1][2], translate_.y,
        scale_.x * rot[2][0], scale_.y * rot[2][1], scale_.z * rot[2][2], translate_.z,
        0, 0, 0, 1
    };
}

template<typename T>
Matrix4x4<T> Transform<T>::ToInverseMatrix() const
{
    const Matrix3x3<T> invScaleRot = Matrix3x3<T>(1 / scale_.x, 0, 0, 0, 1 / scale_.y, 0, 0, 0, 1 / scale_.z)
        * Rtrc::Inverse(rotate_).ToMatrix();
    const Matrix4x4<T> invTranslate = Matrix4x4<T>::Translate(-translate_);
    return Matrix4x4<T>(invScaleRot) * invTranslate;
}

template<typename T>
Vector3<T> Transform<T>::ApplyToPoint(const Vector3<T> &p) const
{
    return rotate_.ApplyRotation(scale_ * p) + translate_;
}

template<typename T>
Vector3<T> Transform<T>::ApplyToVector(const Vector3<T> &v) const
{
    return rotate_.ApplyRotation(scale_ * v);
}

template <typename T>
AABB3<T> Transform<T>::ApplyToAABB(const AABB3<T> &bbox) const
{
    AABB3<T> result;
    for(const Vector3<T> p : 
        {
            { bbox.lower.x, bbox.lower.y, bbox.lower.z },
            { bbox.lower.x, bbox.lower.y, bbox.upper.z },
            { bbox.lower.x, bbox.upper.y, bbox.lower.z },
            { bbox.lower.x, bbox.upper.y, bbox.upper.z },
            { bbox.upper.x, bbox.lower.y, bbox.lower.z },
            { bbox.upper.x, bbox.lower.y, bbox.upper.z },
            { bbox.upper.x, bbox.upper.y, bbox.lower.z },
            { bbox.upper.x, bbox.upper.y, bbox.upper.z },
        })
    {
        result |= this->ApplyToPoint(p);
    }
    return result;
}

template<typename T>
Transform<T> Transform<T>::Inverse() const
{
    // TODO: optimize
    return Transform<T>(ToInverseMatrix());
}

template <typename T>
Transform<T> Inverse(const Transform<T> &t)
{
    return t.Inverse();
}

template <typename T>
Transform<T> operator*(const Transform<T> &a, const Transform<T> &b)
{
    return Transform<T>(a.ToMatrix() * b.ToMatrix());
}

RTRC_END
