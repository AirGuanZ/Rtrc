#pragma once

#include <Rtrc/Core/Resource/Image.h>

RTRC_BEGIN

template<typename T>
Image<T> GenerateNextImageMipmapLevel(const Image<T> &image);

template<typename T>
Image<T> Resize(const Image<T> &image, const Vector2u &targetSize);

ImageDynamic GenerateNextImageMipmapLevel(const ImageDynamic &image);

uint32_t ComputeFullMipmapChainSize(uint32_t width, uint32_t height);

RTRC_END
