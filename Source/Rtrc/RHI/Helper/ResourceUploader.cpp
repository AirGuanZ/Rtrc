#include <Rtrc/RHI/Helper/ResourceUploader.h>

RTRC_RHI_BEGIN

ResourceUploader::ResourceUploader(Ptr<Device> device)
    : device_(std::move(device)), pendingStagingBufferSize_(0)
{
    queue_ = device_->GetQueue(QueueType::Transfer);
    commandPool_ = queue_->CreateCommandPool();
    commandBuffer_ = commandPool_->NewCommandBuffer();
}

ResourceUploader::~ResourceUploader()
{
    assert(pendingStagingBuffers_.empty());
}

BufferAcquireBarrier ResourceUploader::Upload(
    Buffer            *buffer,
    size_t             offset,
    size_t             range,
    const void        *data,
    Queue             *afterQueue,
    PipelineStageFlag  afterStages,
    ResourceAccessFlag afterAccesses)
{
    if(buffer->GetDesc().hostAccessType == BufferHostAccessType::SequentialWrite ||
       buffer->GetDesc().hostAccessType == BufferHostAccessType::Random)
    {
        auto p = buffer->Map(offset, range);
        std::memcpy(p, data, range);
        buffer->Unmap(offset, range);
        return {};
    }

    auto stagingBuffer = device_->CreateBuffer(BufferDesc
        {
            .size                 = range,
            .usage                = BufferUsage::TransferSrc,
            .hostAccessType       = BufferHostAccessType::SequentialWrite,
            .concurrentAccessMode = QueueConcurrentAccessMode::Exclusive
        });

    auto p = stagingBuffer->Map(0, range);
    std::memcpy(p, data, range);
    stagingBuffer->Unmap(0, range);

    if(pendingStagingBuffers_.empty())
    {
        commandBuffer_->Begin();
    }

    pendingStagingBufferSize_ += range;
    pendingStagingBuffers_.push_back(stagingBuffer);
    pendingBufferReleaseBarriers_.reserve(pendingBufferReleaseBarriers_.size() + 1);

    commandBuffer_->CopyBuffer(buffer, offset, stagingBuffer.Get(), 0, range);

    if(afterQueue != queue_.Get())
    {
        // TODO: remove this barrier unless release/acquire is actually needed
        // TODO: since sync between different queues with same family index is ensured by waitidle
        pendingBufferReleaseBarriers_.push_back(BufferReleaseBarrier
            {
                .buffer         = buffer,
                .beforeStages   =  PipelineStage::Copy,
                .beforeAccesses = ResourceAccess::CopyWrite,
                .beforeQueue    = queue_.Get(),
                .afterQueue     = afterQueue
            });
    }

    if(pendingStagingBufferSize_ >= MAX_ALLOWED_STAGING_BUFFER_SIZE ||
       pendingStagingBuffers_.size() >= MAX_ALLOWED_STAGING_BUFFER_COUNT)
    {
        SubmitAndSync();
    }

    if(afterQueue != queue_.Get())
    {
        // TODO: remove this barrier unless release/acquire is actually needed
        return BufferAcquireBarrier
        {
            .buffer        = buffer,
            .afterStages   = afterStages,
            .afterAccesses = afterAccesses,
            .beforeQueue   = queue_.Get(),
            .afterQueue    = afterQueue
        };
    }
    return {};
}

TextureAcquireBarrier ResourceUploader::Upload(
    Texture           *texture,
    AspectTypeFlag     aspect,
    uint32_t           mipLevel,
    uint32_t           arrayLayer,
    const void        *data,
    Queue             *afterQueue,
    PipelineStageFlag  afterStages,
    ResourceAccessFlag afterAccesses,
    TextureLayout      afterLayout)
{
    assert(texture->GetDimension() == TextureDimension::Tex2D);
    auto &texDesc = texture->Get2DDesc();

    const size_t rowBytes = GetTexelSize(texDesc.format) * texDesc.width;
    const size_t stagingBufferSize = rowBytes * texDesc.height;
    assert(rowBytes);

    auto stagingBuffer = device_->CreateBuffer(BufferDesc
        {
            .size                 = stagingBufferSize,
            .usage                = BufferUsage::TransferSrc,
            .hostAccessType       = BufferHostAccessType::SequentialWrite,
            .concurrentAccessMode = QueueConcurrentAccessMode::Exclusive
        });

    auto p = stagingBuffer->Map(0, stagingBufferSize);
    std::memcpy(p, data, stagingBufferSize);
    stagingBuffer->Unmap(0, stagingBufferSize);

    if(pendingStagingBuffers_.empty())
    {
        commandBuffer_->Begin();
    }

    pendingStagingBufferSize_ += stagingBufferSize;
    pendingStagingBuffers_.push_back(stagingBuffer);
    pendingBufferReleaseBarriers_.reserve(pendingBufferReleaseBarriers_.size() + 1);

    commandBuffer_->ExecuteBarriers(TextureTransitionBarrier{
        .texture      = texture,
        .subresources = {
            .aspects    = AspectType::Color,
            .mipLevel   = mipLevel,
            .arrayLayer = arrayLayer
        },
        .beforeStages   = PipelineStage::None,
        .beforeAccesses = ResourceAccess::None,
        .beforeLayout   = TextureLayout::Undefined,
        .afterStages    = PipelineStage::Copy,
        .afterAccesses  = ResourceAccess::CopyWrite,
        .afterLayout    = TextureLayout::CopyDst
    }, {}, {}, {}, {}, {});

    commandBuffer_->CopyBufferToTexture(texture, aspect, mipLevel, arrayLayer, stagingBuffer.Get(), 0);
    
    if(afterQueue != queue_.Get())
    {
        pendingTextureReleaseBarriers_.push_back(TextureReleaseBarrier
            {
                .texture      = texture,
                .subresources = {
                    .aspects    = aspect,
                    .mipLevel   = mipLevel,
                    .arrayLayer = arrayLayer
                },
                .beforeStages   = PipelineStage::Copy,
                .beforeAccesses = ResourceAccess::CopyWrite,
                .beforeLayout   = TextureLayout::CopyDst,
                .afterLayout    = afterLayout,
                .beforeQueue    = queue_.Get(),
                .afterQueue     = afterQueue
            });
    }
    else
    {
        // add a regular barrier when before/after usages are on the same queue
        pendingTextureTransitionBarriers_.push_back(TextureTransitionBarrier
        {
            .texture      = texture,
            .subresources = {
                .aspects    = aspect,
                .mipLevel   = mipLevel,
                .arrayLayer = arrayLayer
            },
            .beforeStages   = PipelineStage::Copy,
            .beforeAccesses = ResourceAccess::CopyWrite,
            .beforeLayout   = TextureLayout::CopyDst,
            .afterStages    = afterStages,
            .afterAccesses  = afterAccesses,
            .afterLayout    = afterLayout
        });
    }

    if(pendingStagingBufferSize_ >= MAX_ALLOWED_STAGING_BUFFER_SIZE ||
       pendingStagingBuffers_.size() >= MAX_ALLOWED_STAGING_BUFFER_COUNT)
    {
        SubmitAndSync();
    }

    if(afterQueue != queue_.Get())
    {
        return TextureAcquireBarrier
            {
                .texture      = texture,
                .subresources = {
                    .aspects    = aspect,
                    .mipLevel   = mipLevel,
                    .arrayLayer = arrayLayer
                },
                .beforeLayout   = TextureLayout::CopyDst,
                .afterStages    = afterStages,
                .afterAccesses  = afterAccesses,
                .afterLayout    = afterLayout,
                .beforeQueue    = queue_.Get(),
                .afterQueue     = afterQueue
            };
    }
    return {};
}

TextureAcquireBarrier ResourceUploader::Upload(
    Texture            *texture,
    AspectTypeFlag      aspect,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    const ImageDynamic &image,
    Queue              *afterQueue,
    PipelineStageFlag   afterStages,
    ResourceAccessFlag  afterAccesses,
    TextureLayout       afterLayout)
{
    assert(texture->GetDimension() == TextureDimension::Tex2D);
    auto &texDesc = texture->Get2DDesc();
    switch(texDesc.format)
    {
    case Format::B8G8R8A8_UNorm:
        {
            auto tdata = image.To(ImageDynamic::U8x4);
            auto &data = tdata.As<Image<Vector4b>>();
            for(auto &v : data)
            {
                v = Vector4b(v.z, v.y, v.x, v.w);
            }
            return Upload(
                texture, aspect, mipLevel, arrayLayer, data.GetData(),
                afterQueue, afterStages, afterAccesses, afterLayout);
        }
    case Format::R32G32B32A32_Float:
        {
            auto tdata = image.To(ImageDynamic::F32x4);
            auto &data = tdata.As<Image<Vector4f>>();
            return Upload(
                texture, aspect, mipLevel, arrayLayer, data.GetData(),
                afterQueue, afterStages, afterAccesses, afterLayout);
        }
    default:
        throw Exception(std::string("unsupported upload texture format: ") + GetFormatName(texDesc.format));
    }
}

void ResourceUploader::SubmitAndSync()
{
    if(pendingStagingBuffers_.empty())
    {
        return;
    }

    commandBuffer_->ExecuteBarriers(
        pendingTextureTransitionBarriers_, {},
        pendingTextureReleaseBarriers_, {},
        pendingBufferReleaseBarriers_, {});
    commandBuffer_->End();

    queue_->Submit({}, {}, commandBuffer_, {}, {}, {});
    queue_->WaitIdle();
    commandPool_->Reset();

    pendingStagingBufferSize_ = 0;
    pendingStagingBuffers_.clear();
    pendingBufferReleaseBarriers_.clear();
    pendingTextureReleaseBarriers_.clear();
    pendingTextureTransitionBarriers_.clear();
}

RTRC_RHI_END
