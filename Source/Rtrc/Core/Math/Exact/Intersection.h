#pragma once

#include <Rtrc/Core/Math/Exact/Expansion.h>
#include <Rtrc/Core/Math/Exact/Vector.h>

RTRC_BEGIN

// Compute the normal 'n' of triangle abc and return the index of the component of 'n' with the maximum absolute value.
int FindMaximalNormalAxis(const Vector3f &a, const Vector3f &b, const Vector3f &c);
int FindMaximalNormalAxis(const Vector3d &a, const Vector3d &b, const Vector3d &c);
int FindMaximalNormalAxis(const Expansion4 &a, const Expansion4 &b, const Expansion4 &c);

// Compute intersection between 2D lines ab and cd. The intersection must exist uniquely.
Expansion3 IntersectLine2D(const Expansion3 &a, const Expansion3 &b, const Expansion3 &c, const Expansion3 &d);

// Calculate the intersection point of two intersecting lines (ab, cd), returning the result in homogeneous coordinates.
// ab must not be colinear with cd.
Expansion4 IntersectLine3D(const Vector3d &a, const Vector3d &b, const Vector3d &c, const Vector3d &d, int projectAxis = -1);
Expansion4 IntersectLine3D(const Expansion4 &a, const Expansion4 &b, const Expansion4 &c, const Expansion4 &d, int projectAxis = -1);

// Calculate the intersection of line segment pq with the interior of triangle abc.
// The result is in homogeneous coordinates.
// The caller must ensure that there is exactly one such intersection.
// This implies that pq is neither coplanar with abc nor intersects any edge of abc.
Expansion4 IntersectLineTriangle3D(
    const Vector3d &p, const Vector3d &q,
    const Vector3d &a, const Vector3d &b, const Vector3d &c);

RTRC_END
