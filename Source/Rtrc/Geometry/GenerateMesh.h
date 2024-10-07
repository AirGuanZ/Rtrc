#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

void GenerateAABB(
    const Vector3d        &lower,
    const Vector3d        &upper,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices);

void GenerateTube(
    Span<Vector3d>         centers,
    double                 radius,
    int                    segments,
    std::vector<Vector3d> &outputPositions,
    std::vector<uint32_t> &outputIndices);

RTRC_GEO_END
