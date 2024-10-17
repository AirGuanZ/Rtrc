#pragma once

#include <vector>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Exact/Vector.h>

RTRC_GEO_BEGIN

// Mesh Corefinement:
//     Resolve intersections between two groups of triangles. The resulting mesh will have the same geometry as the input, without any interesting intersection.
//     Degenerate triangles in the input will be preserved as-is and will not be considered during the intersection computation.
class MeshCorefinement
{
public:

    // Empty indices are implicitly defined as 0, 1, 2, ...
    void Corefine(
        Span<Vector3d> inputPositionsA, Span<uint32_t> inputIndicesA,
        Span<Vector3d> inputPositionsB, Span<uint32_t> inputIndicesB);

    // For each output triangle, track its origin from the input triangles. The results are stored in outputFaceMapA and outputFaceMapB.
    // For example, if input triangle 5 from group A is divided into 2 output triangles (0 and 3), then outputFaceMapA[0] and outputFaceMapA[3] will both be set to 5.
    bool trackFaceMap = false;

    // Collect all edges on the intersection cut line.
    bool trackCutEdges = false;

    // Store output positions in exact homogeneous coordinates, avoiding rounding to double precision.
    // When set to true, the output has no degenerate triangle except those inherited directly from the input.
    // Otherwise, despite exact corefinement, snap rounding may lead to the degenerate triangles or new unresolved intersections.
    // See MeshCleaner::RemoveDegenerateTriangles for removing degenrates.
    bool preserveExactPositions = false;

    bool delaunay = true;

    // Output connectivity
    std::vector<uint32_t> outputIndicesA;
    std::vector<uint32_t> outputIndicesB;

    // Available when preserveExactPositions is false
    std::vector<Vector3d> outputPositionsA;
    std::vector<Vector3d> outputPositionsB;

    // Available when preserveExactPositions is true
    std::vector<Expansion4> outputExactPositionsA;
    std::vector<Expansion4> outputExactPositionsB;

    // Available when trackFaceMap is true
    std::vector<uint32_t> outputFaceMapA;
    std::vector<uint32_t> outputFaceMapB;

    // Available when trackCutEdges is true
    std::vector<Vector2u> outputCutEdgesA;
    std::vector<Vector2u> outputCutEdgesB;
};

// Similar to MeshCorefinement, but designed to operate within a single mesh
class MeshSelfIntersectionRefinement
{
public:

    void Refine(Span<Vector3d> inputPositions, Span<uint32_t> inputIndices);

    bool trackFaceMap = false;
    bool trackCutEdges = false;
    bool preserveExactPositions = false;
    bool delaunay = true;

    std::vector<uint32_t>   outputIndices;
    std::vector<Vector3d>   outputPositions;
    std::vector<Expansion4> outputExactPositions;

    std::vector<uint32_t> outputFaceMap;
    std::vector<Vector2u> outputCutEdges;
};

RTRC_GEO_END
