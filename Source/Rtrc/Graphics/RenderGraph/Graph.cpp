#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

RC<Buffer> BufferResource::Get(PassContext &passContext) const
{
    return passContext.Get(this);
}

const RC<Tlas> &TlasResource::Get(PassContext &passContext) const
{
    assert(passContext.Get(tlasBuffer_));
    return tlas_;
}

RC<Texture> TextureResource::Get(PassContext &passContext) const
{
    return passContext.Get(this);
}

CommandBuffer &PassContext::GetCommandBuffer()
{
    return commandBuffer_;
}

RC<Buffer> PassContext::Get(const BufferResource *resource)
{
    auto &result = resources_.indexToBuffer[resource->GetResourceIndex()].buffer;
    assert(result);
#if RTRC_RG_DEBUG
    if(!declaredResources_->contains(resource))
    {
        throw Exception(fmt::format(
            "Cannot use rg buffer resource {} without declaring usage", result->GetRHIObject()->GetName()));
    }
#endif
    return result;
}

const RC<Tlas> &PassContext::Get(const TlasResource *resource)
{
    return resource->Get(*this);
}

RC<Texture> PassContext::Get(const TextureResource *resource)
{
    auto &result = resources_.indexToTexture[resource->GetResourceIndex()].texture;
    assert(result);
#if RTRC_RG_DEBUG
    if(!declaredResources_->contains(resource))
    {
        throw Exception(fmt::format(
            "Cannot use rg texture resource {} without declaring usage", result->GetRHIObject()->GetName()));
    }
#endif
    return result;
}

PassContext::PassContext(const ExecutableResources &resources, CommandBuffer &commandBuffer)
    : resources_(resources), commandBuffer_(commandBuffer)
{
    
}

void Connect(Pass *head, Pass *tail)
{
    head->succs_.insert(tail);
    tail->prevs_.insert(head);
}

Pass *Pass::Use(BufferResource *buffer, const UseInfo &info)
{
    auto &usage = bufferUsages_[buffer];
    usage.stages |= info.stages;
    usage.accesses |= info.accesses;
    return this;
}

Pass *Pass::Use(TextureResource *texture, const UseInfo &info)
{
    const uint32_t mipLevels = texture->GetMipLevels();
    const uint32_t arraySize = texture->GetArraySize();
    for(uint32_t mi = 0; mi < mipLevels; ++mi)
    {
        for(uint32_t ai = 0; ai < arraySize; ++ai)
        {
            Use(texture, RHI::TextureSubresource{ mi, ai }, info);
        }
    }
    return this;
}

Pass *Pass::Use(TextureResource *texture, const RHI::TextureSubresource &subrsc, const UseInfo &info)
{
    TextureUsage &usageMap = textureUsages_[texture];
    if(!usageMap.GetArrayLayerCount())
    {
        usageMap = TextureUsage(texture->GetMipLevels(), texture->GetArraySize());
    }
    assert(!usageMap(subrsc.mipLevel, subrsc.arrayLayer).has_value());
    usageMap(subrsc.mipLevel, subrsc.arrayLayer) = SubTexUsage(info.layout, info.stages, info.accesses);
    return this;
}

Pass *Pass::Use(TlasResource *tlas, const UseInfo &info)
{
    return Use(tlas->GetInternalBuffer(), info);
}

Pass *Pass::Build(TlasResource *tlas)
{
    return Use(tlas, BuildAS_Output);
}

Pass *Pass::Read(TlasResource *tlas, RHI::PipelineStageFlag stages)
{
    return Use(tlas, UseInfo
    {
        .layout   = RHI::TextureLayout::Undefined,
        .stages   = stages,
        .accesses = RHI::ResourceAccess::ReadAS
    });
}

Pass *Pass::SetCallback(Callback callback)
{
    callback_ = std::move(callback);
    return this;
}

Pass *Pass::SetSignalFence(RHI::FencePtr fence)
{
    signalFence_ = std::move(fence);
    return this;
}

Pass::Pass(int index, const LabelStack::Node *node)
    : index_(index), nameNode_(node)
{
    
}

RenderGraph::RenderGraph(ObserverPtr<Device> device, Queue queue)
    : device_(device), queue_(std::move(queue)), swapchainTexture_(nullptr), executableResource_(nullptr)
{
    
}

BufferResource *RenderGraph::CreateBuffer(const RHI::BufferDesc &desc, std::string name)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<InternalBufferResource>(index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::CreateTexture(const RHI::TextureDesc &desc, std::string name)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<InternalTextureResource>(index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

BufferResource *RenderGraph::RegisterBuffer(RC<StatefulBuffer> buffer)
{
    const void *rhiPtr = buffer->GetRHIObject().Get();
    if(auto it = externalResourceMap_.find(rhiPtr); it != externalResourceMap_.end())
    {
        return buffers_[it->second].get();
    }
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<ExternalBufferResource>(index);
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
    auto resource = MakeBox<ExternalTextureResource>(index);
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
    auto resource = MakeBox<ExternalTextureResource>(index);
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
    RHI::TexturePtr             rhiTexture,
    RHI::BackBufferSemaphorePtr acquireSemaphore,
    RHI::BackBufferSemaphorePtr presentSemaphore)
{
    assert(!swapchainTexture_);
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<SwapchainTexture>(index);
    resource->texture = MakeRC<WrappedStatefulTexture>(
        Texture::FromRHIObject(std::move(rhiTexture)), TextureSubrscState{});
    resource->acquireSemaphore = std::move(acquireSemaphore);
    resource->presentSemaphore = std::move(presentSemaphore);
    swapchainTexture_ = resource.get();
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return swapchainTexture_;
}

TextureResource *RenderGraph::RegisterSwapchainTexture(const RHI::SwapchainPtr &swapchain)
{
    return RegisterSwapchainTexture(
        swapchain->GetRenderTarget(), swapchain->GetAcquireSemaphore(), swapchain->GetPresentSemaphore());
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
        context.GetCommandBuffer().ClearColorTexture2D(context.Get(tex2D), clearValue);
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
    pass->SetCallback([buffer, value, this](PassContext &context)
    {
        device_->GetClearBufferUtils().ClearRWBuffer(context.GetCommandBuffer(), context.Get(buffer), value);
    });
    return pass;
}

Pass *RenderGraph::CreateClearRWStructuredBufferPass(std::string name, BufferResource *buffer, uint32_t value)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(buffer, CS_RWStructuredBuffer_WriteOnly);
    pass->SetCallback([buffer, value, this](PassContext &context)
    {
        device_->GetClearBufferUtils().ClearRWStructuredBuffer(context.GetCommandBuffer(), context.Get(buffer), value);
    });
    return pass;
}

Pass *RenderGraph::CreateCopyBufferPass(
    std::string name, BufferResource *src, size_t srcOffset, BufferResource *dst, size_t dstOffset, size_t size)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(src, CopySrc);
    pass->Use(dst, CopyDst);
    pass->SetCallback([=](PassContext &context)
    {
        context.GetCommandBuffer().CopyBuffer(*dst->Get(context), dstOffset, *src->Get(context), srcOffset, size);
    });
    return pass;
}

Pass *RenderGraph::CreateCopyBufferPass(std::string name, BufferResource *src, BufferResource *dst, size_t size)
{
    return CreateCopyBufferPass(std::move(name), src, 0, dst, 0, size);
}

Pass *RenderGraph::CreateBlitTexture2DPass(
    std::string name,
    TextureResource *src, uint32_t srcArrayLayer, uint32_t srcMipLevel,
    TextureResource *dst, uint32_t dstArrayLayer, uint32_t dstMipLevel,
    bool usePointSampling)
{
    auto pass = CreatePass(std::move(name));
    pass->Use(src, PS_Texture);
    pass->Use(dst, ColorAttachmentWriteOnly);
    pass->SetCallback([=](PassContext &context)
    {
        device_->GetCopyTextureUtils().RenderQuad(
            context.GetCommandBuffer(),
            src->Get(context)->CreateSrv(srcMipLevel, 1, srcArrayLayer),
            dst->Get(context)->CreateRtv(dstMipLevel, dstArrayLayer),
            usePointSampling ? CopyTextureUtils::Point : CopyTextureUtils::Linear);
    });
    return pass;
}

Pass *RenderGraph::CreateBlitTexture2DPass(
    std::string name, TextureResource *src, TextureResource *dst, bool usePointSampling)
{
    assert(src->GetArraySize() == 1);
    assert(src->GetMipLevels() == 1);
    assert(dst->GetArraySize() == 1);
    assert(dst->GetMipLevels() == 1);
    return CreateBlitTexture2DPass(std::move(name), src, 0, 0, dst, 0, 0, usePointSampling);
}

void RenderGraph::SetCompleteFence(RHI::FencePtr fence)
{
    completeFence_.Swap(fence);
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
