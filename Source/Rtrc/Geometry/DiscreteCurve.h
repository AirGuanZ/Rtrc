#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Math/Vector3.h>

RTRC_GEO_BEGIN

double ComputeDiscreteCurveLength(Span<Vector3d> points, bool isLoop);

std::vector<Vector3d> UniformSampleDiscreteCurve(Span<Vector3d> points, bool isLoop, uint32_t numSamples);

RTRC_GEO_END
