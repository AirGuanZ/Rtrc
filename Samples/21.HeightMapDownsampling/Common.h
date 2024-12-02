#pragma once

#include <Rtrc/Core/Resource/Image.h>

using namespace Rtrc;

Image<double> Downsample_Native(const Image<double> &source, const Vector2i &targetResolution);

// Downsample a heightmap based on its underlying volume (assume it's sampled using bilinear filtering).
// This is some kind of 'average' w.r.t. the bilinear filtering.
Image<double> Downsample_PreserveVolume(
    const Image<double> &source,
    const Vector2i      &targetResolution,
    uint32_t             iterations,
    double               centerWeight,
    bool                 initUsingNativeDownsampler);
