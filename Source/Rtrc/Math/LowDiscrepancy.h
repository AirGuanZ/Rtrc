#pragma once

#include <vector>

#include <Rtrc/Math/Vector2.h>

RTRC_BEGIN

std::vector<Vector2f> GeneratePoissonDiskSamples(int count, int seed);

RTRC_END
