#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Geometry/Expansion.h>

RTRC_GEO_BEGIN

// See 'An improved incremental algorithm for constructing restricted Delaunay triangulations'
//
// Assumptions for input validity (non-degenerate):
// 1. No coincident points exist among the inputs.
// 2. There are at least three distinct points provided.
// 3. The points do not all lie on a single straight line.
//
// New intersections between constraints may be generated during processing.
// These intersections will be represented using homogeneous coordinates and indexed starting from points.size().
//
// The constraints are assumed to define the boundary of the shape. Thus any triangles that include a vertex from
// the 'super triangle' will be discarded as they are outside the relevant area.
std::vector<Vector3i> ConstrainedTriangulation2D(
    Span<Expansion2>         points,
    Span<Vector2i>           constraints,
    std::vector<Expansion3> &newIntersections,
    bool                     delaunay);

RTRC_GEO_END
