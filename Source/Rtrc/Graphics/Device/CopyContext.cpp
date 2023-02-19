#include <Rtrc/Graphics/Device/CopyContext.h>
#include <Rtrc/Utility/Image.h>

RTRC_BEGIN

CopyContext::CopyContext(RHI::DevicePtr  device)
    : device_(std::move(device)), copyQueue_(device_->GetQueue(RHI::QueueType::Transfer))
{
    
}

void CopyContext::UploadBuffer(
    const RC<Buffer> &buffer,
    const void       *initData,
    size_t            offset,
    size_t            size)
{
    auto &bufferDesc = buffer->GetRHIObject()->GetDesc();

    if(!size)
    {
        assert(bufferDesc.size > offset);
        size = bufferDesc.size - offset;
    }

    if(bufferDesc.hostAccessType != RHI::BufferHostAccessType::None)
    {
        buffer->Upload(initData, offset, size);
        return;
    }

    assert(bufferDesc.usage.contains(RHI::BufferUsage::TransferDst));
    auto stagingBuffer = device_->CreateBuffer(RHI::BufferDesc
    {
        .size = size,
        .usage = RHI::BufferUsage::TransferSrc,
        .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
    });
    auto mappedStagingBuffer = stagingBuffer->Map(0, size);
    std::memcpy(mappedStagingBuffer, initData, size);
    stagingBuffer->Unmap(0, size, true);

    auto batch = GetBatch();
    batch.fence->Reset();
    RTRC_SCOPE_EXIT{ batches_.push(batch); };

    auto commandBuffer = batch.commandPool->NewCommandBuffer();
    commandBuffer->Begin();
    commandBuffer->CopyBuffer(buffer->GetRHIObject(), offset, stagingBuffer, 0, size);
    commandBuffer->End();
    copyQueue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();
}

void CopyContext::UploadTexture2D(
    const RC<Texture> &texture,
    uint32_t             arrayLayer,
    uint32_t             mipLevel,
    const void          *data,
    RHI::TextureLayout   postLayout)
{
    auto &texDesc = texture->GetDesc();

    if(texDesc.concurrentAccessMode != RHI::QueueConcurrentAccessMode::Concurrent)
    {
        throw Exception("CopyContext::UploadTexture2D: texture must be concurrent accessible");
    }

    const uint32_t width = texDesc.width >> mipLevel;
    const uint32_t height = texDesc.height >> mipLevel;

    const size_t rowSize = GetTexelSize(texDesc.format) * width;
    const size_t stagingBufferSize = rowSize * height;

    auto stagingBuffer = device_->CreateBuffer(RHI::BufferDesc
    {
        .size = stagingBufferSize,
        .usage = RHI::BufferUsage::TransferSrc,
        .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
    });
    auto mappedStagingBuffer = stagingBuffer->Map(0, stagingBufferSize);
    std::memcpy(mappedStagingBuffer, data, stagingBufferSize);
    stagingBuffer->Unmap(0, stagingBufferSize);

    auto batch = GetBatch();
    batch.fence->Reset();
    RTRC_SCOPE_EXIT{ batches_.push(batch); };

    auto commandBuffer = batch.commandPool->NewCommandBuffer();
    commandBuffer->Begin();
    commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
    {
        .texture        = texture->GetRHIObject(),
        .subresources   = { mipLevel, 1, arrayLayer, 1 },
        .beforeStages   = RHI::PipelineStage::None,
        .beforeAccesses = RHI::ResourceAccess::None,
        .beforeLayout   = RHI::TextureLayout::Undefined,
        .afterStages    = RHI::PipelineStage::Copy,
        .afterAccesses  = RHI::ResourceAccess::CopyWrite,
        .afterLayout    = RHI::TextureLayout::CopyDst
    });
    commandBuffer->CopyBufferToColorTexture2D(texture->GetRHIObject(), mipLevel, arrayLayer, stagingBuffer, 0);
    if(postLayout != RHI::TextureLayout::CopyDst)
    {
        commandBuffer->ExecuteBarriers(RHI::TextureTransitionBarrier
        {
            .texture        = texture->GetRHIObject(),
            .subresources   = { mipLevel, 1, arrayLayer, 1 },
            .beforeStages   = RHI::PipelineStage::Copy,
            .beforeAccesses = RHI::ResourceAccess::CopyWrite,
            .beforeLayout   = RHI::TextureLayout::CopyDst,
            .afterStages    = RHI::PipelineStage::All,
            .afterAccesses  = RHI::ResourceAccess::All,
            .afterLayout    = postLayout
        });
    }
    commandBuffer->End();
    copyQueue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();
}

void CopyContext::UploadTexture2D(
    const RC<Texture> &texture,
    uint32_t            arrayLayer,
    uint32_t            mipLevel,
    const ImageDynamic &image,
    RHI::TextureLayout  postLayout)
{
    auto &desc = texture->GetDesc();
    if(desc.format == RHI::Format::B8G8R8A8_UNorm)
    {
        auto tdata = image.To(ImageDynamic::U8x4);
        auto &data = tdata.As<Image<Vector4b>>();
        for(auto &v : data)
        {
            v = Vector4b(v.z, v.y, v.x, v.w);
        }
        UploadTexture2D(texture, arrayLayer, mipLevel, data.GetData(), postLayout);
        return;
    }
    if(desc.format == RHI::Format::R8G8B8A8_UNorm)
    {
        auto tdata = image.To(ImageDynamic::U8x4);
        auto &data = tdata.As<Image<Vector4b>>();
        UploadTexture2D(texture, arrayLayer, mipLevel, data.GetData(), postLayout);
        return;
    }
    if(desc.format == RHI::Format::R32G32B32A32_Float)
    {
        auto tdata = image.To(ImageDynamic::F32x4);
        auto &data = tdata.As<Image<Vector4f>>();
        UploadTexture2D(texture, arrayLayer, mipLevel, data.GetData(), postLayout);
        return;
    }
    throw Exception(std::string("CopyContext: Unsupported texture format: ") + GetFormatName(desc.format));
}

CopyContext::Batch CopyContext::GetBatch()
{
    Batch result;
    if(batches_.try_pop(result))
    {
        return result;
    }
    result.commandPool = device_->CreateCommandPool(copyQueue_);
    result.fence = device_->CreateFence(true);
    return result;
}

RTRC_END
