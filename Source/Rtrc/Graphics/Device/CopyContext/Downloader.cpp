#include <Rtrc/Graphics/Device/CopyContext/Downloader.h>

RTRC_BEGIN

DownloadBatch::~DownloadBatch()
{
    if(!bufferTasks_.empty() || !textureTasks_.empty())
    {
        LogError("UploadBatch is destructed with pending uploading tasks");
    }
}

DownloadBatch::DownloadBatch(DownloadBatch &&other) noexcept
    : downloader_(std::move(other.downloader_))
{
    Swap(other);
}

DownloadBatch &DownloadBatch::operator=(DownloadBatch &&other) noexcept
{
    Swap(other);
    return *this;
}

void DownloadBatch::Swap(DownloadBatch &other) noexcept
{
    std::swap(bufferTasks_, other.bufferTasks_);
    std::swap(textureTasks_, other.textureTasks_);
    std::swap(downloader_, other.downloader_);
}

void DownloadBatch::Record(const RC<StatefulBuffer> &buffer, void *data, size_t offset, size_t size)
{
    if(size == 0)
    {
        size = buffer->GetSize() - offset;
    }
    bufferTasks_.push_back({ buffer, data, offset, size });
}

void DownloadBatch::Record(const RC<StatefulTexture> &texture, TexSubrsc subrsc, void *outputData, size_t dataRowBytes)
{
    const size_t packedRowBytes = texture->GetMipLevelWidth(subrsc.mipLevel) * GetTexelSize(texture->GetFormat());
    if(dataRowBytes == 0)
    {
        dataRowBytes = packedRowBytes;
    }
    assert(dataRowBytes >= packedRowBytes);

    const size_t stagingRowAlignment = downloader_->device_->GetTextureBufferCopyRowPitchAlignment(texture->GetFormat());
    const size_t stagingRowBytes = UpAlignTo(packedRowBytes, stagingRowAlignment);

    textureTasks_.push_back({ texture, subrsc, outputData, dataRowBytes, stagingRowBytes, packedRowBytes });
}

void DownloadBatch::Cancel()
{
    bufferTasks_.clear();
    textureTasks_.clear();
}

void DownloadBatch::SubmitAndWait()
{
    if(bufferTasks_.empty() && textureTasks_.empty())
    {
        return;
    }

    auto device = downloader_->device_;
    auto batch = downloader_->GetBatch();
    batch.fence->Reset();
    RTRC_SCOPE_EXIT{ downloader_->FreeBatch(std::move(batch)); };

    auto commandBuffer = batch.commandPool->NewCommandBuffer();
    commandBuffer->Begin();

    std::vector<RHI::BufferTransitionBarrier> beforeBufferTransitions;
    for(auto& task : bufferTasks_)
    {
        auto &prevState = task.buffer->GetState();
        if(prevState.stages == RHI::PipelineStage::None && prevState.accesses == RHI::ResourceAccess::None)
        {
            continue;
        }
        auto &b = beforeBufferTransitions.emplace_back();
        b.buffer = task.buffer->GetRHIObject().Get();
        b.beforeStages = prevState.stages;
        b.beforeAccesses = prevState.accesses;
        b.afterStages = RHI::PipelineStage::Copy;
        b.afterAccesses = RHI::ResourceAccess::CopyRead;
        task.buffer->SetState({ RHI::INITIAL_QUEUE_SESSION_ID, RHI::PipelineStage::None, RHI::ResourceAccess::None });
    }

    std::vector<RHI::TextureTransitionBarrier> beforeTextureTransitions;
    for(auto &task : textureTasks_)
    {
        auto &prevState = task.texture->GetState(task.subrsc);
        if(prevState.layout == RHI::TextureLayout::CopySrc)
        {
            assert(RHI::IsReadOnly(prevState.accesses));
            task.texture->SetState(task.subrsc, TexSubrscState(
                RHI::INITIAL_QUEUE_SESSION_ID, RHI::TextureLayout::CopySrc,
                RHI::PipelineStage::None, RHI::ResourceAccess::None));
        }
        else
        {
            auto &b = beforeTextureTransitions.emplace_back();
            b.texture = task.texture->GetRHIObject().Get();
            b.subresources =
            {
                .mipLevel = task.subrsc.mipLevel,
                .levelCount = 1,
                .arrayLayer = task.subrsc.arrayLayer,
                .layerCount = 1
            };
            b.beforeLayout = prevState.layout;
            b.beforeStages = prevState.stages;
            b.beforeAccesses = prevState.accesses;
            b.afterLayout = RHI::TextureLayout::CopySrc;
            b.afterStages = RHI::PipelineStage::Copy;
            b.afterAccesses = RHI::ResourceAccess::CopyRead;

            task.texture->SetState(task.subrsc, TexSubrscState(
                RHI::INITIAL_QUEUE_SESSION_ID, RHI::TextureLayout::CopySrc,
                RHI::PipelineStage::None, RHI::ResourceAccess::None));
        }
    }

    if(!beforeBufferTransitions.empty() || !beforeTextureTransitions.empty())
    {
        commandBuffer->ExecuteBarriers(beforeBufferTransitions, beforeTextureTransitions);
    }
    
    std::vector<RHI::BufferUPtr> stagingBuffers;
    auto GetStagingBuffer = [&](size_t size)
    {
        auto stagingBuffer = device->CreateBuffer(RHI::BufferDesc
        {
            .size = size,
            .usage = RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::Readback
        });
        stagingBuffer->SetName("StagingBuffer");

        stagingBuffers.push_back(std::move(stagingBuffer));
        return RHI::OPtr(stagingBuffers.back().Get());
    };

    for(auto &task : bufferTasks_)
    {
        auto stagingBuffer = GetStagingBuffer(task.size);
        commandBuffer->CopyBuffer(stagingBuffer.Get(), 0, task.buffer->GetRHIObject().Get(), task.offset, task.size);
    }

    for(auto &task : textureTasks_)
    {
        auto stagingBuffer = GetStagingBuffer(task.stagingRowBytes * task.texture->GetMipLevelHeight(task.subrsc.mipLevel));
        commandBuffer->CopyColorTexture2DToBuffer(
            stagingBuffer.Get(), 0, task.stagingRowBytes,
            task.texture->GetRHIObject().Get(), task.subrsc.mipLevel, task.subrsc.arrayLayer);
    }

    commandBuffer->End();

    downloader_->queue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();

    size_t stagingBufferIndex = 0;

    for(auto &task : bufferTasks_)
    {
        auto &stagingBuffer = stagingBuffers[stagingBufferIndex++];
        auto p = stagingBuffer->Map(0, task.size, RHI::BufferReadRange{ 0, task.size }, true);
        std::memcpy(task.outputData, p, task.size);
        stagingBuffer->Unmap(0, task.size);
    }

    for(auto& task : textureTasks_)
    {
        auto &stagingBuffer = stagingBuffers[stagingBufferIndex++];
        const size_t stagingBufferSize = stagingBuffer->GetDesc().size;
        auto p = stagingBuffer->Map(0, stagingBufferSize, RHI::BufferReadRange{ 0, stagingBufferSize }, true);
        if(task.dataRowBytes == task.stagingRowBytes)
        {
            std::memcpy(task.outputData, p, stagingBufferSize);
        }
        else
        {
            const uint32_t height = task.texture->GetMipLevelHeight(task.subrsc.mipLevel);
            for(unsigned y = 0; y < height; ++y)
            {
                auto src = reinterpret_cast<const uint8_t*>(p) + y * task.stagingRowBytes;
                auto dst = reinterpret_cast<uint8_t*>(task.outputData) + y * task.dataRowBytes;
                std::memcpy(dst, src, task.packedRowBytes);
            }
        }
        stagingBuffer->Unmap(0, stagingBufferSize);
    }

    bufferTasks_.clear();
    textureTasks_.clear();
}

DownloadBatch::DownloadBatch(Ref<Downloader> downloader)
    : downloader_(std::move(downloader))
{
    
}

Downloader::Downloader(RHI::DeviceOPtr device, RHI::QueueRPtr queue)
    : device_(device), queue_(std::move(queue))
{
    
}

void Downloader::Download(const RC<StatefulBuffer> &buffer, void *outputData, size_t offset, size_t size)
{
    auto batch = CreateBatch();
    batch.Record(buffer, outputData, offset, size);
    batch.SubmitAndWait();
}

void Downloader::Download(const RC<StatefulTexture> &texture, TexSubrsc subrsc, void *outputData, size_t dataRowBytes)
{
    auto batch = CreateBatch();
    batch.Record(texture, subrsc, outputData, dataRowBytes);
    batch.SubmitAndWait();
}

DownloadBatch Downloader::CreateBatch()
{
    return DownloadBatch(this);
}

Downloader::Batch Downloader::GetBatch()
{
    Batch result;
    if(batches_.try_pop(result))
    {
        result.commandPool->Reset();
        return result;
    }
    result.commandPool = device_->CreateCommandPool(queue_);
    result.fence = device_->CreateFence(true);
    return result;
}

void Downloader::FreeBatch(Batch batch)
{
    batches_.push(std::move(batch));
}

RTRC_END
