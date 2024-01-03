#pragma once

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Core/Resource/Image.h>

RTRC_BEGIN

class CopyContext : public Uncopyable
{
public:

    explicit CopyContext(RHI::DeviceOPtr device);

    void UploadBuffer(
        const RC<Buffer> &buffer,
        const void       *initData,
        size_t            offset = 0,
        size_t            size   = 0);

    void UploadTexture2D(
        const RC<Texture> &texture,
        uint32_t           arrayLayer,
        uint32_t           mipLevel,
        const void        *data,
        RHI::TextureLayout postLayout = RHI::TextureLayout::CopyDst);

    void UploadTexture2D(
        const RC<Texture>  &texture,
        uint32_t            arrayLayer,
        uint32_t            mipLevel,
        const ImageDynamic &image,
        RHI::TextureLayout  postLayout = RHI::TextureLayout::CopyDst);

private:

    struct Batch
    {
        RHI::CommandPoolUPtr commandPool;
        RHI::FenceUPtr fence;
    };

    Batch GetBatch();

    RHI::DeviceOPtr              device_;
    RHI::QueueRPtr               copyQueue_;
    tbb::concurrent_queue<Batch> batches_;
};

RTRC_END
