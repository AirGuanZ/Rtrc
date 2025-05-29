#pragma once

#include <Rtrc/Core/Math/AABB.h>

RTRC_BEGIN

// ================================ Declarations ================================

template<typename T>
T ComputeDistanceBetweenPointAndAABB(const Vector2<T> &point, const AABB2<T> &aabb);
template<typename T>
T ComputeSquaredDistanceBetweenPointAndAABB(const Vector2<T> &point, const AABB2<T> &aabb);

template<typename T>
T ComputeDistanceBetweenPointAndAABB(const Vector3<T> &point, const AABB3<T> &aabb);
template<typename T>
T ComputeSquaredDistanceBetweenPointAndAABB(const Vector3<T> &point, const AABB3<T> &aabb);

template<typename T>
bool IntersectTriangleAABB(
    Vector3<T> a,
    Vector3<T> b,
    Vector3<T> c,
    const Vector3<T> &lower,
    const Vector3<T> &upper);

// Note that the current implementation may produce unstable results if the ray lies exactly
// on the plane of a bounding box face.
// boundingBox[0/1/2]: lower
// boundingBox[2/3/4]: upper
template<typename T>
bool IntersectRayBox(
    const Vector3<T> &origin,
    const Vector3<T> &invDir,
    T                 t0,
    T                 t1,
    T                &outT0,
    T                &outT1,
    const T          *boundingBox);

template<typename T>
bool IntersectRayBox(
    const Vector3<T> &origin,
    const Vector3<T> &invDir,
    T                 t0,
    T                 t1,
    const T          *boundingBox);

// Note: the current implementation is not watertight
template<typename T>
bool IntersectRayTriangle(
    const Vector3<T> &o,
    const Vector3<T> &d,
    T                 t0,
    T                 t1,
    const Vector3<T> &a,
    const Vector3<T> &ab,
    const Vector3<T> &ac,
    float            &outT,
    Vector2<T>       &outUV);

// ================================ Implementations ================================

template<typename T>
T ComputeSquaredDistanceBetweenPointAndAABB(const Vector2<T> &point, const AABB2<T> &aabb)
{
    const Vector2<T> closestPoint = Rtrc::Max(aabb.lower, Rtrc::Min(point, aabb.upper));
    return Rtrc::LengthSquare(closestPoint - point);
}

template<typename T>
T ComputeDistanceBetweenPointAndAABB(const Vector2<T> &point, const AABB2<T> &aabb)
{
    return std::sqrt(Rtrc::ComputeSquaredDistanceBetweenPointAndAABB(point, aabb));
}

template<typename T>
T ComputeSquaredDistanceBetweenPointAndAABB(const Vector3<T> &point, const AABB3<T> &aabb)
{
    const Vector3<T> closestPoint = Rtrc::Max(aabb.lower, Rtrc::Min(point, aabb.upper));
    return Rtrc::LengthSquare(closestPoint - point);
}

template<typename T>
T ComputeDistanceBetweenPointAndAABB(const Vector3<T> &point, const AABB3<T> &aabb)
{
    return std::sqrt(Rtrc::ComputeSquaredDistanceBetweenPointAndAABB(point, aabb));
}

// Intersection test based on separating axis theorem
template<typename T>
bool IntersectTriangleAABB(
    Vector3<T>        a,
    Vector3<T>        b,
    Vector3<T>        c,
    const Vector3<T> &lower,
    const Vector3<T> &upper)
{
    // Move aabb to original

    const Vector3<T> aabbCenter = T(0.5) * (lower + upper);
    const Vector3<T> aabbHalfSize = T(0.5) * (upper - lower);
    a -= aabbCenter;
    b -= aabbCenter;
    c -= aabbCenter;

    // Test intersection when projected onto given axis

    auto IntersectProjected = [&](const Vector3<T> &axis)
    {
        const float pa = Rtrc::Dot(a, axis);
        const float pb = Rtrc::Dot(b, axis);
        const float pc = Rtrc::Dot(c, axis);
        const float triMin = (std::min)({ pa, pb, pc });
        const float triMax = (std::max)({ pa, pb, pc });
        const float aabbMax = Rtrc::AddReduce(Rtrc::Abs(axis * aabbHalfSize));
        const float aabbMin = -aabbMax;
        const bool missed = triMax < aabbMin || aabbMax < triMin;
        return !missed;
    };

    // Edge vectors & edge normal vectors

    const Vector3<T> ab = b - a;
    const Vector3<T> bc = c - b;
    const Vector3<T> ca = a - c;

    const Vector3<T> e0 = ab - Rtrc::Dot(ab, bc) * bc / Rtrc::LengthSquare(bc);
    const Vector3<T> e1 = bc - Rtrc::Dot(bc, ca) * ca / Rtrc::LengthSquare(ca);
    const Vector3<T> e2 = ca - Rtrc::Dot(ca, ab) * ab / Rtrc::LengthSquare(ab);

    // Primary axis

    if(!IntersectProjected(Rtrc::Cross(bc, ca)) ||
       !IntersectProjected({ 1, 0, 0 }) ||
       !IntersectProjected({ 0, 1, 0 }) ||
       !IntersectProjected({ 0, 0, 1 }))
    {
        return false;
    }

    // Crossing axis

    if(!IntersectProjected(Rtrc::Cross({ 1, 0, 0 }, e0)) ||
       !IntersectProjected(Rtrc::Cross({ 1, 0, 0 }, e1)) ||
       !IntersectProjected(Rtrc::Cross({ 1, 0, 0 }, e2)))
    {
        return false;
    }

    if(!IntersectProjected(Rtrc::Cross({ 0, 1, 0 }, e0)) ||
       !IntersectProjected(Rtrc::Cross({ 0, 1, 0 }, e1)) ||
       !IntersectProjected(Rtrc::Cross({ 0, 1, 0 }, e2)))
    {
        return false;
    }

    if(!IntersectProjected(Rtrc::Cross({ 0, 0, 1 }, e0)) ||
       !IntersectProjected(Rtrc::Cross({ 0, 0, 1 }, e1)) ||
       !IntersectProjected(Rtrc::Cross({ 0, 0, 1 }, e2)))
    {
        return false;
    }

    return true;
}

template<typename T>
bool IntersectRayBox(
    const Vector3<T> &origin,
    const Vector3<T> &invDir,
    T                 t0,
    T                 t1,
    T                &outT0,
    T                &outT1,
    const T          *boundingBox)
{
    const T nx = invDir[0] * (boundingBox[0] - origin[0]);
    const T ny = invDir[1] * (boundingBox[1] - origin[1]);
    const T nz = invDir[2] * (boundingBox[2] - origin[2]);

    const T fx = invDir[0] * (boundingBox[3] - origin[0]);
    const T fy = invDir[1] * (boundingBox[4] - origin[1]);
    const T fz = invDir[2] * (boundingBox[5] - origin[2]);

    t0 = (std::max)(t0, (std::min)(nx, fx));
    t0 = (std::max)(t0, (std::min)(ny, fy));
    t0 = (std::max)(t0, (std::min)(nz, fz));

    t1 = (std::min)(t1, (std::max)(nx, fx));
    t1 = (std::min)(t1, (std::max)(ny, fy));
    t1 = (std::min)(t1, (std::max)(nz, fz));

    outT0 = t0;
    outT1 = t1;

    return t0 <= t1;
}

template<typename T>
bool IntersectRayBox(
    const Vector3<T> &origin,
    const Vector3<T> &invDir,
    T                 t0,
    T                 t1,
    const T          *boundingBox)
{
    T outT0, outT1;
    return Rtrc::IntersectRayBox(origin, invDir, t0, t1, outT0, outT1, boundingBox);
}

template<typename T>
bool IntersectRayTriangle(
    const Vector3<T> &o,
    const Vector3<T> &d,
    T                 t0,
    T                 t1,
    const Vector3<T> &a,
    const Vector3<T> &ab,
    const Vector3<T> &ac,
    float            &outT,
    Vector2<T>       &outUV)
{
    const Vector3<T> s1 = Cross(d, ac);
    const T div = Dot(s1, ab);
    if(div == T(0))
    {
        return false;
    }
    const T invDiv = 1 / div;

    const Vector3<T> ao = o - a;
    const T alpha = Dot(ao, s1) * invDiv;
    if(alpha < 0)
    {
        return false;
    }

    const Vector3<T> s2 = Cross(ao, ab);
    const T beta = Dot(d, s2) * invDiv;
    if(beta < 0 || alpha + beta > 1)
    {
        return false;
    }

    const T t = Dot(ac, s2) * invDiv;
    if(t < t0 || t > t1)
    {
        return false;
    }

    outT = t;
    outUV = Vector2<T>(alpha, beta);
    return true;
}

RTRC_END
