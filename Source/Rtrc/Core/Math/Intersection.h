#pragma once

#include <Rtrc/Core/Math/AABB.h>

RTRC_BEGIN

// ================================ Declarations ================================

inline float ComputeDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);

inline float ComputeDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);

inline bool IntersectionTriangleAABB(
    Vector3f a,
    Vector3f b,
    Vector3f c,
    const Vector3f &lower,
    const Vector3f &upper);

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
inline bool IntersectionTriangleAABB(
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
        return triMax < aabbMin || aabbMax < triMin;
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

RTRC_END
