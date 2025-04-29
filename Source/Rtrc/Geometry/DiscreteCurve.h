#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Frame.h>
#include <Rtrc/Core/Math/Vector.h>

RTRC_GEO_BEGIN

double ComputeDiscreteCurveLength(Span<Vector3d> points, bool isLoop);

std::vector<Vector3d> UniformSampleDiscreteCurve(Span<Vector3d> points, bool isLoop, uint32_t numSamples);

// Hanson, A.J., & Ma, H. (1995). Parallel Transport Approach to Curve Framing.
std::vector<Framed> ComputeParallelTransportFrame(Span<Vector3d> points, bool isLoop);

RTRC_GEO_END
