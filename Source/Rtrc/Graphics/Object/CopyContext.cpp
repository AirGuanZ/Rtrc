#include <Rtrc/Graphics/Object/CopyContext.h>

RTRC_BEGIN

CopyContext::CopyContext(RHI::DevicePtr device, BufferManager *bufferManager)
    : bufferManager_(bufferManager)
{
    copyQueue_ = device->GetQueue(RHI::QueueType::Transfer);
    stagingHostSync_ = MakeBox<HostSynchronizer>(device, copyQueue_);
    stagingBufferManager_ = MakeBox<BufferManager>(*stagingHostSync_, device);
    stagingCommandBufferManager_ = MakeBox<CommandBufferManager>(*stagingHostSync_, device);
}

CopyContext::~CopyContext()
{
    stagingHostSync_->End();
    stagingCommandBufferManager_.reset();
    stagingBufferManager_.reset();
    stagingHostSync_.reset();
}

RC<Buffer> CopyContext::CreateBuffer(
    size_t                    size,
    RHI::BufferUsageFlag      usages,
    RHI::BufferHostAccessType hostAccess,
    const void               *initData,
    size_t                    initDataSize)
{
    if(initDataSize == 0)
    {
        initDataSize = size;
    }

    if(hostAccess != RHI::BufferHostAccessType::None)
    {
        auto buffer = bufferManager_->CreateBuffer(size, usages, hostAccess, false);
        buffer->Upload(initData, 0, initDataSize);
        return buffer;
    }
    
    auto buffer = bufferManager_->CreateBuffer(size, usages | RHI::BufferUsage::TransferDst, hostAccess, false);
    auto stagingBuffer = stagingBufferManager_->CreateBuffer(
        size, RHI::BufferUsage::TransferSrc, RHI::BufferHostAccessType::SequentialWrite, false);
    stagingBuffer->Upload(initData, 0, initDataSize);

    {
        auto commandBuffer = stagingCommandBufferManager_->Create();
        commandBuffer.Begin();
        commandBuffer.CopyBuffer(*buffer, 0, *stagingBuffer, 0, initDataSize);
        commandBuffer.End();
        copyQueue_->Submit({}, {}, commandBuffer.GetRHIObject(), {}, {}, {});
    }

    stagingHostSync_->WaitIdle();

    return buffer;
}

RC<Texture2D> CopyContext::CreateTexture2D(
    uint32_t              width,
    uint32_t              height,
    uint32_t              arraySize,
    uint32_t              mipLevelCount,
    RHI::Format           format,
    RHI::TextureUsageFlag usages,
    const void           *data)
{
    return {};
}
    
RC<Texture2D> CopyContext::CreateTexture2D(
    uint32_t              width,
    uint32_t              height,
    uint32_t              arraySize,
    uint32_t              mipLevelCount,
    RHI::Format           format,
    RHI::TextureUsageFlag usages,
    const ImageDynamic   &data)
{
    return {};
}

RTRC_END
