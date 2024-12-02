#include <avir.h>

#include <Rtrc/Core/Resource/GenerateMipmap.h>

RTRC_BEGIN

namespace GenerateMipmapDetail
{

    template<typename Texel, typename Component, int Channels>
    Image<Texel> ResizeImpl(const Image<Texel> &input, const Vector2u &targetSize)
    {
        Image<Texel> ret(targetSize.x, targetSize.y);
        avir::CImageResizer imageResizer;
        imageResizer.resizeImage(
            reinterpret_cast<const Component *>(input.GetData()),
            static_cast<int>(input.GetWidth()), static_cast<int>(input.GetHeight()), 0,
            reinterpret_cast<Component *>(ret.GetData()),
            static_cast<int>(targetSize.x), static_cast<int>(targetSize.y), Channels, 0);
        return ret;
    }

    template<typename Texel, typename Component, int Channels>
    Image<Texel> GenerateImpl(const Image<Texel> &input)
    {
        const uint32_t newWidth = (input.GetWidth() + 1) >> 1;
        const uint32_t newHeight = (input.GetHeight() + 1) >> 1;
        return ResizeImpl<Texel, Component, Channels>(input, { newWidth, newHeight });
    }

} // namespace GenerateMipmapDetail

template<typename T>
Image<T> GenerateNextImageMipmapLevel(const Image<T> &image)
{
    if(image.GetWidth() <= 1 && image.GetHeight() <= 1)
    {
        throw Exception("Cannot generate next mipmap level for given image (width <= 1 && height <= 1)");
    }
    using Component = typename ImageDetail::Trait<T>::Component;
    constexpr int Channels = ImageDetail::Trait<T>::ComponentCount;
    return GenerateMipmapDetail::GenerateImpl<T, Component, Channels>(image);
}

template<typename T>
Image<T> Resize(const Image<T> &image, const Vector2u &targetSize)
{
    using Component = typename ImageDetail::Trait<T>::Component;
    constexpr int Channels = ImageDetail::Trait<T>::ComponentCount;
    return GenerateMipmapDetail::ResizeImpl<T, Component, Channels>(image, targetSize);
}

ImageDynamic GenerateNextImageMipmapLevel(const ImageDynamic &image)
{
    return image.Match(
        [](std::monostate) -> ImageDynamic
    {
        throw Exception("Cannot generate next mipmap level for empty image");
    },
        [](auto &image) -> ImageDynamic
    {
        return ImageDynamic(GenerateNextImageMipmapLevel(image));
    });
}

uint32_t ComputeFullMipmapChainSize(uint32_t width, uint32_t height)
{
    uint32_t ret = 1;
    while(width > 1 && height > 1)
    {
        width >>= 1;
        height >>= 1;
        ++ret;
    }
    return ret;
}

#define RTRC_EXPLICIT_INSTANTIATE(T) \
    template Image<T> GenerateNextImageMipmapLevel<T>(const Image<T> &); \
    template Image<T> Resize<T>(const Image<T> &, const Vector2u&);

RTRC_EXPLICIT_INSTANTIATE(uint8_t)
RTRC_EXPLICIT_INSTANTIATE(Vector3b)
RTRC_EXPLICIT_INSTANTIATE(Vector4b)
RTRC_EXPLICIT_INSTANTIATE(float)
RTRC_EXPLICIT_INSTANTIATE(double)
RTRC_EXPLICIT_INSTANTIATE(Vector3f)
RTRC_EXPLICIT_INSTANTIATE(Vector4f)

#undef RTRC_EXPLICIT_INSTANTIATE

RTRC_END
