#pragma once

#include <vector>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_GEO_BEGIN

// Mesh Corefinement:
//     Resolve intersections between two groups of triangles. The resulting mesh will only contain shared edges as intersections, preserving the original geometry.
//     Degenerate triangles in the input will be preserved as-is and will not be considered during the intersection computation.
class MeshCorefinement
{
public:

    // Empty indices are implicitly defined as 0,1, 2, ...
    void Corefine(
        Span<Vector3d> inputPositionsA, Span<uint32_t> inputIndicesA,
        Span<Vector3d> inputPositionsB, Span<uint32_t> inputIndicesB);

    // For each output triangle, track its origin from the input triangles. The results are stored in outputFaceMapA and outputFaceMapB.
    // For example, if input triangle number 5 from group A is divided into 2 output triangles {0, 3}, then outputFaceMapA[0] and outputFaceMapA[3] will both be set to 5.
    bool trackFaceMap = false;

    // Collect all edges on the intersection cut line
    bool trackCutEdges = false;

    // Store output positions in exact homogeneous coordinates, avoiding rounding to double precision.
    // When set to true, the output excludes any degenerate triangles, except for those inherited directly from the input.
    // If not enabled, despite exact corefinement, snap rounding may lead to the degenerate triangles or new, unresolved intersections.
    bool preserveExactPositions = false;

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

RTRC_GEO_END
