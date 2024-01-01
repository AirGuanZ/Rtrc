#pragma once

#include <Rtrc/Core/Math/Matrix4x4.h>
#include <Rtrc/Core/Math/Quaternion.h>

RTRC_BEGIN

class Transform
{
public:

    Transform();
    explicit Transform(const Matrix4x4f &m);

    Transform &SetTranslation(const Vector3f &position);
    const Vector3f &GetTranslation() const;

    Transform &SetRotation(const Quaternion &rotation);
    const Quaternion &GetRotation() const;

    Transform &SetScale(const Vector3f &scale);
    const Vector3f &GetScale() const;

    Matrix4x4f ToMatrix() const;
    Matrix4x4f ToInverseMatrix() const;

    Vector3f ApplyToPoint(const Vector3f &p) const;
    Vector3f ApplyToVector(const Vector3f &v) const;

    Transform Inverse() const;

private:

    Vector3f   translate_;
    Quaternion rotate_;
    Vector3f   scale_;
};

Transform Inverse(const Transform &t);

Transform operator*(const Transform &a, const Transform &b);

inline Transform::Transform()
{
    rotate_ = { 0, 0, 0, 1 };
    scale_ = { 1, 1, 1 };
}

inline Transform::Transform(const Matrix4x4f &m)
{
    translate_ = { m[0][3], m[1][3], m[2][3] };
    const Vector3f ex = { m[0][0], m[1][0], m[2][0] };
    const Vector3f ey = { m[0][1], m[1][1], m[2][1] };
    const Vector3f ez = { m[0][2], m[1][2], m[2][2] };
    scale_.x = Length(ex);
    scale_.y = Length(ey);
    scale_.z = Length(ez);
    rotate_ = Quaternion::FromMatrix(Matrix3x3f::FromCols(ex / scale_.x, ey / scale_.y, ez / scale_.z));
}

inline Transform &Transform::SetTranslation(const Vector3f &position)
{
    translate_ = position;
    return *this;
}

inline const Vector3f &Transform::GetTranslation() const
{
    return translate_;
}

inline Transform &Transform::SetRotation(const Quaternion &rotation)
{
    rotate_ = rotation;
    return *this;
}

inline const Quaternion &Transform::GetRotation() const
{
    return rotate_;
}

inline Transform &Transform::SetScale(const Vector3f &scale)
{
    scale_ = scale;
    return *this;
}

inline const Vector3f &Transform::GetScale() const
{
    return scale_;
}

inline Matrix4x4f Transform::ToMatrix() const
{
    const Matrix3x3f rot = rotate_.ToMatrix();
    return {
        scale_.x * rot[0][0], scale_.y * rot[0][1], scale_.z * rot[0][2], translate_.x,
        scale_.x * rot[1][0], scale_.y * rot[1][1], scale_.z * rot[1][2], translate_.y,
        scale_.x * rot[2][0], scale_.y * rot[2][1], scale_.z * rot[2][2], translate_.z,
        0, 0, 0, 1
    };
}

inline Matrix4x4f Transform::ToInverseMatrix() const
{
    const Matrix3x3f invScaleRot = Matrix3x3f(1 / scale_.x, 0, 0, 0, 1 / scale_.y, 0, 0, 0, 1 / scale_.z)
        * Rtrc::Inverse(rotate_).ToMatrix();
    const Matrix4x4f invTranslate = Matrix4x4f::Translate(-translate_);
    return Matrix4x4f(invScaleRot) * invTranslate;
}

inline Vector3f Transform::ApplyToPoint(const Vector3f &p) const
{
    return rotate_.ApplyRotation(scale_ * p) + translate_;
}

inline Vector3f Transform::ApplyToVector(const Vector3f &v) const
{
    return rotate_.ApplyRotation(scale_ * v);
}

inline Transform Transform::Inverse() const
{
    // TODO: optimize
    return Transform(ToInverseMatrix());
}

RTRC_END
