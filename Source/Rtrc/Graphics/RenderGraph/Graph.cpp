#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

void RenderGraph::InternalBufferResource::SetDefaultStructStride(size_t stride)
{
    defaultStructStride_ = stride;
}

void RenderGraph::InternalBufferResource::SetDefaultTexelFormat(RHI::Format format)
{
    defaultTexelFormat_ = format;
}

size_t RenderGraph::InternalBufferResource::GetDefaultStructStride() const
{
    return defaultStructStride_;
}

RHI::Format RenderGraph::InternalBufferResource::GetDefaultTexelFormat() const
{
    return defaultTexelFormat_;
}

RenderGraph::RenderGraph(Ref<Device> device, Queue queue)
    : device_(device), queue_(std::move(queue)), swapchainTexture_(nullptr), executableResource_(nullptr)
{
    
}

BufferResource *RenderGraph::CreateBuffer(const RHI::BufferDesc &desc, std::string name)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<InternalBufferResource>(this, index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::CreateTexture(const RHI::TextureDesc &desc, std::string name)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<InternalTextureResource>(this, index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

BufferResource *RenderGraph::CreateTexelBuffer(
    size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name)
{
    auto ret = this->CreateBuffer(RHI::BufferDesc
    {
        .size = count * RHI::GetTexelSize(format),
        .usage = usages
    }, std::move(name));
    ret->SetDefaultTexelFormat(format);
    return ret;
}

BufferResource *RenderGraph::CreateStructuredBuffer(
    size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name)
{
    auto ret = this->CreateBuffer(RHI::BufferDesc
    {
        .size = count * stride,
        .usage = usages
    }, std::move(name));
    ret->SetDefaultStructStride(stride);
    return ret;
}

BufferResource *RenderGraph::RegisterBuffer(RC<StatefulBuffer> buffer)
{
    const void *rhiPtr = buffer->GetRHIObject().Get();
    if(auto it = externalResourceMap_.find(rhiPtr); it != externalResourceMap_.end())
    {
        return buffers_[it->second].get();
    }
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<ExternalBufferResource>(this, index);
    resource->buffer = std::move(buffer);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    externalResourceMap_.insert({ rhiPtr, index });
    return buffers_.back().get();
}

TextureResource *RenderGraph::RegisterTexture(RC<StatefulTexture> texture)
{
    const void *rhiPtr = texture->GetRHIObject().Get();
    if(auto it = externalResourceMap_.find(rhiPtr); it != externalResourceMap_.end())
    {
        return textures_[it->second].get();
    }
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<ExternalTextureResource>(this, index);
    resource->texture = std::move(texture);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    externalResourceMap_.insert({ rhiPtr, index });
    return textures_.back().get();
}

TextureResource *RenderGraph::RegisterReadOnlyTexture(RC<Texture> texture)
{
    const void *rhiPtr = texture->GetRHIObject().Get();
    if(auto it = externalResourceMap_.find(rhiPtr); it != externalResourceMap_.end())
    {
        auto extRsc = static_cast<ExternalTextureResource *>(textures_[it->second].get());
        if(!extRsc->isReadOnlySampledTexture)
        {
            throw Exception(
                "RenderGraph::RegisterReadOnlyTexture: "
                "given texture is already registered as non-read-only external texture");
        }
        return extRsc;
    }

    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<ExternalTextureResource>(this, index);
    resource->texture = StatefulTexture::FromTexture(std::move(texture));
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    externalResourceMap_.insert({ rhiPtr, index });
    return textures_.back().get();
}

TlasResource *RenderGraph::RegisterTlas(RC<Tlas> tlas, BufferResource *internalBuffer)
{
    if(auto it = tlasResources_.find(internalBuffer); it != tlasResources_.end())
    {
        return it->second.get();
    }
    auto ret = new TlasResource(std::move(tlas), internalBuffer);
    auto rsc = Box<TlasResource>(ret);
    tlasResources_.insert({ internalBuffer, std::move(rsc) });
    return ret;
}

TextureResource *RenderGraph::RegisterSwapchainTexture(
    RHI::TextureRPtr             rhiTexture,
    RHI::BackBufferSemaphoreOPtr acquireSemaphore,
    RHI::BackBufferSemaphoreOPtr presentSemaphore)
{
    if(swapchainTexture_)
    {
        assert(swapchainTexture_->texture->GetRHIObject() == rhiTexture);
        return swapchainTexture_;
    }

    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<SwapchainTexture>(this, index);
    resource->texture = MakeRC<WrappedStatefulTexture>(
        Texture::FromRHIObject(std::move(rhiTexture)), TextureSubrscState{});
    resource->acquireSemaphore = std::move(acquireSemaphore);
    resource->presentSemaphore = std::move(presentSemaphore);
    swapchainTexture_ = resource.get();
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return swapchainTexture_;
}

TextureResource *RenderGraph::RegisterSwapchainTexture(RHI::SwapchainOPtr swapchain)
{
    return RegisterSwapchainTexture(
        swapchain->GetRenderTarget(), swapchain->GetAcquireSemaphore(), swapchain->GetPresentSemaphore());
}

void RenderGraph::SetCompleteFence(RHI::FenceOPtr  fence)
{
    completeFence_.Swap(fence);
}

void RenderGraph::PushPassGroup(std::string name)
{
    labelStack_.Push(std::move(name));
}

void RenderGraph::PopPassGroup()
{
    labelStack_.Pop();
}

Pass *RenderGraph::CreatePass(std::string name)
{
    labelStack_.Push(std::move(name));
    auto node = labelStack_.GetCurrentNode();
    labelStack_.Pop();

    const int index = static_cast<int>(passes_.size());
    auto pass = Box<Pass>(new Pass(index, node));
    passes_.push_back(std::move(pass));
    return passes_.back().get();
}

Pass *RenderGraph::CreateClearTexture2DPass(std::string name, TextureResource *tex2D, const Vector4f &clearValue)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(tex2D, ClearDst);
    pass->SetCallback([tex2D, clearValue](PassContext &context)
    {
        context.GetCommandBuffer().ClearColorTexture2D(tex2D->Get(), clearValue);
    });
    return pass;
}

Pass *RenderGraph::CreateDummyPass(std::string name)
{
    return CreatePass(std::move(name));
}

Pass *RenderGraph::CreateClearRWBufferPass(std::string name, BufferResource *buffer, uint32_t value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(buffer, CS_RWBuffer_WriteOnly);
    pass->SetCallback([buffer, value, this]
    {
        device_->GetClearBufferUtils().ClearRWBuffer(GetCurrentCommandBuffer(), buffer->Get(), value);
    });
    return pass;
}

Pass *RenderGraph::CreateClearRWStructuredBufferPass(std::string name, BufferResource *buffer, uint32_t value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(buffer, CS_RWStructuredBuffer_WriteOnly);
    pass->SetCallback([buffer, value, this]
    {
        device_->GetClearBufferUtils().ClearRWStructuredBuffer(GetCurrentCommandBuffer(), buffer->Get(), value);
    });
    return pass;
}

Pass *RenderGraph::CreateCopyBufferPass(
    std::string name, BufferResource *src, size_t srcOffset, BufferResource *dst, size_t dstOffset, size_t size)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(src, CopySrc);
    pass->Use(dst, CopyDst);
    pass->SetCallback([=]
    {
        GetCurrentCommandBuffer().CopyBuffer(dst, dstOffset, src, srcOffset, size);
    });
    return pass;
}

Pass *RenderGraph::CreateCopyBufferPass(std::string name, BufferResource *src, BufferResource *dst, size_t size)
{
    return CreateCopyBufferPass(std::move(name), src, 0, dst, 0, size);
}

Pass *RenderGraph::CreateCopyColorTexturePass(std::string name, TextureResource *src, TextureResource *dst)
{
    assert(src->GetArraySize() == dst->GetArraySize());
    assert(src->GetMipLevels() == dst->GetMipLevels());
    assert(src->GetSize() == dst->GetSize());
    return CreateCopyColorTexturePass(std::move(name), src, dst, TextureSubresources
    {
        .mipLevel = 0,
        .levelCount = src->GetMipLevels(),
        .arrayLayer = 0,
        .layerCount = dst->GetArraySize()
    });
}

Pass *RenderGraph::CreateCopyColorTexturePass(
    std::string                name,
    TextureResource           *src,
    TextureResource           *dst,
    const TextureSubresources &subrscs)
{
    return CreateCopyColorTexturePass(std::move(name), src, subrscs, dst, subrscs);
}

Pass *RenderGraph::CreateCopyColorTexturePass(
    std::string                name,
    TextureResource           *src,
    const TextureSubresources &srcSubrscs,
    TextureResource           *dst,
    const TextureSubresources &dstSubrscs)
{
    assert(srcSubrscs.layerCount == dstSubrscs.layerCount);
    assert(srcSubrscs.levelCount == dstSubrscs.levelCount);
    auto pass = CreatePass(std::move(name));
    for(unsigned a = srcSubrscs.arrayLayer; a < srcSubrscs.arrayLayer + srcSubrscs.layerCount; ++a)
    {
        assert(a < src->GetArraySize());
        for(unsigned m = srcSubrscs.mipLevel; m < srcSubrscs.mipLevel + srcSubrscs.levelCount; ++m)
        {
            assert(m < src->GetMipLevels());
            pass->Use(src, TextureSubresource{ m, a }, CopySrc);
        }
    }
    for(unsigned a = dstSubrscs.arrayLayer; a < dstSubrscs.arrayLayer + dstSubrscs.layerCount; ++a)
    {
        assert(a < dst->GetArraySize());
        for(unsigned m = dstSubrscs.mipLevel; m < dstSubrscs.mipLevel + dstSubrscs.levelCount; ++m)
        {
            assert(m < dst->GetMipLevels());
            pass->Use(dst, TextureSubresource{ m, a }, CopyDst);
        }
    }
    pass->SetCallback([src, srcSubrscs, dst, dstSubrscs]
    {
        auto &cmds = GetCurrentCommandBuffer();
        auto tsrc = src->Get();
        auto tdst = dst->Get();
        for(unsigned ai = 0; ai < srcSubrscs.layerCount; ++ai)
        {
            const unsigned srcArrayLayer = srcSubrscs.arrayLayer + ai;
            const unsigned dstArrayLayer = dstSubrscs.arrayLayer + ai;
            for(unsigned mi = 0; mi < srcSubrscs.levelCount; ++mi)
            {
                const unsigned srcMipLevel = srcSubrscs.mipLevel + mi;
                const unsigned dstMipLevel = dstSubrscs.mipLevel + mi;
                assert(tsrc->GetSize(srcMipLevel) == tdst->GetSize(dstMipLevel));
                cmds.CopyColorTexture(*tdst, dstMipLevel, dstArrayLayer, *tsrc, srcMipLevel, srcArrayLayer);
            }
        }
    });
    return pass;
}

Pass *RenderGraph::CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4f &value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value, this]
    {
        device_->GetClearTextureUtils().ClearRWTexture2D(GetCurrentCommandBuffer(), tex2D->GetUavImm(), value);
    });
    return pass;
}

Pass *RenderGraph::CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4i &value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value, this]
    {
        device_->GetClearTextureUtils().ClearRWTexture2D(GetCurrentCommandBuffer(), tex2D->GetUavImm(), value);
    });
    return pass;
}

Pass *RenderGraph::CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4u &value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(tex2D, CS_RWTexture);
    pass->SetCallback([tex2D, value, this]
    {
        device_->GetClearTextureUtils().ClearRWTexture2D(GetCurrentCommandBuffer(), tex2D->GetUavImm(), value);
    });
    return pass;
}

Pass *RenderGraph::CreateBlitTexture2DPass(
    std::string name,
    TextureResource *src, uint32_t srcArrayLayer, uint32_t srcMipLevel,
    TextureResource *dst, uint32_t dstArrayLayer, uint32_t dstMipLevel,
    bool usePointSampling, float gamma)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(src, PS_Texture);
    pass->Use(dst, ColorAttachmentWriteOnly);
    pass->SetCallback([=, this](PassContext &context)
    {
        device_->GetCopyTextureUtils().RenderFullscreenTriangle(
            context.GetCommandBuffer(),
            src->GetSrvImm(srcMipLevel, 1, srcArrayLayer),
            dst->GetRtvImm(dstMipLevel, dstArrayLayer),
            usePointSampling ? CopyTextureUtils::Point : CopyTextureUtils::Linear, gamma);
    });
    return pass;
}

Pass *RenderGraph::CreateBlitTexture2DPass(
    std::string name, TextureResource *src, TextureResource *dst, bool usePointSampling, float gamma)
{
    assert(src->GetArraySize() == 1);
    assert(src->GetMipLevels() == 1);
    assert(dst->GetArraySize() == 1);
    assert(dst->GetMipLevels() == 1);
    return CreateBlitTexture2DPass(
        std::move(name), src, 0, 0, dst, 0, 0, usePointSampling, gamma);
}

size_t RenderGraph::ExternalBufferResource::GetDefaultStructStride() const
{
    return buffer->GetDefaultStructStride();
}

RHI::Format RenderGraph::ExternalBufferResource::GetDefaultTexelFormat() const
{
    return buffer->GetDefaultTexelFormat();
}

void RenderGraph::ExternalBufferResource::SetDefaultTexelFormat(RHI::Format format)
{
    buffer->SetDefaultTexelFormat(format);
}

void RenderGraph::ExternalBufferResource::SetDefaultStructStride(size_t stride)
{
    buffer->SetDefaultStructStride(stride);
}

const RHI::TextureDesc &RenderGraph::ExternalTextureResource::GetDesc() const
{
    return texture->GetDesc();
}

const RHI::TextureDesc &RenderGraph::InternalTextureResource::GetDesc() const
{
    return rhiDesc;
}

RTRC_RG_END
