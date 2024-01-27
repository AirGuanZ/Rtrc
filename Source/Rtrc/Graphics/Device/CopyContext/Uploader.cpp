#include <Rtrc/Core/Resource/Image.h>
#include <Rtrc/Graphics/Device/CopyContext/Uploader.h>

RTRC_BEGIN

UploadBatch::UploadBatch(Ref<Uploader> uploader)
    : uploader_(uploader)
{
    
}

UploadBatch::~UploadBatch()
{
    if(!bufferTasks_.empty() || !textureTasks_.empty())
    {
        LogError("UploadBatch is destructed with pending uploading tasks");
    }
}

UploadBatch::UploadBatch(UploadBatch &&other) noexcept
    : uploader_(other.uploader_)
{
    Swap(other);
}

UploadBatch &UploadBatch::operator=(UploadBatch &&other) noexcept
{
    Swap(other);
    return *this;
}

void UploadBatch::Swap(UploadBatch &other) noexcept
{
    std::swap(bufferTasks_, other.bufferTasks_);
    std::swap(textureTasks_, other.textureTasks_);
    std::swap(uploader_, other.uploader_);
}

void UploadBatch::Record(const RC<Buffer> &buffer, const void *data, bool takeCopyOfData, size_t offset, size_t size)
{
    Record(StatefulBuffer::FromBuffer(buffer), data, takeCopyOfData, offset, size);
}

void UploadBatch::Record(
    const RC<StatefulBuffer> &buffer, const void *data, bool takeCopyOfData, size_t offset, size_t size)
{
    assert(buffer->GetDesc().usage.Contains(RHI::BufferUsage::TransferDst));
    auto &task = bufferTasks_.emplace_back();
    task.buffer = StatefulBuffer::FromBuffer(buffer);
    task.data = data;
    task.offset = offset;
    task.size = size;
    if(task.size == 0)
    {
        task.size = buffer->GetSize() - task.offset;
    }
    if(takeCopyOfData)
    {
        task.ownedData.resize(task.size);
        std::memcpy(task.ownedData.data(), data, task.size);
        task.data = task.ownedData.data();
    }
}

void UploadBatch::Record(
    const RC<Texture> &texture,
    TexSubrsc          subrsc,
    const void        *data,
    size_t             dataRowBytes,
    RHI::TextureLayout afterLayout,
    bool               takeCopyOfData)
{
    Record(StatefulTexture::FromTexture(texture), subrsc, data, dataRowBytes, afterLayout, takeCopyOfData);
}

void UploadBatch::Record(
    const RC<StatefulTexture> &texture,
    RHI::TexSubrsc    subrsc,
    const void                *data,
    size_t                     dataRowBytes,
    RHI::TextureLayout         afterLayout,
    bool                       takeCopyOfData)
{
    assert(texture->GetDesc().usage.Contains(RHI::TextureUsage::TransferDst));

    const uint32_t width = std::max<uint32_t>(texture->GetWidth() >> subrsc.mipLevel, 1);
    const uint32_t height = std::max<uint32_t>(texture->GetHeight() >> subrsc.mipLevel, 1);
    if(dataRowBytes == 0)
    {
        dataRowBytes = width * GetTexelSize(texture->GetFormat());
    }

    auto &task = textureTasks_.emplace_back();
    task.texture      = StatefulTexture::FromTexture(texture);
    task.subrsc       = subrsc;
    task.data         = data;
    task.dataRowBytes = dataRowBytes;
    task.afterLayout  = afterLayout;

    const size_t rowAlignment = uploader_->device_->GetTextureBufferCopyRowPitchAlignment(texture->GetFormat());
    size_t ownedDataRowBytes = dataRowBytes;
    if(ownedDataRowBytes % rowAlignment)
    {
        takeCopyOfData = true;
        ownedDataRowBytes = UpAlignTo(ownedDataRowBytes, rowAlignment);
    }

    if(takeCopyOfData)
    {
        task.ownedData.resize(ownedDataRowBytes * height);
        if(ownedDataRowBytes == dataRowBytes)
        {
            std::memcpy(task.ownedData.data(), data, task.ownedData.size());
        }
        else
        {
            for(unsigned y = 0; y < height; ++y)
            {
                auto src = reinterpret_cast<const char *>(data) + dataRowBytes * y;
                auto dst = task.ownedData.data() + ownedDataRowBytes * y;
                std::memcpy(dst, src, ownedDataRowBytes);
            }
        }
        task.dataRowBytes = ownedDataRowBytes;
        task.data = task.ownedData.data();
    }
}

void UploadBatch::Record(
    const RC<Texture>  &texture,
    TexSubrsc           subrsc,
    const ImageDynamic &image,
    RHI::TextureLayout  afterLayout)
{
    Record(StatefulTexture::FromTexture(texture), subrsc, image, afterLayout);
}

void UploadBatch::Record(
    const RC<StatefulTexture> &texture,
    RHI::TexSubrsc    subrsc,
    const ImageDynamic        &image,
    RHI::TextureLayout         afterLayout)
{
    assert(texture->GetDimension() == RHI::TextureDimension::Tex2D);
    assert(texture->GetWidth(subrsc.mipLevel) == image.GetWidth());
    assert(texture->GetHeight(subrsc.mipLevel) == image.GetHeight());
    switch(texture->GetFormat())
    {
    case RHI::Format::B8G8R8A8_UNorm:
    {
        auto data = image.To<Vector4b>();
        for(auto &v : data)
        {
            v = { v.z, v.y, v.x, v.w };
        }
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    case RHI::Format::R8G8B8A8_UNorm:
    {
        auto data = image.To<Vector4b>();
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    case RHI::Format::R32G32B32A32_Float:
    {
        auto data = image.To<Vector4f>();
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    case RHI::Format::R32G32B32_Float:
    {
        auto data = image.To<Vector3f>();
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    case RHI::Format::R32G32_Float:
    {
        auto data = image.To<Vector2f>();
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    case RHI::Format::R32_Float:
    {
        auto data = image.To<float>();
        Record(texture, subrsc, data.GetData(), 0, afterLayout, true);
        break;
    }
    default:
        throw Exception(
            std::string("UploadContext: Unsupported texture format: ") + GetFormatName(texture->GetFormat()));
    }
}

void UploadBatch::Cancel()
{
    bufferTasks_.clear();
    textureTasks_.clear();
}

void UploadBatch::SubmitAndWait()
{
    if(bufferTasks_.empty() && textureTasks_.empty())
    {
        return;
    }

    auto device = uploader_->device_;
    auto batch = uploader_->GetBatch();
    batch.fence->Reset();
    RTRC_SCOPE_EXIT{ uploader_->FreeBatch(std::move(batch)); };

    auto commandBuffer = batch.commandPool->NewCommandBuffer();
    commandBuffer->Begin();

    std::vector<RHI::BufferTransitionBarrier> beforeBufferBarriers;
    for(auto& task : bufferTasks_)
    {
        auto &prevState = task.buffer->GetState();
        if(prevState.stages == RHI::PipelineStage::None && prevState.accesses == RHI::ResourceAccess::None)
        {
            continue;
        }
        auto &b = beforeBufferBarriers.emplace_back();
        b.buffer = task.buffer->GetRHIObject().Get();
        b.beforeStages = prevState.stages;
        b.beforeAccesses = prevState.accesses;
        b.afterStages = RHI::PipelineStage::Copy;
        b.afterAccesses = RHI::ResourceAccess::CopyWrite;

        task.buffer->SetState(RHI::PipelineStage::None, RHI::ResourceAccess::None);
    }

    std::vector<RHI::TextureTransitionBarrier> beforeTextureBarriers, afterTextureBarriers;
    for(auto &task : textureTasks_)
    {
        auto &prevState = task.texture->GetState(task.subrsc.mipLevel, task.subrsc.arrayLayer);

        const bool skipBeforeBarrier = prevState.layout == RHI::TextureLayout::CopyDst &&
                                       prevState.stages == RHI::PipelineStage::None &&
                                       prevState.accesses == RHI::ResourceAccess::None;
        if(!skipBeforeBarrier)
        {
            auto &b = beforeTextureBarriers.emplace_back();
            b.texture = task.texture->GetRHIObject().Get();
            b.subresources = TexSubrscs
            {
                .mipLevel = task.subrsc.mipLevel,
                .levelCount = 1,
                .arrayLayer = task.subrsc.arrayLayer,
                .layerCount = 1
            };
            b.beforeStages = prevState.stages;
            b.beforeAccesses = prevState.accesses;
            b.beforeLayout = RHI::TextureLayout::Undefined;
            b.afterStages = RHI::PipelineStage::Copy;
            b.afterAccesses = RHI::ResourceAccess::CopyWrite;
            b.afterLayout = RHI::TextureLayout::CopyDst;
        }

        if(task.afterLayout != RHI::TextureLayout::CopyDst)
        {
            auto &a = afterTextureBarriers.emplace_back();
            a.texture = task.texture->GetRHIObject().Get();
            a.subresources = TexSubrscs
            {
                .mipLevel = task.subrsc.mipLevel,
                .levelCount = 1,
                .arrayLayer = task.subrsc.arrayLayer,
                .layerCount = 1
            };
            a.beforeStages = RHI::PipelineStage::Copy;
            a.beforeAccesses = RHI::ResourceAccess::CopyWrite;
            a.beforeLayout = RHI::TextureLayout::CopyDst;
            a.afterStages = RHI::PipelineStage::None;
            a.afterAccesses = RHI::ResourceAccess::None;
            a.afterLayout = task.afterLayout;
        }

        task.texture->SetState(
            task.subrsc.mipLevel, task.subrsc.arrayLayer, TexSubrscState(
                task.afterLayout, RHI::PipelineStage::None, RHI::ResourceAccess::None));
    }

    if(!beforeBufferBarriers.empty() || !beforeTextureBarriers.empty())
    {
        commandBuffer->ExecuteBarriers(beforeBufferBarriers, beforeTextureBarriers);
    }

    std::vector<RHI::BufferUPtr> stagingBuffers;
    auto GetStagingBuffer = [&](size_t size, const void *data)
    {
        auto stagingBuffer = device->CreateBuffer(RHI::BufferDesc
        {
            .size = size,
            .usage = RHI::BufferUsage::TransferSrc,
            .hostAccessType = RHI::BufferHostAccessType::Upload
        });
        stagingBuffer->SetName("StagingBuffer");

        auto ptr = stagingBuffer->Map(0, size, {}, false);
        std::memcpy(ptr, data, size);
        stagingBuffer->Unmap(0, size, true);

        stagingBuffers.push_back(std::move(stagingBuffer));
        return RHI::OPtr(stagingBuffers.back().Get());
    };

    for(auto &task : bufferTasks_)
    {
        auto stagingBuffer = GetStagingBuffer(task.size, task.data);
        commandBuffer->CopyBuffer(
            task.buffer->GetRHIObject().Get(), task.offset, stagingBuffer.Get(), 0, task.size);
    }

    for(auto& task : textureTasks_)
    {
        const size_t size = task.dataRowBytes * (task.texture->GetHeight() >> task.subrsc.mipLevel);
        auto stagingBuffer = GetStagingBuffer(size, task.data);
        commandBuffer->CopyBufferToColorTexture2D(
            task.texture->GetRHIObject().Get(),
            task.subrsc.mipLevel, task.subrsc.arrayLayer,
            stagingBuffer.Get(), 0, task.dataRowBytes);
    }

    if(!afterTextureBarriers.empty())
    {
        commandBuffer->ExecuteBarriers(afterTextureBarriers);
    }

    commandBuffer->End();

    uploader_->queue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();

    bufferTasks_.clear();
    textureTasks_.clear();
}

Uploader::Uploader(RHI::DeviceOPtr device, RHI::QueueRPtr queue)
    : device_(device), queue_(std::move(queue))
{
    
}

void Uploader::Upload(const RC<Buffer> &buffer, const void *data, size_t offset, size_t size)
{
    auto batch = CreateBatch();
    batch.Record(buffer, data, false, offset, size);
    batch.SubmitAndWait();
}

void Uploader::Upload(
    const RC<Texture> &texture, TexSubrsc subrsc,
    const void *data, size_t dataRowBytes, RHI::TextureLayout afterLayout)
{
    auto batch = CreateBatch();
    batch.Record(texture, subrsc, data, dataRowBytes, afterLayout, false);
    batch.SubmitAndWait();
}

void Uploader::Upload(
    const RC<StatefulTexture> &texture, TexSubrsc subrsc,
    const void *data, size_t dataRowBytes, RHI::TextureLayout afterLayout)
{
    auto batch = CreateBatch();
    batch.Record(texture, subrsc, data, dataRowBytes, afterLayout, false);
    batch.SubmitAndWait();
}

void Uploader::Upload(
    const RC<Texture> &texture, TexSubrsc subrsc,
    const ImageDynamic &image, RHI::TextureLayout afterLayout)
{
    auto batch = CreateBatch();
    batch.Record(texture, subrsc, image, afterLayout);
    batch.SubmitAndWait();
}

void Uploader::Upload(
    const RC<StatefulTexture> &texture, TexSubrsc subrsc,
    const ImageDynamic &image, RHI::TextureLayout afterLayout)
{
    auto batch = CreateBatch();
    batch.Record(texture, subrsc, image, afterLayout);
    batch.SubmitAndWait();
}

UploadBatch Uploader::CreateBatch()
{
    return UploadBatch(this);
}

Uploader::Batch Uploader::GetBatch()
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

void Uploader::FreeBatch(Batch batch)
{
    batches_.push(std::move(batch));
}

RTRC_END
