#pragma once

#include <vector>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>

RTRC_GEO_BEGIN

// Mesh Corefinement:
//     Resolve all intersections between two groups of input triangles. The output triangles will represent the same
//     geometry as the input, but only contain shared edges as intersections.
void Corefine(
    Span<Vector3d>                     inputPositionsA,
    Span<Vector3d>                     inputPositionsB,
    std::vector<Vector3d>             &outputPositionsA,
    std::vector<Vector3d>             &outputPositionsB,
    ObserverPtr<std::vector<uint32_t>> outputFaceToInputFaceA = nullptr,
    ObserverPtr<std::vector<uint32_t>> outputFaceToInputFaceB = nullptr);

RTRC_GEO_END
