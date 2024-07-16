#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

// Helper class for removing degenerates in a mesh.
// Two cases are considered as degenerate triangles:
// 1. When two vertices are coincident: These vertices will be merged, and any triangles containing these vertices will be removed.
// 2. When the three vertices of a triangle are colinear: The triangle will be removed. The longest edge will be split
//    at the central vertex to maintain valid triangulation, if necessary.
// Removing these degenerates doesn't change the underlying geometry.
class MeshCleaner
{
public:

    // Empty indices are implicitly defined as 0, 1, 2, ...
    void RemoveDegenerateTriangles(Span<Vector3d> inputPositions, Span<uint32_t> inputIndices);

    bool trackFaceMap = false;
    bool trackVertexMap = false;
    bool trackInverseVertexMap = false;

    std::vector<Vector3d> outputPositions;
    std::vector<uint32_t> outputIndices;

    // Available when trackFaceMap is set to true
    std::vector<uint32_t> outputFaceMap;

    // Available when trackVertexMap is true.
    // Map output vertex to input vertex. When coincident input vertices are merged, only store one of them.
    std::vector<uint32_t> outputVertexMap;

    // Available when trackInverseVertexMap is true
    // Map input vertex to output vertex. Removed vertices are mapped to UINT32_MAX.
    std::vector<uint32_t> outputInverseVertexMap;
};

RTRC_GEO_END
