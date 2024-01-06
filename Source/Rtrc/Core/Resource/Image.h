#pragma once

#include <array>
#include <cassert>
#include <filesystem>
#include <vector>

#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/Unreachable.h>
#include <Rtrc/Core/Variant.h>

RTRC_BEGIN

enum class ImageFormat
{
    Auto,
    PNG,
    JPG,
    HDR,
    EXR
};

template<typename T>
class Image
{
public:

    using Texel = T;

    Image();
    Image(uint32_t width, uint32_t height);
    Image(const Image &other);
    Image(Image &&other) noexcept;

    Image &operator=(const Image &other);
    Image &operator=(Image &&other) noexcept;

    ~Image() = default;

    void Swap(Image &other) noexcept;

    operator bool() const;

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    int32_t GetSignedWidth() const { return static_cast<int32_t>(GetWidth()); }
    int32_t GetSignedHeight() const { return static_cast<int32_t>(GetHeight()); }

          Texel &operator()(uint32_t x, uint32_t y);
    const Texel &operator()(uint32_t x, uint32_t y) const;

    template<typename U>
    Image<U> To() const;

    void Pow_(float v);
    Image Pow(float v) const;

    auto begin() { return data_.begin(); }
    auto end()   { return data_.end(); }

    auto begin() const { return data_.begin(); }
    auto end()   const { return data_.end(); }

    auto GetData()       { return data_.data(); }
    auto GetData() const { return data_.data(); }

    void Save(const std::string &filename, ImageFormat format = ImageFormat::Auto) const;
    static Image Load(const std::string &filename, ImageFormat format = ImageFormat::Auto);

    std::vector<unsigned char> GetRowAlignedData(size_t rowAlignment) const;
    static Image FromRawData(const void *data, uint32_t width, uint32_t height, size_t rowSize);

private:

    uint32_t width_;
    uint32_t height_;
    std::vector<Texel> data_;
};

class ImageDynamic
{
public:

    enum TexelType
    {
        U8x1,
        U8x3,
        U8x4,
        F32x1,
        F32x2,
        F32x3,
        F32x4
    };

    ImageDynamic() = default;

    template<typename T>
    ImageDynamic(const Image<T> &image);

    template<typename T>
    ImageDynamic(Image<T> &&image);

    void Swap(ImageDynamic &other) noexcept;

    operator bool() const;

    template<typename T>
    bool Is() const;

    template<typename T>
    T &As();

    template<typename T>
    const T &As() const;

    template<typename T>
    T *AsIf();

    template<typename T>
    const T *AsIf() const;

    ImageDynamic To(TexelType newTexelType) const;

    template<typename T>
    ImageDynamic To() const;

    uint32_t GetWidth() const;

    uint32_t GetHeight() const;

    template<typename...Vs>
    auto Match(Vs &&...vs) const;

    template<typename...Vs>
    auto Match(Vs &&...vs);

    void Save(const std::string &filename, ImageFormat format = ImageFormat::Auto) const;

    static ImageDynamic Load(const std::string &filename, ImageFormat format = ImageFormat::Auto);

private:

    Variant<
        std::monostate,
        Image<uint8_t>,
        Image<Vector3b>,
        Image<Vector4b>,
        Image<float>,
        Image<Vector2f>,
        Image<Vector3f>,
        Image<Vector4f>> image_ = std::monostate{};
};

// ========================== impl ==========================

namespace ImageDetail
{
    
    template<typename T>
    struct Trait;
    template<>
    struct Trait<uint8_t>
    {
        using Component = uint8_t;
        static constexpr int ComponentCount = 1;
    };
    template<>
    struct Trait<Vector3b>
    {
        using Component = uint8_t;
        static constexpr int ComponentCount = 3;
    };
    template<>
    struct Trait<Vector4b>
    {
        using Component = uint8_t;
        static constexpr int ComponentCount = 4;
    };
    template<>
    struct Trait<float>
    {
        using Component = float;
        static constexpr int ComponentCount = 1;
    };
    template<>
    struct Trait<Vector2f>
    {
        using Component = float;
        static constexpr int ComponentCount = 2;
    };
    template<>
    struct Trait<Vector3f>
    {
        using Component = float;
        static constexpr int ComponentCount = 3;
    };
    template<>
    struct Trait<Vector4f>
    {
        using Component = float;
        static constexpr int ComponentCount = 4;
    };

    template<typename To, typename From>
    To ToComponent(From from)
    {
        if constexpr(std::is_same_v<To, From>)
        {
            return from;
        }
        else if constexpr(std::is_same_v<To, uint8_t>)
        {
            static_assert(std::is_same_v<From, float>);
            return static_cast<uint8_t>(
                (std::min)(static_cast<int>(from * 256), 255));
        }
        else
        {
            static_assert(std::is_same_v<To, float>);
            static_assert(std::is_same_v<From, uint8_t>);
            return from / 255.0f;
        }
    }

    template<typename T, int SrcComps, int DstComps>
    std::array<T, DstComps> ConvertComps(const T *src)
    {
        constexpr T DEFAULT_ALPHA = static_cast<T>(std::is_same_v<T, uint8_t> ? 255 : 1);

        std::array<T, DstComps> ret = {};
        if constexpr(SrcComps == DstComps)
        {
            for(int i = 0; i < SrcComps; ++i)
                ret[i] = src[i];
        }
        else if constexpr(SrcComps == 1)
        {
            static_assert(DstComps == 2 || DstComps == 3 || DstComps == 4);
            ret[0] = src[0];
            ret[1] = src[0];
            if constexpr(DstComps > 2)
            {
                ret[2] = src[0];
            }
            if constexpr(DstComps > 3)
            {
                ret[3] = DEFAULT_ALPHA;
            }
        }
        else if constexpr(SrcComps == 2)
        {
            ret[0] = src[0];
            if constexpr(DstComps == 3)
            {
                ret[1] = src[0];
                ret[2] = src[1];
            }
            else if constexpr(DstComps == 4)
            {
                ret[1] = src[0];
                ret[2] = src[0];
                ret[3] = src[1];
            }
        }
        else if constexpr(SrcComps == 3)
        {
            if constexpr(DstComps == 1)
            {
                ret[0] = src[0];
            }
            else if constexpr(DstComps == 2)
            {
                ret[0] = src[0];
                ret[1] = src[1];
                ret[2] = src[2];
                ret[3] = DEFAULT_ALPHA;
            }
            else
            {
                static_assert(DstComps == 4);
                ret[0] = src[0];
                ret[1] = src[1];
                ret[2] = src[2];
                ret[3] = DEFAULT_ALPHA;
            }
        }
        else
        {
            static_assert(SrcComps == 4);
            ret[0] = src[0];
            if constexpr(DstComps == 2)
            {
                ret[1] = src[1];
            }
            else if constexpr(DstComps == 3)
            {
                ret[1] = src[1];
                ret[2] = src[2];
            }
        }

        return ret;
    }

    template<typename ToT, typename From>
    ToT To(const From &from)
    {
        if constexpr(std::is_same_v<ToT, From>)
        {
            return from;
        }

        constexpr int FromComps = Trait<From>::ComponentCount;
        constexpr int ToComps = Trait<ToT>::ComponentCount;

        using FromComp = typename Trait<From>::Component;
        using ToComp = typename Trait<ToT>::Component;

        std::array<FromComp, ToComps> convertedChannels =
            ConvertComps<FromComp, FromComps, ToComps>(reinterpret_cast<const FromComp *>(&from));

        std::array<ToComp, ToComps> resultChannels = {};
        for(int i = 0; i < ToComps; ++i)
        {
            resultChannels[i] = ToComponent<ToComp>(convertedChannels[i]);
        }

        ToT result;
        std::memcpy(&result, &resultChannels, sizeof(ToT));
        return result;
    }

    inline float Pow(float a, float b)
    {
        return std::pow(a, b);
    }

    inline Vector3f Pow(const Vector3f &a, float b)
    {
        return Vector3f(std::pow(a.x, b), std::pow(a.y, b), std::pow(a.z, b));
    }

    inline Vector4f Pow(const Vector4f &a, float b)
    {
        return Vector4(
            std::pow(a.x, b), std::pow(a.y, b), std::pow(a.z, b), std::pow(a.w, b));
    }

    inline uint8_t Pow(uint8_t a, float b)
    {
        return To<uint8_t>(Pow(To<float>(a), b));
    }

    inline Vector3b Pow(const Vector3b &a, float b)
    {
        return To<Vector3b>(Pow(To<Vector3f>(a), b));
    }

    inline Vector4b Pow(const Vector4b &a, float b)
    {
        return To<Vector4b>(Pow(To<Vector4f>(a), b));
    }
    
    void SavePNG(
        const std::string &filename,
        int                width,
        int                height,
        int                dataChannels,
        const uint8_t     *data);
    
    void SaveJPG(
        const std::string &filename,
        int                width,
        int                height,
        int                dataChannels,
        const uint8_t     *data);
    
    void SaveHDR(
        const std::string &filename,
        int                width,
        int                height,
        const float       *data);

    void SaveEXR(
        const std::string &filename,
        int                width,
        int                height,
        const float       *data);

    std::vector<uint8_t> LoadPNG(
        const std::string &filename,
        int               *width,
        int               *height,
        int               *channels);

    std::vector<uint8_t> LoadJPG(
        const std::string &filename,
        int               *width,
        int               *height,
        int               *channels);

    std::vector<Vector3f> LoadHDR(
        const std::string &filename,
        int               *width,
        int               *height);

    ImageFormat InferFormat(const std::string &filename);

} // namespace image_detail

template<typename T>
Image<T>::Image()
    : width_(0), height_(0)
{

}

template<typename T>
Image<T>::Image(uint32_t width, uint32_t height)
    : width_(width), height_(height)
{
    assert(width > 0 && height > 0);
    data_.resize(width * height);
}

template<typename T>
Image<T>::Image(const Image &other)
    : width_(other.width_), height_(other.height_)
{
    data_ = other.data_;
}

template<typename T>
Image<T>::Image(Image &&other) noexcept
    : width_(0), height_(0)
{
    this->Swap(other);
}

template<typename T>
Image<T> &Image<T>::operator=(const Image &other)
{
    Image t(other);
    this->Swap(t);
    return *this;
}

template<typename T>
Image<T> &Image<T>::operator=(Image &&other) noexcept
{
    this->Swap(other);
    return *this;
}

template<typename T>
void Image<T>::Swap(Image &other) noexcept
{
    std::swap(width_, other.width_);
    std::swap(height_, other.height_);
    data_.swap(other.data_);
}

template<typename T>
Image<T>::operator bool() const
{
    assert(width_ > 0 == height_ > 0);
    assert(width_ > 0 == !data_.empty());
    return width_ > 0;
}

template<typename T>
uint32_t Image<T>::GetWidth() const
{
    return width_;
}

template<typename T>
uint32_t Image<T>::GetHeight() const
{
    return height_;
}

template<typename T>
typename Image<T>::Texel &Image<T>::operator()(uint32_t x, uint32_t y)
{
    assert(x < width_);
    assert(y < height_);
    const int idx = y * width_ + x;
    return data_[idx];
}

template<typename T>
const typename Image<T>::Texel &Image<T>::operator()(uint32_t x, uint32_t y) const
{
    assert(x < width_);
    assert(y < height_);
    const int idx = y * width_ + x;
    return data_[idx];
}

template<typename T>
template<typename U>
Image<U> Image<T>::To() const
{
    assert(!!*this);
    Image<U> result(width_, height_);
    for(uint32_t i = 0; i < width_ * height_; ++i)
    {
        result.GetData()[i] = ImageDetail::To<U>(data_[i]);
    }
    return result;
}

template<typename T>
Image<T> Image<T>::Pow(float v) const
{
    Image ret = *this;
    ret.Pow_(v);
    return ret;
}

template<typename T>
void Image<T>::Pow_(float v)
{
    for(auto &d : data_)
    {
        d = ImageDetail::Pow(d, v);
    }
}

template<typename T>
void Image<T>::Save(const std::string &filename, ImageFormat format) const
{
    using namespace ImageDetail;

    if(format == ImageFormat::Auto)
    {
        format = InferFormat(filename);
    }

    switch(format)
    {
    case ImageFormat::PNG:
    case ImageFormat::JPG:
    {
        auto saveFunc = format == ImageFormat::PNG ? &SavePNG : &SaveJPG;
        if constexpr(std::is_same_v<T, float>)
        {
            To<uint8_t>().Save(filename, format);
        }
        else if constexpr(std::is_same_v<T, Vector2f>)
        {
            To<Vector4b>().Save(filename, format);
        }
        else if constexpr(std::is_same_v<T, Vector3f>)
        {
            To<Vector3b>().Save(filename, format);
        }
        else if constexpr(std::is_same_v<T, Vector4f>)
        {
            To<Vector4b>().Save(filename, format);
        }
        else if constexpr(std::is_same_v<T, uint8_t>)
        {
            saveFunc(filename, width_, height_, 1, &data_[0]);
        }
        else if constexpr(std::is_same_v<T, Vector3b>)
        {
            saveFunc(filename, width_, height_, 3, &data_[0].x);
        }
        else
        {
            static_assert(std::is_same_v<T, Vector4b>);
            saveFunc(filename, width_, height_, 4, &data_[0].x);
        }
        break;
    }
    case ImageFormat::HDR:
    case ImageFormat::EXR:
    {
        auto saveFunc = format == ImageFormat::HDR ? &SaveHDR : &SaveEXR;
        if constexpr(std::is_same_v<T, Vector3f>)
        {
            saveFunc(filename, width_, height_, &data_[0].x);
        }
        else
        {
            To<Vector3f>().Save(filename, format);
        }
        break;
    }
    case ImageFormat::Auto:
        Unreachable();
    }
}

template<typename T>
Image<T> Image<T>::Load(const std::string &filename, ImageFormat format)
{
    auto imageDynamic = ImageDynamic::Load(filename, format);
    if(auto result = imageDynamic.AsIf<Image<T>>())
    {
        return std::move(*result);
    }
    return imageDynamic.Match(
        [&](std::monostate) -> Image<T>
        {
            throw Exception("failed to load image from " + filename);
        },
        [](auto &image)
        {
            return image.template To<T>();
        });
}

template<typename T>
std::vector<unsigned char> Image<T>::GetRowAlignedData(size_t rowAlignment) const
{
    const size_t rowSize = width_ * sizeof(T);
    const size_t alignedRowSize = UpAlignTo(rowSize, rowAlignment);
    std::vector<unsigned char> ret(alignedRowSize * height_);
    for(uint32_t y = 0;y < height_; ++y)
    {
        auto src = &data_[y * width_];
        auto dst = ret.data() + y * alignedRowSize;
        std::memcpy(dst, src, rowSize);
    }
    return ret;
}

template<typename T>
Image<T> Image<T>::FromRawData(const void *data, uint32_t width, uint32_t height, size_t rowSize)
{
    Image ret(width, height);
    for(uint32_t y = 0; y < height; ++y)
    {
        auto src = static_cast<const unsigned char *>(data) + y * rowSize;
        auto dst = &ret(0, y);
        std::memcpy(dst, src, width * sizeof(T));
    }
    return ret;
}

template<typename T>
ImageDynamic::ImageDynamic(const Image<T> &image)
    : image_(image)
{
    
}

template<typename T>
ImageDynamic::ImageDynamic(Image<T> &&image)
    : image_(std::move(image))
{
    
}

template<typename T>
bool ImageDynamic::Is() const
{
    return image_.Is<T>();
}

template<typename T>
T &ImageDynamic::As()
{
    return *image_.AsIf<T>();
}

template<typename T>
const T &ImageDynamic::As() const
{
    return *image_.AsIf<T>();
}

template<typename T>
T *ImageDynamic::AsIf()
{
    return image_.AsIf<T>();
}

template<typename T>
const T *ImageDynamic::AsIf() const
{
    return image_.AsIf<T>();
}

template<typename T>
ImageDynamic ImageDynamic::To() const
{
    return image_.Match(
        [](std::monostate) -> ImageDynamic
        {
            throw Exception("ImageDynamic::to: empty source image");
        },
        [](auto &image)
        {
            return ImageDynamic(image.template To<T>());
        });
}

template<typename...Vs>
auto ImageDynamic::Match(Vs &&...vs) const
{
    return image_.Match(std::forward<Vs>(vs)...);
}

template<typename ... Vs>
auto ImageDynamic::Match(Vs &&... vs)
{
    return image_.Match(std::forward<Vs>(vs)...);
}

RTRC_END
