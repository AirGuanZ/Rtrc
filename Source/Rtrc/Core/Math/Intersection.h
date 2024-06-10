#pragma once

#include <Rtrc/Core/Math/AABB.h>

RTRC_BEGIN

// ================================ Declarations ================================

inline float ComputeDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);

inline float ComputeDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);

inline bool IntersectTriangleAABB(
    Vector3f a,
    Vector3f b,
    Vector3f c,
    const Vector3f &lower,
    const Vector3f &upper);

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
inline bool IntersectTriangleAABB(
    Vector3f a,
    Vector3f b,
    Vector3f c,
    const Vector3f &lower,
    const Vector3f &upper)
{
    // Move aabb to original

    const Vector3f aabbCenter = 0.5f * (lower + upper);
    const Vector3f aabbHalfSize = 0.5f * (upper - lower);
    a -= aabbCenter;
    b -= aabbCenter;
    c -= aabbCenter;

    // Test intersection when projected onto given axis

    auto IntersectProjected = [&](const Vector3f &axis)
    {
        const float pa = Dot(a, axis);
        const float pb = Dot(b, axis);
        const float pc = Dot(c, axis);
        const float triMin = (std::min)({ pa, pb, pc });
        const float triMax = (std::max)({ pa, pb, pc });
        const float aabbMax = AddReduce(Abs(axis * aabbHalfSize));
        const float aabbMin = -aabbMax;
        const bool missed = triMax < aabbMin || aabbMax < triMin;
        return !missed;
    };

    // Edge vectors & edge normal vectors

    const Vector3f ab = b - a;
    const Vector3f bc = c - b;
    const Vector3f ca = a - c;

    const Vector3f e0 = ab - Dot(ab, bc) * bc / LengthSquare(bc);
    const Vector3f e1 = bc - Dot(bc, ca) * ca / LengthSquare(ca);
    const Vector3f e2 = ca - Dot(ca, ab) * ab / LengthSquare(ab);

    // Primary axis

    if(!IntersectProjected(Cross(bc, ca)) ||
       !IntersectProjected({ 1, 0, 0 }) ||
       !IntersectProjected({ 0, 1, 0 }) ||
       !IntersectProjected({ 0, 0, 1 }))
    {
        return false;
    }

    // Crossing axis

    if(!IntersectProjected(Cross({ 1, 0, 0 }, e0)) ||
       !IntersectProjected(Cross({ 1, 0, 0 }, e1)) ||
       !IntersectProjected(Cross({ 1, 0, 0 }, e2)))
    {
        return false;
    }

    if(!IntersectProjected(Cross({ 0, 1, 0 }, e0)) ||
       !IntersectProjected(Cross({ 0, 1, 0 }, e1)) ||
       !IntersectProjected(Cross({ 0, 1, 0 }, e2)))
    {
        return false;
    }

    if(!IntersectProjected(Cross({ 0, 0, 1 }, e0)) ||
       !IntersectProjected(Cross({ 0, 0, 1 }, e1)) ||
       !IntersectProjected(Cross({ 0, 0, 1 }, e2)))
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
