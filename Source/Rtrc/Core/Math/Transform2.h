#pragma once

#include <Rtrc/Core/Math/Matrix3x3.h>

RTRC_BEGIN

template<typename T>
class Transform2
{
public:

    Transform2();
    explicit Transform2(const Matrix3x3<T> &m); // Assuming no shearing

    Transform2 &SetTranslation(const Vector2<T> &offset);
    const Vector2<T> &GetTranslation() const;

    Transform2 &SetRotation(T angleInRad);
    T GetRotation() const;

    Transform2 &SetScale(const Vector2<T> &scale);
    const Vector2<T> &GetScale() const;

    Matrix3x3<T> ToMatrix() const;
    Matrix3x3<T> ToInverseMatrix() const;

    Vector2<T> ApplyToPoint(const Vector2<T> &p) const;
    Vector2<T> ApplyToVector(const Vector2<T> &p) const;

    Transform2 Inverse() const;

private:

    Vector2<T> translate_;
    T rotate_;
    Vector2<T> scale_;
};

using Transform2f = Transform2<float>;
using Transform2d = Transform2<double>;

template<typename T>
Transform2<T> operator*(const Transform2<T> &a, const Transform2<T> &b);

template <typename T>
Transform2<T>::Transform2()
{
    translate_ = { 0, 0 };
    rotate_ = 0;
    scale_ = { 1, 1 };
}

template <typename T>
Transform2<T>::Transform2(const Matrix3x3<T> &m)
{
    const T det = m[0][0] * m[1][1] - m[0][1] * m[1][0];
    const Vector2<T> ex = { m[0][0], m[1][0] };
    const Vector2<T> ey = { m[0][1], m[1][1] };
    translate_ = { m[0][2], m[1][2] };
    scale_.x = Length(ex);
    scale_.y = Length(ey);
    if(det < 0)
    {
        scale_.y = -scale_.y;
    }
    rotate_ = std::atan2(ex.y, ex.x);
}

template <typename T>
Transform2<T> &Transform2<T>::SetTranslation(const Vector2<T> &offset)
{
    translate_ = offset;
    return *this;
}

template <typename T>
const Vector2<T> &Transform2<T>::GetTranslation() const
{
    return translate_;
}

template <typename T>
Transform2<T> &Transform2<T>::SetRotation(T angleInRad)
{
    rotate_ = angleInRad;
    return *this;
}

template <typename T>
T Transform2<T>::GetRotation() const
{
    return rotate_;
}

template <typename T>
Transform2<T> &Transform2<T>::SetScale(const Vector2<T> &scale)
{
    scale_ = scale;
    return *this;
}

template <typename T>
const Vector2<T> &Transform2<T>::GetScale() const
{
    return scale_;
}

template <typename T>
Matrix3x3<T> Transform2<T>::ToMatrix() const
{
    const T s = std::sin(rotate_), c = std::cos(rotate_);
    return {
        scale_.x * c, scale_.y * -s, translate_.x,
        scale_.x * s, scale_.y * c, translate_.y,
        0, 0, 1
    };
}

template <typename T>
Matrix3x3<T> Transform2<T>::ToInverseMatrix() const
{
    const T s = std::sin(-rotate_), c = std::cos(-rotate_);
    return 
        Matrix3x3<T>(
            1 / scale_.x, 0, 0,
            0, 1 / scale_.y, 0,
            0, 0, 1) *
        Matrix3x3<T>(
            c, s, 0,
            -s, c, 0,
            0, 0, 1) *
        Matrix3x3<T>(
            1, 0, -translate_.x,
            0, 1, -translate_.y,
            0, 0, 1);
}

template <typename T>
Vector2<T> Transform2<T>::ApplyToPoint(const Vector2<T> &p) const
{
    return this->ApplyToVector(p) +translate_;
}

template <typename T>
Vector2<T> Transform2<T>::ApplyToVector(const Vector2<T> &p) const
{
    const T s = std::sin(rotate_), c = std::cos(rotate_);
    const Vector2<T> scaledPoint = scale_ * p;
    const Vector2<T> rotatedPoint =
    {
        c * scaledPoint.x + -s * scaledPoint.y,
        s * scaledPoint.x + c * scaledPoint.y
    };
    return rotatedPoint;
}

template <typename T>
Transform2<T> Transform2<T>::Inverse() const
{
    // TODO
    return Transform2(ToInverseMatrix());
}

template <typename T>
Transform2<T> operator*(const Transform2<T> &a, const Transform2<T> &b)
{
    Transform2<T> result;

    // Translation

    result.SetTranslation(a.GetTranslation() + a.ApplyToVector(b.GetTranslation()));

    // Rotation & scale

    const Vector2<T> &scaleA = a.GetScale();
    const T epsilon = std::numeric_limits<T>::epsilon() * 100;

    // Fast path for uniform scaling
    if (std::abs(scaleA.x - scaleA.y) < epsilon)
    {
        result.SetRotation(a.GetRotation() + b.GetRotation());
        result.SetScale(scaleA * b.GetScale());
        return result;
    }

    // General path for non-uniform scaling

    const T ra = a.GetRotation();
    const T sa = std::sin(ra);
    const T ca = std::cos(ra);

    const T rb = b.GetRotation();
    const T sb = std::sin(rb);
    const T cb = std::cos(rb);
    
    const Vector2<T> &scaleB = b.GetScale();

    // Translated x basis

    const T x1 = scaleB.x * cb;
    const T y1 = scaleB.x * sb;

    const T x2 = x1 * scaleA.x;
    const T y2 = y1 * scaleA.y;
    
    const T finalX_x = x2 * ca - y2 * sa;
    const T finalX_y = x2 * sa + y2 * ca;

    // Transformed y basis

    const T x3 = -scaleB.y * sb;
    const T y3 =  scaleB.y * cb;

    const T x4 = x3 * scaleA.x;
    const T y4 = y3 * scaleA.y;

    const T finalY_x = x4 * ca - y4 * sa;
    const T finalY_y = x4 * sa + y4 * ca;

    // Build TRS from the new basis
    
    result.SetRotation(std::atan2(finalX_y, finalX_x));
    result.SetScale(Vector2<T>(0, 0));

    Vector2<T> newScale;
    newScale.x = std::sqrt(finalX_x * finalX_x + finalX_y * finalX_y);
    newScale.y = std::sqrt(finalY_x * finalY_x + finalY_y * finalY_y);

    const T det = finalX_x * finalY_y - finalX_y * finalY_x;
    if (det < 0)
    {
        newScale.y = -newScale.y;
    }

    result.SetScale(newScale);

    return result;
}

RTRC_END
