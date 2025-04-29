#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector.h>

RTRC_GEO_BEGIN

void GenerateAABB(
    const Vector3d        &lower,
    const Vector3d        &upper,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices);

void GenerateTube(
    Span<Vector3d>         centers,
    bool                   isLoop,
    double                 radius,
    uint32_t               subdivisions,
    bool                   closeEnds,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices);

RTRC_GEO_END
