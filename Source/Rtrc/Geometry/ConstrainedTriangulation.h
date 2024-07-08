#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Geometry/Expansion.h>

RTRC_GEO_BEGIN

// Assumes inputs are not degenerate:
// 1. no coincident points
// 2. there are at least 3 points
// 3. not all points sit on the same line
// New intersections may be generated between constraints.
// They will be represented using homogeneous coordinate and indexed from points.size().
std::vector<Vector3i> ConstrainedTriangulation2D(
    Span<Expansion2>         points,
    Span<Vector2i>           constraints,
    std::vector<Expansion3> &newIntersections);

RTRC_GEO_END
