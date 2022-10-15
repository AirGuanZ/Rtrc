#pragma once

#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>
#include <Rtrc/Graphics/Object/Texture.h>

RTRC_BEGIN

class CopyContext : public Uncopyable
{
public:

    CopyContext(RHI::DevicePtr device, BufferManager *bufferManager);
    ~CopyContext();

    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        const void               *initData,
        size_t                    initDataSize = 0);

    RC<Texture2D> CreateTexture2D(
        uint32_t              width,
        uint32_t              height,
        uint32_t              arraySize,
        uint32_t              mipLevelCount,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        const void           *data);
    
    RC<Texture2D> CreateTexture2D(
        uint32_t              width,
        uint32_t              height,
        uint32_t              arraySize,
        uint32_t              mipLevelCount,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        const ImageDynamic   &data);

private:

    RHI::QueuePtr             copyQueue_;
    BufferManager            *bufferManager_;
    Box<HostSynchronizer>     stagingHostSync_;
    Box<BufferManager>        stagingBufferManager_;
    Box<CommandBufferManager> stagingCommandBufferManager_;
};

RTRC_END
