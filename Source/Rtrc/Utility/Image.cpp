#include <array>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>
#include <tinyexr.h>

#include <Rtrc/Utility/Image.h>
#include <Rtrc/Utility/ScopeGuard.h>
#include <Rtrc/Utility/String.h>

RTRC_BEGIN

namespace ImageDetail
{

    void SavePNG(
        const std::string &filename,
        int                width,
        int                height,
        int                dataChannels,
        const uint8_t *data)
    {
        if(!stbi_write_png(filename.c_str(), width, height, dataChannels, data, 0))
        {
            throw Exception("failed to write png file: " + filename);
        }
    }
    
    void SaveJPG(
        const std::string &filename,
        int                width,
        int                height,
        int                dataChannels,
        const uint8_t     *data)
    {
        if(!stbi_write_jpg(filename.c_str(), width, height, dataChannels, data, 0))
        {
            throw Exception("failed to write jpg file: " + filename);
        }
    }
    
    void SaveHDR(
        const std::string &filename,
        int                width,
        int                height,
        const float       *data)
    {
        if(!stbi_write_hdr(filename.c_str(), width, height, 3, data))
        {
            throw Exception("failed to write hdr file: " + filename);
        }
    }

    void SaveEXR(
        const std::string &filename,
        int                width,
        int                height,
        const float       *data)
    {
        EXRHeader header;
        InitEXRHeader(&header);

        EXRImage image;
        InitEXRImage(&image);

        image.num_channels = 3;
        std::vector channels(3, std::vector<float>(width * height));
        for(int i = 0; i < width * height; ++i)
        {
            channels[0][i] = data[3 * i + 0];
            channels[1][i] = data[3 * i + 1];
            channels[2][i] = data[3 * i + 2];
        }

        std::array channelPtrs = {
            channels[2].data(),
            channels[1].data(),
            channels[0].data()
        };

        image.images = reinterpret_cast<unsigned char **>(channelPtrs.data());
        image.width = width;
        image.height = height;

        header.num_channels = 3;

        std::vector<EXRChannelInfo> headerChannels(3);
        header.channels = headerChannels.data();
        strcpy_s(header.channels[0].name, 255, "B");
        strcpy_s(header.channels[1].name, 255, "G");
        strcpy_s(header.channels[2].name, 255, "R");

        std::vector<int> headerPixelTypes(3);
        std::vector<int> headerRequestedPixelTypes(3);
        header.pixel_types = headerPixelTypes.data();
        header.requested_pixel_types = headerRequestedPixelTypes.data();
        for(int i = 0; i < 3; ++i)
        {
            header.pixel_types[i]           = TINYEXR_PIXELTYPE_FLOAT;
            header.requested_pixel_types[i] = TINYEXR_PIXELTYPE_FLOAT;
        }

        const char *err;
        const int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
        if(ret != TINYEXR_SUCCESS)
        {
            std::string msg = "failed to save exr file: " + filename;
            if(err)
            {
                msg += std::string(". ") + err;
                FreeEXRErrorMessage(err);
            }
            throw Exception(msg);
        }
    }
    
    std::vector<uint8_t> LoadPNG(
        const std::string &filename,
        int               *width,
        int               *height,
        int               *channels)
    {
        std::FILE *file = std::fopen(filename.c_str(), "rb");
        if(!file)
        {
            throw Exception("failed to open file: " + filename);
        }
        RTRC_SCOPE_EXIT{ std::fclose(file); };

        int x, y, c;
        auto bytes = stbi_load_from_file(file, &x, &y, &c, 0);
        if(!bytes)
        {
            throw Exception("failed to load image: " + filename);
        }
        RTRC_SCOPE_EXIT{ stbi_image_free(bytes); };

        if(width)
        {
            *width = x;
        }
        if(height)
        {
            *height = y;
        }
        if(channels)
        {
            *channels = c;
        }
        return std::vector(bytes, bytes + x * y * c);
    }

    std::vector<uint8_t> LoadJPG(
        const std::string &filename,
        int               *width,
        int               *height,
        int               *channels)
    {
        return LoadPNG(filename, width, height, channels);
    }

    std::vector<Vector3f> LoadHDR(
        const std::string &filename,
        int               *width,
        int               *height)
    {
        std::FILE *file = std::fopen(filename.c_str(), "rb");
        if(!file)
        {
            throw Exception("failed to open file: " + filename);
        }
        RTRC_SCOPE_EXIT{ std::fclose(file); };

        int x, y, c;
        auto floats = stbi_loadf_from_file(file, &x, &y, &c, 3);
        if(!floats)
        {
            throw Exception("failed to load image: " + filename);
        }
        RTRC_SCOPE_EXIT{ stbi_image_free(floats); };
        
        assert(x > 0 && y > 0);
        if(width)
        {
            *width = x;
        }
        if(height)
        {
            *height = y;
        }

        std::vector<Vector3f> result(x * y);
        std::memcpy(result.data(), floats, sizeof(float) * x * y * 3);
        return result;
    }

    ImageFormat InferFormat(const std::string &filename)
    {
        auto ext = std::filesystem::path(filename).extension().string();
        ToUpper_(ext);
        if(ext == ".PNG")
        {
            return ImageFormat::PNG;
        }
        if(ext == ".JPG" || ext == ".JPEG")
        {
            return ImageFormat::JPG;
        }
        if(ext == ".HDR")
        {
            return ImageFormat::HDR;
        }
        if(ext == ".EXR")
        {
            return ImageFormat::EXR;
        }
        throw Exception("unknown image format: " + ext);
    }

} // namespace image_detail

void ImageDynamic::Swap(ImageDynamic &other) noexcept
{
    image_.swap(other.image_);
}

ImageDynamic::operator bool() const
{
    return image_.Match(
        [](std::monostate) { return false; },
        [](auto &image) { return static_cast<bool>(image); });
}

ImageDynamic ImageDynamic::To(TexelType newTexelType) const
{
    return image_.Match(
        [](std::monostate) -> ImageDynamic
    {
        throw Exception("ImageDynamic::to: empty source image");
    },
        [&](auto &image)
    {
        switch(newTexelType)
        {
        case U8x1:  return ImageDynamic(image.template To<uint8_t>());
        case U8x3:  return ImageDynamic(image.template To<Vector3b>());
        case U8x4:  return ImageDynamic(image.template To<Vector4b>());
        case F32x1: return ImageDynamic(image.template To<float>());
        case F32x3: return ImageDynamic(image.template To<Vector3f>());
        case F32x4: return ImageDynamic(image.template To<Vector4f>());
        }
        Unreachable();
    });
}

uint32_t ImageDynamic::GetWidth() const
{
    return image_.Match(
        [](std::monostate) { return 0u; },
        [](auto &image) { return image.GetWidth(); });
}

uint32_t ImageDynamic::GetHeight() const
{
    return image_.Match(
        [](std::monostate) { return 0u; },
        [](auto &image) { return image.GetHeight(); });
}

void ImageDynamic::Save(const std::string &filename, ImageFormat format) const
{
    image_.Match(
        [](std::monostate)
    {
        throw Exception("ImageDynamic::save: empty image");
    },
        [&](auto &image)
    {
        image.Save(filename, format);
    });
}

ImageDynamic ImageDynamic::Load(const std::string &filename, ImageFormat format)
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
        int width, height, channels;
        const auto data = format == ImageFormat::PNG ?
            LoadPNG(filename, &width, &height, &channels) :
            LoadJPG(filename, &width, &height, &channels);
        if(channels == 1)
        {
            auto image = Image<uint8_t>(width, height);
            std::memcpy(image.GetData(), data.data(), sizeof(uint8_t) * data.size());
            return ImageDynamic(std::move(image));
        }
        if(channels == 3)
        {
            auto image = Image<Vector3b>(width, height);
            std::memcpy(image.GetData(), data.data(), sizeof(uint8_t) * data.size());
            return ImageDynamic(std::move(image));
        }
        if(channels == 4)
        {
            auto image = Image<Vector4b>(width, height);
            std::memcpy(image.GetData(), data.data(), sizeof(uint8_t) * data.size());
            return ImageDynamic(std::move(image));
        }
        throw Exception("ImageDynamic::load: unsupported channels: " + std::to_string(channels));
    }
    case ImageFormat::HDR:
    {
        int width, height;
        auto data = LoadHDR(filename, &width, &height);
        auto image = Image<Vector3f>(width, height);
        std::memcpy(image.GetData(), data.data(), sizeof(Vector3f) * data.size());
        return ImageDynamic(image);
    }
    case ImageFormat::EXR:
        throw Exception("ImageDynamic::load: EXR is not supported");
    case ImageFormat::Auto:
        Unreachable();
    }
    Unreachable();
}

RTRC_END
