#pragma once

#include <map>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Exact/Vector.h>

RTRC_GEO_BEGIN

// See 'An improved incremental algorithm for constructing restricted Delaunay triangulations'
//
// Assumptions for input validity (non-degenerate):
// 1. No coincident points exist among the inputs.
// 2. There are at least three distinct points provided.
// 3. The points do not all lie on a single straight line.
//
// All points are represented in homogeneous coordinates.
//
// New intersections between constraints may be generated during processing.
// These intersections will be indexed starting from points.size().
//
// The constraints are assumed to define the boundary of the shape. Thus any triangles that include a vertex from
// the 'super triangle' will be discarded as they are outside the relevant area.
class CDT2D
{
public:

    struct ConstraintIntersection
    {
        Expansion3 position;
        int constraint0;
        int constraint1;
    };

    struct Constraint
    {
        int a;
        int b;
        uint32_t mask;
    };

    void Triangulate(Span<Expansion3> points, Span<Constraint> constraints);

    // Ensure the result satisfy constrained delaunay conditions.
    bool delaunay = true;

    // When enabled, in some cases, the delaunay conditions will be computed using regular floating-point numbers
    // instead of exact predicates. The latter is much slower.
    bool approxDelaunay = false;

    // If enabled, for each edge 'e' in the result, the masks of constraints overlapping with 'e' will be bitwise unioned.
    // The resulting mask is stored in `edgeToConstraintMask`.
    // This allows the user to track the correspondence between the original constraints and the resulting edges.
    bool trackConstraintMask = false;

    std::vector<Vector3i> triangles;
    std::vector<ConstraintIntersection> newIntersections;

    // Available when trackConstraintMask is true
    std::map<std::pair<int, int>, uint32_t> edgeToConstraintMask;
};

RTRC_GEO_END
