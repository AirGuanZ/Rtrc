#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

template<typename T>
void ComputeAngleAveragedNormals(
    Span<Vector3<T>>    positions,
    Span<uint32_t>      indices,
    MutSpan<Vector3<T>> outNormals);

RTRC_GEO_END

#include <Rtrc/Geometry/Utility.inl>
