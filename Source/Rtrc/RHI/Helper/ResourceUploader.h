#pragma once

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Utils/Image.h>

RTRC_RHI_BEGIN

class ResourceUploader : public Uncopyable
{
public:

    explicit ResourceUploader(RC<Device> device);

    ~ResourceUploader();

    BufferAcquireBarrier Upload(
        const RC<Buffer> &buffer,
        size_t            offset,
        size_t            range,
        const void       *data,
        const RC<Queue>  &afterQueue,
        ResourceStateFlag afterState);

    // use width * texelBytes if rowBytes is 0
    TextureAcquireBarrier Upload(
        const RC<Texture> &texture,
        AspectTypeFlag     aspect,
        uint32_t           mipLevel,
        uint32_t           arrayLayer,
        const void        *data,
        const RC<Queue>   &afterQueue,
        ResourceStateFlag  afterState);

    TextureAcquireBarrier Upload(
        const RC<Texture>  &texture,
        AspectTypeFlag      aspect,
        uint32_t            mipLevel,
        uint32_t            arrayLayer,
        const ImageDynamic &image,
        const RC<Queue>    &afterQueue,
        ResourceStateFlag   afterState);

    void SubmitAndSync();

private:

    static constexpr size_t MAX_ALLOWED_STAGING_BUFFER_COUNT = 64;
    static constexpr size_t MAX_ALLOWED_STAGING_BUFFER_SIZE = 128 * 1024 * 1024;

    RC<Device> device_;
    RC<Queue>  queue_;

    RC<CommandPool>   commandPool_;
    RC<CommandBuffer> commandBuffer_;

    size_t                                pendingStagingBufferSize_;
    std::vector<RC<Buffer>>               pendingStagingBuffers_;
    std::vector<BufferReleaseBarrier>     pendingBufferReleaseBarriers_;
    std::vector<TextureReleaseBarrier>    pendingTextureReleaseBarriers_;
    std::vector<TextureTransitionBarrier> pendingTextureTransitionBarriers_;
};

RTRC_RHI_END