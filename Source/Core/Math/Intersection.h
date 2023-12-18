#pragma once

#include <Core/Math/AABB.h>

RTRC_BEGIN

// ================================ Declarations ================================

inline float ComputeDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector2f &point, const AABB2f &aabb);

inline float ComputeDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);
inline float ComputeSquaredDistanceBetweenPointAndAABB(const Vector3f &point, const AABB3f &aabb);

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

RTRC_END
