#pragma once

#include <vector>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template<typename T>
std::vector<T> GeneratePoissonDiskSamples(int count, int seed);

RTRC_END
