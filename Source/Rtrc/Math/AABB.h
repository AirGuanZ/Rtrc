#pragma once

#include <cassert>

#include <Rtrc/Math/Matrix4x4.h>

RTRC_BEGIN

template<typename T>
class AABB3
{
public:

    using Component = T;

    AABB3();

    AABB3(const Vector3<T> &lower, const Vector3<T> &upper);

    bool IsValid() const;
    bool IsEmpty() const;

    Vector3<T> ComputeExtent() const;
    T ComputeSurfaceArea() const;
    T ComputeVolume() const;

    Vector3<T> ComputeCenter() const;

    bool Contains(const Vector3<T> &point) const;
    AABB3 Contains(const AABB3 &box) const;

    AABB3 &operator|=(const AABB3 &rhs);
    AABB3 &operator|=(const Vector3<T> &rhs);

    Vector3<T> lower;
    Vector3<T> upper;
};

using AABB3f = AABB3<float>;

template<typename T>
AABB3<T> operator|(const AABB3<T> &lhs, const AABB3<T> &rhs);
template<typename T>
AABB3<T> operator|(const AABB3<T> &lhs, const Vector3<T> &rhs);
template<typename T>
AABB3<T> operator|(const Vector3<T> &lhs, const AABB3<T> &rhs);

inline AABB3f operator*(const Matrix4x4f &mat, const AABB3f &bbox);

template<typename T>
AABB3<T>::AABB3()
    : lower((std::numeric_limits<T>::max)()), upper((std::numeric_limits<T>::lowest)())
{
    
}

template<typename T>
AABB3<T>::AABB3(const Vector3<T> &lower, const Vector3<T> &upper)
    : lower(lower), upper(upper)
{

}

template<typename T>
Vector3<T> AABB3<T>::ComputeExtent() const
{
    return upper - lower;
}

template<typename T>
bool AABB3<T>::IsValid() const
{
    return lower.x <= upper.x && lower.y <= upper.y && lower.z <= upper.z;
}

template<typename T>
bool AABB3<T>::IsEmpty() const
{
    return lower.x >= upper.x || lower.y >= upper.y || lower.z >= upper.z;
}

template<typename T>
T AABB3<T>::ComputeSurfaceArea() const
{
    assert(IsValid());
    const Vector3<T> extent = ComputeExtent();
    return extent.x * extent.y + extent.y * extent.z + extent.x * extent.z;
}

template<typename T>
T AABB3<T>::ComputeVolume() const
{
    assert(IsValid());
    const Vector3<T> extent = ComputeExtent();
    return extent.x * extent.y * extent.z;
}

template<typename T>
AABB3<T> AABB3<T>::Contains(const AABB3 &rhs) const
{
    return lower.x <= rhs.lower.x &&
           lower.y <= rhs.lower.y &&
           lower.z <= rhs.lower.z &&
           upper.x >= rhs.upper.x &&
           upper.y >= rhs.upper.y &&
           upper.z >= rhs.upper.z;
}

template<typename T>
bool AABB3<T>::Contains(const Vector3<T> &point) const
{
    return lower.x <= point.x &&
           lower.y <= point.y &&
           lower.z <= point.z &&
           upper.x >= point.x &&
           upper.y >= point.y &&
           upper.z >= point.z;
}

template<typename T>
Vector3<T> AABB3<T>::ComputeCenter() const
{
    static_assert(std::is_floating_point_v<T>);
    return static_cast<float>(0.5) * (lower + upper);
}

template<typename T>
AABB3<T> &AABB3<T>::operator|=(const AABB3 &rhs)
{
    return *this = *this | rhs;
}

template<typename T>
AABB3<T> &AABB3<T>::operator|=(const Vector3<T> &rhs)
{
    return *this = *this | rhs;
}

template<typename T>
AABB3<T> operator|(const AABB3<T> &lhs, const AABB3<T> &rhs)
{
    return AABB3<T>(Min(lhs.lower, rhs.lower), Max(lhs.upper, rhs.upper));
}

template<typename T>
AABB3<T> operator|(const AABB3<T> &lhs, const Vector3<T> &rhs)
{
    return AABB3<T>(Min(lhs.lower, rhs), Max(lhs.upper, rhs));
}

template<typename T>
AABB3<T> operator|(const Vector3<T> &lhs, const AABB3<T> &rhs)
{
    return rhs | lhs;
}

inline AABB3f operator*(const Matrix4x4f &mat, const AABB3f &bbox)
{
    if(!bbox.IsValid())
    {
        return bbox;
    }
    AABB3f ret;
    for(auto &p : {
        Vector3f(bbox.lower.x, bbox.lower.y, bbox.lower.z),
        Vector3f(bbox.lower.x, bbox.lower.y, bbox.upper.z),
        Vector3f(bbox.lower.x, bbox.upper.y, bbox.lower.z),
        Vector3f(bbox.lower.x, bbox.upper.y, bbox.upper.z),
        Vector3f(bbox.upper.x, bbox.lower.y, bbox.lower.z),
        Vector3f(bbox.upper.x, bbox.lower.y, bbox.upper.z),
        Vector3f(bbox.upper.x, bbox.upper.y, bbox.lower.z),
        Vector3f(bbox.upper.x, bbox.upper.y, bbox.upper.z),
    })
    {
        ret |= (mat * Vector4f(p, 1)).xyz();
    }
    return ret;
}

RTRC_END
