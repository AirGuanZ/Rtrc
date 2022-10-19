#include <Rtrc/Graphics/Object/CopyContext.h>
#include <Rtrc/Utils/Image.h>
#include <Rtrc/Utils/GenerateMipmap.h>

RTRC_BEGIN

CopyContext::CopyContext(
    RHI::DevicePtr  device,
    BufferManager  *bufferManager,
    TextureManager *textureManager)
    : device_(device),
      copyQueue_(device->GetQueue(RHI::QueueType::Transfer)),
      bufferManager_(bufferManager),
      textureManager_(textureManager)
{
    
}

void CopyContext::UploadBuffer(
    const RC<Buffer> &buffer,
    const void       *initData,
    size_t            offset,
    size_t            size)
{
    auto &bufferDesc = buffer->GetRHIObjectDesc();

    if(!size)
    {
        assert(bufferDesc.size > offset);
        size = bufferDesc.size - offset;
    }

    if(!buffer->GetUnsyncAccess().IsEmpty())
    {
        throw Exception("CopyContext::UploadBuffer: buffer must be externally synchronized");
    }

    if(bufferDesc.hostAccessType != RHI::BufferHostAccessType::None)
    {
        buffer->Upload(initData, offset, size);
        return;
    }

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
    const RC<Texture2D> &texture,
    uint32_t             arrayLayer,
    uint32_t             mipLevel,
    const void          *data)
{
    auto &texDesc = texture->GetRHIObjectDesc();

    if(texDesc.concurrentAccessMode != RHI::QueueConcurrentAccessMode::Concurrent)
    {
        throw Exception("CopyContext::UploadTexture2D: texture must be concurrent accessible");
    }

    auto &sync = texture->GetUnsyncAccess(arrayLayer, mipLevel);
    if(!sync.IsEmpty())
    {
        throw Exception("CopyContext::UploadTexture2D: texture must be externally synchronized");
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
        .afterStages    = RHI::PipelineStage::None,
        .afterAccesses  = RHI::ResourceAccess::None,
        .afterLayout    = RHI::TextureLayout::CopyDst
    });
    commandBuffer->CopyBufferToColorTexture2D(texture->GetRHIObject(), mipLevel, arrayLayer, stagingBuffer, 0);
    commandBuffer->End();
    copyQueue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();
}

void CopyContext::UploadTexture2D(
    const RC<Texture2D> &texture,
    uint32_t            arrayLayer,
    uint32_t            mipLevel,
    const ImageDynamic &image)
{
    auto &desc = texture->GetRHIObjectDesc();
    if(desc.format == RHI::Format::B8G8R8A8_UNorm)
    {
        auto tdata = image.To(ImageDynamic::U8x4);
        auto &data = tdata.As<Image<Vector4b>>();
        for(auto &v : data)
        {
            v = Vector4b(v.z, v.y, v.x, v.w);
        }
        UploadTexture2D(texture, arrayLayer, mipLevel, data.GetData());
        return;
    }
    if(desc.format == RHI::Format::R32G32B32A32_Float)
    {
        auto tdata = image.To(ImageDynamic::F32x4);
        auto &data = tdata.As<Image<Vector4f>>();
        UploadTexture2D(texture, arrayLayer, mipLevel, data.GetData());
        return;
    }
    throw Exception(std::string("CopyContext: Unsupported texture format: ") + GetFormatName(desc.format));
}

RC<Buffer> CopyContext::CreateBuffer(
    size_t                    size,
    RHI::BufferUsageFlag      usages,
    RHI::BufferHostAccessType hostAccess,
    const void               *initData,
    size_t                    initDataOffset,
    size_t                    initDataSize)
{
    if(hostAccess == RHI::BufferHostAccessType::None)
    {
        usages |= RHI::BufferUsage::TransferDst;
    }
    auto ret = bufferManager_->CreateBuffer(size, usages, hostAccess, false);
    UploadBuffer(ret, initData, initDataOffset, initDataSize);
    return ret;
}

RC<Texture2D> CopyContext::CreateTexture2D(
    uint32_t              width,
    uint32_t              height,
    uint32_t              arraySize,
    uint32_t              mipLevelCount,
    RHI::Format           format,
    RHI::TextureUsageFlag usages,
    Span<const void *>    imageData)
{
    assert(imageData.size() == mipLevelCount * arraySize);
    auto ret = textureManager_->CreateTexture2D(
        width, height, arraySize, mipLevelCount, format, usages | RHI::TextureUsage::TransferDst,
        RHI::QueueConcurrentAccessMode::Concurrent, 1, false);
    for(uint32_t a = 0, i = 0; a < arraySize; ++a)
    {
        for(uint32_t m = 0; m < mipLevelCount; ++m)
        {
            UploadTexture2D(ret, a, m, imageData[i++]);
        }
    }
    return ret;
}

RC<Texture2D> CopyContext::CreateTexture2D(
    uint32_t              width,
    uint32_t              height,
    uint32_t              arraySize,
    uint32_t              mipLevelCount,
    RHI::Format           format,
    RHI::TextureUsageFlag usages,
    Span<ImageDynamic>    images)
{
    assert(images.size() == mipLevelCount * arraySize);
    auto ret = textureManager_->CreateTexture2D(
        width, height, arraySize, mipLevelCount, format, usages | RHI::TextureUsage::TransferDst,
        RHI::QueueConcurrentAccessMode::Concurrent, 1, false);
    for(uint32_t a = 0, i = 0; a < arraySize; ++a)
    {
        for(uint32_t m = 0; m < mipLevelCount; ++m)
        {
            UploadTexture2D(ret, a, m, images[i++]);
        }
    }
    return ret;
}

RC<Texture2D> CopyContext::LoadTexture2D(
    const std::string &filename, RHI::Format format, RHI::TextureUsageFlag usages, bool generateMipLevels)
{
    std::vector<ImageDynamic> images;
    images.push_back(ImageDynamic::Load(filename));
    uint32_t mipLevels = 1;
    if(generateMipLevels)
    {
        mipLevels = ComputeFullMipmapChainSize(images[0].GetWidth(), images[0].GetHeight());
        images.reserve(mipLevels);
        for(uint32_t i = 1; i < mipLevels; ++i)
        {
            images.push_back(GenerateNextImageMipmapLevel(images.back()));
        }
    }
    return CreateTexture2D(images[0].GetWidth(), images[0].GetHeight(), 1, mipLevels, format, usages, images);
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
