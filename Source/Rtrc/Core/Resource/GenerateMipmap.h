#pragma once

#include <Rtrc/Core/Resource/Image.h>

RTRC_BEGIN

template<typename T>
Image<T> GenerateNextImageMipmapLevel(const Image<T> &image);

ImageDynamic GenerateNextImageMipmapLevel(const ImageDynamic &image);

uint32_t ComputeFullMipmapChainSize(uint32_t width, uint32_t height);

RTRC_END
