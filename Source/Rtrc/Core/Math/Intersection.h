#pragma once

#include <Rtrc/Core/Math/AABB.h>

RTRC_BEGIN

// ================================ Declarations ================================

inline float ComputeDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);

inline float ComputeDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);

template<typename T>
bool IntersectTriangleAABB(
    Vector3<T> a,
    Vector3<T> b,
    Vector3<T> c,
    const Vector3<T> &lower,
    const Vector3<T> &upper);

// Note that the current implementation may produce unstable results if the ray lies exactly
// on the plane of a bounding box face.
inline bool IntersectRayBox(
    const Vector3f &origin, const Vector3f &invDir, float t0, float t1, const float *boundingBox);

// Note: the current implementation is not watertight
inline bool IntersectRayTriangle(
    const Vector3f &o,
    const Vector3f &d,
    float           t0,
    float           t1,
    const Vector3f &a,
    const Vector3f &ab,
    const Vector3f &ac,
    float          &outT,
    Vector2f       &uv);

// ================================ Implementations ================================

inline float ComputeDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb)
{
    const Vector2f closestPoint = Max(aabb.lower, Min(point, aabb.upper));
    return LengthSquare(closestPoint - point);
}

inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb)
{
    return std::sqrt(ComputeDistanceBetweenPointAndAABB(point, aabb));
}

inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb)
{
    const Vector3f closestPoint = Max(aabb.lower, Min(point, aabb.upper));
    return LengthSquare(closestPoint - point);
}

inline float ComputeDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb)
{
    return std::sqrt(ComputeDistanceBetweenPointAndAABB(point, aabb));
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

inline bool IntersectRayBox(
    const Vector3f &origin, const Vector3f &invDir, float t0, float t1, const float *boundingBox)
{
    const float nx = invDir[0] * (boundingBox[0] - origin[0]);
    const float ny = invDir[1] * (boundingBox[1] - origin[1]);
    const float nz = invDir[2] * (boundingBox[2] - origin[2]);

    const float fx = invDir[0] * (boundingBox[3] - origin[0]);
    const float fy = invDir[1] * (boundingBox[4] - origin[1]);
    const float fz = invDir[2] * (boundingBox[5] - origin[2]);

    t0 = (std::max)(t0, (std::min)(nx, fx));
    t0 = (std::max)(t0, (std::min)(ny, fy));
    t0 = (std::max)(t0, (std::min)(nz, fz));

    t1 = (std::min)(t1, (std::max)(nx, fx));
    t1 = (std::min)(t1, (std::max)(ny, fy));
    t1 = (std::min)(t1, (std::max)(nz, fz));

    return t0 <= t1;
}

inline bool IntersectRayTriangle(
    const Vector3f &o,
    const Vector3f &d,
    float           t0,
    float           t1,
    const Vector3f &a,
    const Vector3f &ab,
    const Vector3f &ac,
    float          &outT,
    Vector2f       &outUV)
{
    const Vector3f s1 = Cross(d, ac);
    const float div = Dot(s1, ab);
    if(div == 0.0f)
    {
        return false;
    }
    const float invDiv = 1 / div;

    const Vector3f ao = o - a;
    const float alpha = Dot(ao, s1) * invDiv;
    if(alpha < 0)
    {
        return false;
    }

    const Vector3f s2 = Cross(ao, ab);
    const float beta = Dot(d, s2) * invDiv;
    if(beta < 0 || alpha + beta > 1)
    {
        return false;
    }

    const float t = Dot(ac, s2) * invDiv;
    if(t < t0 || t > t1)
    {
        return false;
    }

    outT = t;
    outUV = Vector2f(alpha, beta);
    return true;
}

RTRC_END
