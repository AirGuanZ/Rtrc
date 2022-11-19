#pragma once

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Utility/Image.h>

RTRC_BEGIN

class CopyContext : public Uncopyable
{
public:

    CopyContext(
        RHI::DevicePtr  device,
        BufferManager  *bufferManager,
        TextureManager *textureManager);

    void UploadBuffer(
        const RC<Buffer> &buffer,
        const void       *initData,
        size_t            offset = 0,
        size_t            size   = 0);

    void UploadTexture2D(
        const RC<Texture> &texture,
        uint32_t             arrayLayer,
        uint32_t             mipLevel,
        const void          *data);

    void UploadTexture2D(
        const RC<Texture> &texture,
        uint32_t             arrayLayer,
        uint32_t             mipLevel,
        const ImageDynamic  &image);

    RC<Buffer> CreateBuffer(
        const RHI::BufferDesc &desc,
        const void            *initData,
        size_t                 initDataOffset = 0,
        size_t                 initDataSize = 0);

    RC<StatefulTexture> CreateTexture2D(
        const RHI::TextureDesc &desc,
        Span<const void *>      imageData);

    RC<StatefulTexture> CreateTexture2D(
        const RHI::TextureDesc &desc,
        Span<ImageDynamic>      images);

    RC<StatefulTexture> LoadTexture2D(
        const std::string    &filename,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        bool                  generateMipLevels);

private:

    struct Batch
    {
        RHI::CommandPoolPtr commandPool;
        RHI::FencePtr fence;
    };

    Batch GetBatch();

    RHI::DevicePtr               device_;
    RHI::QueuePtr                copyQueue_;
    tbb::concurrent_queue<Batch> batches_;

    BufferManager  *bufferManager_;
    TextureManager *textureManager_;
};

RTRC_END
