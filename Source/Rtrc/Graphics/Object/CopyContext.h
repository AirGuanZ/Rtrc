#pragma once

#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>
#include <Rtrc/Graphics/Object/Texture.h>

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
        const RC<Texture2D> &texture,
        uint32_t             arrayLayer,
        uint32_t             mipLevel,
        const void          *data);

    void UploadTexture2D(
        const RC<Texture2D> &texture,
        uint32_t             arrayLayer,
        uint32_t             mipLevel,
        const ImageDynamic  &image);

    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        const void               *initData,
        size_t                    initDataOffset = 0,
        size_t                    initDataSize = 0);

    RC<Texture2D> CreateTexture2D(
        uint32_t              width,
        uint32_t              height,
        uint32_t              arraySize,
        uint32_t              mipLevelCount,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        Span<const void *>    imageData);

    RC<Texture2D> CreateTexture2D(
        uint32_t              width,
        uint32_t              height,
        uint32_t              arraySize,
        uint32_t              mipLevelCount,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        Span<ImageDynamic>    images);

    RC<Texture2D> LoadTexture2D(
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
