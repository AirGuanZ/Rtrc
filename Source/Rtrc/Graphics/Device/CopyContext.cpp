#include <Rtrc/Graphics/Device/CopyContext.h>
#include <Rtrc/Utility/Image.h>
#include <Rtrc/Utility/GenerateMipmap.h>

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
    const void          *data)
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
    commandBuffer->End();
    copyQueue_->Submit({}, {}, commandBuffer, {}, {}, batch.fence);
    batch.fence->Wait();
}

void CopyContext::UploadTexture2D(
    const RC<Texture> &texture,
    uint32_t            arrayLayer,
    uint32_t            mipLevel,
    const ImageDynamic &image)
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
    const RHI::BufferDesc    &_desc,
    const void               *initData,
    size_t                    initDataOffset,
    size_t                    initDataSize)
{
    RHI::BufferDesc desc = _desc;
    if(desc.hostAccessType == RHI::BufferHostAccessType::None)
    {
        desc.usage |= RHI::BufferUsage::TransferDst;
    }
    auto ret = bufferManager_->Create(desc);
    UploadBuffer(ret, initData, initDataOffset, initDataSize);
    return ret;
}

RC<Texture> CopyContext::CreateTexture2D(
    const RHI::TextureDesc &_desc,
    Span<const void *>      imageData)
{
    RHI::TextureDesc desc = _desc;
    desc.concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent;
    desc.usage |= RHI::TextureUsage::TransferDst;
    assert(imageData.size() == desc.mipLevels * desc.arraySize);
    auto ret = textureManager_->Create(desc);
    for(uint32_t a = 0, i = 0; a < desc.arraySize; ++a)
    {
        for(uint32_t m = 0; m < desc.mipLevels; ++m)
        {
            UploadTexture2D(ret, a, m, imageData[i++]);
        }
    }
    return ret;
}

RC<Texture> CopyContext::CreateTexture2D(
    const RHI::TextureDesc &_desc,
    Span<ImageDynamic>      images)
{
    RHI::TextureDesc desc = _desc;
    desc.concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent;
    desc.usage |= RHI::TextureUsage::TransferDst;
    assert(images.size() == desc.mipLevels * desc.arraySize);
    auto ret = StatefulTexture::FromTexture(textureManager_->Create(desc));
    for(uint32_t a = 0, i = 0; a < desc.arraySize; ++a)
    {
        for(uint32_t m = 0; m < desc.mipLevels; ++m)
        {
            UploadTexture2D(ret, a, m, images[i++]);
        }
    }
    ret->SetState({ RHI::TextureLayout::CopyDst, RHI::PipelineStage::None, RHI::ResourceAccess::None });
    return ret;
}

RC<Texture> CopyContext::LoadTexture2D(
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
    const RHI::TextureDesc desc =
    {
        .dim           = RHI::TextureDimension::Tex2D,
        .format        = format,
        .width         = images[0].GetWidth(),
        .height        = images[0].GetHeight(),
        .arraySize     = 1,
        .mipLevels     = mipLevels,
        .sampleCount   = 1,
        .usage         = usages,
        .initialLayout = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent
    };
    return CreateTexture2D(desc, images);
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
