#pragma once

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Utils/Image.h>

RTRC_BEGIN

class ResourceUploader : public Uncopyable
{
public:

    explicit ResourceUploader(RHI::Ptr<RHI::Device> device);

    ~ResourceUploader();

    RHI::BufferAcquireBarrier Upload(
        RHI::Buffer            *buffer,
        size_t                  offset,
        size_t                  range,
        const void             *data,
        RHI::Queue             *afterQueue,
        RHI::PipelineStageFlag  afterStages,
        RHI::ResourceAccessFlag afterAccesses);

    // use width * texelBytes if rowBytes is 0
    RHI::TextureAcquireBarrier Upload(
        RHI::Texture           *texture,
        uint32_t                mipLevel,
        uint32_t                arrayLayer,
        const void             *data,
        RHI::Queue             *afterQueue,
        RHI::PipelineStageFlag  afterStages,
        RHI::ResourceAccessFlag afterAccesses,
        RHI::TextureLayout      afterLayout);

    RHI::TextureAcquireBarrier Upload(
        RHI::Texture            *texture,
        uint32_t                 mipLevel,
        uint32_t                 arrayLayer,
        const ImageDynamic      &image,
        RHI::Queue              *afterQueue,
        RHI::PipelineStageFlag   afterStages,
        RHI::ResourceAccessFlag  afterAccesses,
        RHI::TextureLayout       afterLayout);

    void SubmitAndSync();

private:

    static constexpr size_t MAX_ALLOWED_STAGING_BUFFER_COUNT = 64;
    static constexpr size_t MAX_ALLOWED_STAGING_BUFFER_SIZE = 128 * 1024 * 1024;

    RHI::Ptr<RHI::Device> device_;
    RHI::Ptr<RHI::Queue>  queue_;

    RHI::Ptr<RHI::CommandPool>   commandPool_;
    RHI::Ptr<RHI::CommandBuffer> commandBuffer_;

    size_t                                     pendingStagingBufferSize_;
    std::vector<RHI::Ptr<RHI::Buffer>>         pendingStagingBuffers_;
    std::vector<RHI::BufferReleaseBarrier>     pendingBufferReleaseBarriers_;
    std::vector<RHI::TextureReleaseBarrier>    pendingTextureReleaseBarriers_;
    std::vector<RHI::TextureTransitionBarrier> pendingTextureTransitionBarriers_;
};

RTRC_END
