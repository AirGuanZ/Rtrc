#include <Rtrc/Graphics/Resource/ResourceUploader.h>

RTRC_BEGIN

using namespace RHI;

ResourceUploader::ResourceUploader(Ptr<Device> device)
    : device_(std::move(device)), pendingStagingBufferSize_(0)
{
    queue_ = device_->GetQueue(QueueType::Transfer);
    commandPool_ = device_->CreateCommandPool(queue_);
    commandBuffer_ = commandPool_->NewCommandBuffer();
}

ResourceUploader::~ResourceUploader()
{
    assert(pendingStagingBuffers_.empty());
}

ResourceUploadHandle<ResourceUploader::DummyAcquire> ResourceUploader::Upload(
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
        buffer->Unmap(offset, range, true);
        return ResourceUploadHandle<DummyAcquire>({}, {});
    }

    auto stagingBuffer = device_->CreateBuffer(BufferDesc
        {
            .size           = range,
            .usage          = BufferUsage::TransferSrc,
            .hostAccessType = BufferHostAccessType::SequentialWrite
        });

    auto p = stagingBuffer->Map(0, range);
    std::memcpy(p, data, range);
    stagingBuffer->Unmap(0, range);
    stagingBuffer->FlushAfterWrite(0, range);

    if(pendingStagingBuffers_.empty())
    {
        commandBuffer_->Begin();
    }

    pendingStagingBufferSize_ += range;
    pendingStagingBuffers_.push_back(stagingBuffer);

    commandBuffer_->CopyBuffer(buffer, offset, stagingBuffer.Get(), 0, range);

    TimelineSemaphoreWaiter waiter;
    if(pendingStagingBufferSize_ >= MAX_ALLOWED_STAGING_BUFFER_SIZE ||
       pendingStagingBuffers_.size() >= MAX_ALLOWED_STAGING_BUFFER_COUNT)
    {
        SubmitAndSync();
    }
    else
    {
        waiter = timelineSemaphore_.CreateWaiter();
    }
    return ResourceUploadHandle(waiter, DummyAcquire{});
}

ResourceUploadHandle<TextureAcquireBarrier> ResourceUploader::Upload(
    Texture           *texture,
    uint32_t           mipLevel,
    uint32_t           arrayLayer,
    const void        *data,
    Queue             *afterQueue,
    PipelineStageFlag  afterStages,
    ResourceAccessFlag afterAccesses,
    TextureLayout      afterLayout)
{
    auto &texDesc = texture->GetDesc();

    const size_t rowBytes = GetTexelSize(texDesc.format) * texDesc.width;
    const size_t stagingBufferSize = rowBytes * texDesc.height;
    assert(rowBytes);

    auto stagingBuffer = device_->CreateBuffer(BufferDesc
        {
            .size           = stagingBufferSize,
            .usage          = BufferUsage::TransferSrc,
            .hostAccessType = BufferHostAccessType::SequentialWrite
        });

    auto p = stagingBuffer->Map(0, stagingBufferSize);
    std::memcpy(p, data, stagingBufferSize);
    stagingBuffer->Unmap(0, stagingBufferSize);
    stagingBuffer->FlushAfterWrite(0, stagingBufferSize);

    if(pendingStagingBuffers_.empty())
    {
        commandBuffer_->Begin();
    }

    pendingStagingBufferSize_ += stagingBufferSize;
    pendingStagingBuffers_.push_back(stagingBuffer);

    commandBuffer_->ExecuteBarriers(TextureTransitionBarrier{
        .texture      = texture,
        .subresources = {
            .mipLevel   = mipLevel,
            .arrayLayer = arrayLayer
        },
        .beforeStages   = PipelineStage::None,
        .beforeAccesses = ResourceAccess::None,
        .beforeLayout   = TextureLayout::Undefined,
        .afterStages    = PipelineStage::Copy,
        .afterAccesses  = ResourceAccess::CopyWrite,
        .afterLayout    = TextureLayout::CopyDst
    });

    commandBuffer_->CopyBufferToColorTexture2D(texture, mipLevel, arrayLayer, stagingBuffer.Get(), 0);
    
    if(afterQueue->GetType() != queue_->GetType())
    {
        pendingTextureReleaseBarriers_.push_back(TextureReleaseBarrier
            {
                .texture      = texture,
                .subresources = {
                    .mipLevel   = mipLevel,
                    .arrayLayer = arrayLayer
                },
                .beforeStages   = PipelineStage::Copy,
                .beforeAccesses = ResourceAccess::CopyWrite,
                .beforeLayout   = TextureLayout::CopyDst,
                .afterLayout    = afterLayout,
                .beforeQueue    = queue_->GetType(),
                .afterQueue     = afterQueue->GetType()
            });
    }
    else
    {
        // add a regular barrier when before/after usages are on the same queue
        pendingTextureTransitionBarriers_.push_back(TextureTransitionBarrier
        {
            .texture      = texture,
            .subresources = {
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

    TimelineSemaphoreWaiter waiter;
    if(pendingStagingBufferSize_ >= MAX_ALLOWED_STAGING_BUFFER_SIZE ||
       pendingStagingBuffers_.size() >= MAX_ALLOWED_STAGING_BUFFER_COUNT)
    {
        SubmitAndSync();
    }
    else
    {
        waiter = timelineSemaphore_.CreateWaiter();
    }

    if(afterQueue->GetType() != queue_->GetType())
    {
        return ResourceUploadHandle(waiter, TextureAcquireBarrier
            {
                .texture      = texture,
                .subresources = {
                    .mipLevel   = mipLevel,
                    .arrayLayer = arrayLayer
                },
                .beforeLayout   = TextureLayout::CopyDst,
                .afterStages    = afterStages,
                .afterAccesses  = afterAccesses,
                .afterLayout    = afterLayout,
                .beforeQueue    = queue_->GetType(),
                .afterQueue     = afterQueue->GetType()
            });
    }
    return ResourceUploadHandle(waiter, TextureAcquireBarrier{});
}

ResourceUploadHandle<TextureAcquireBarrier> ResourceUploader::Upload(
    Texture            *texture,
    uint32_t            mipLevel,
    uint32_t            arrayLayer,
    const ImageDynamic &image,
    Queue              *afterQueue,
    PipelineStageFlag   afterStages,
    ResourceAccessFlag  afterAccesses,
    TextureLayout       afterLayout)
{
    auto &texDesc = texture->GetDesc();
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
                texture, mipLevel, arrayLayer, data.GetData(),
                afterQueue, afterStages, afterAccesses, afterLayout);
        }
    case Format::R32G32B32A32_Float:
        {
            auto tdata = image.To(ImageDynamic::F32x4);
            auto &data = tdata.As<Image<Vector4f>>();
            return Upload(
                texture, mipLevel, arrayLayer, data.GetData(),
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

    commandBuffer_->ExecuteBarriers(pendingTextureTransitionBarriers_, pendingTextureReleaseBarriers_);
    commandBuffer_->End();

    queue_->Submit({}, {}, commandBuffer_, {}, {}, {});
    queue_->WaitIdle();
    timelineSemaphore_.Signal();
    commandPool_->Reset();

    pendingStagingBufferSize_ = 0;
    pendingStagingBuffers_.clear();
    pendingTextureReleaseBarriers_.clear();
    pendingTextureTransitionBarriers_.clear();
}

RTRC_END
