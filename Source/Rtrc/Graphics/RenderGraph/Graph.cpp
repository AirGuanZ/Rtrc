#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

void RenderGraph::InternalBufferResource::SetDefaultStructStride(size_t stride)
{
    defaultStructStride = stride;
}

void RenderGraph::InternalBufferResource::SetDefaultTexelFormat(RHI::Format format)
{
    defaultTexelFormat = format;
}

size_t RenderGraph::InternalBufferResource::GetDefaultStructStride() const
{
    return defaultStructStride;
}

RHI::Format RenderGraph::InternalBufferResource::GetDefaultTexelFormat() const
{
    return defaultTexelFormat;
}

RenderGraph::RenderGraph(Ref<Device> device, Queue queue)
    : device_(device), queue_(std::move(queue)), recording_(true)
    , swapchainTexture_(nullptr), executableResource_(nullptr)
{
    
}

RGBufImpl *RenderGraph::CreateBuffer(const RHI::BufferDesc &desc, std::string name)
{
    assert(recording_);
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<InternalBufferResource>(this, index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

RGTexImpl *RenderGraph::CreateTexture(const RHI::TextureDesc &desc, std::string name)
{
    assert(recording_);
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<InternalTextureResource>(this, index);
    resource->rhiDesc = desc;
    resource->name = std::move(name);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

RGBufImpl *RenderGraph::CreateTexelBuffer(
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

RGBufImpl *RenderGraph::CreateStructuredBuffer(
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

RGBufImpl *RenderGraph::RegisterBuffer(RC<StatefulBuffer> buffer)
{
    assert(recording_);
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

RGTexImpl *RenderGraph::RegisterTexture(RC<StatefulTexture> texture)
{
    assert(recording_);
    const void *rhiPtr = texture->GetRHIObject().Get();
    if(auto it = externalResourceMap_.find(rhiPtr); it != externalResourceMap_.end())
    {
        auto extRsc = static_cast<ExternalTextureResource *>(textures_[it->second].get());
        if(extRsc->isReadOnlySampledTexture)
        {
            throw Exception(
                "RenderGraph::RegisterReadOnlyTexture: "
                "given texture is already registered as read-only external texture");
        }
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

RGTexImpl *RenderGraph::RegisterReadOnlyTexture(RC<Texture> texture)
{
    assert(recording_);
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

RGTlasImpl *RenderGraph::RegisterTlas(RC<Tlas> tlas, RGBufImpl *internalBuffer)
{
    assert(recording_);
    if(auto it = tlasResources_.find(internalBuffer); it != tlasResources_.end())
    {
        return it->second.get();
    }
    auto ret = new RGTlasImpl(std::move(tlas), internalBuffer);
    auto rsc = Box<RGTlasImpl>(ret);
    tlasResources_.insert({ internalBuffer, std::move(rsc) });
    return ret;
}

RGTexImpl *RenderGraph::RegisterSwapchainTexture(
    RHI::TextureRPtr             rhiTexture,
    RHI::BackBufferSemaphoreOPtr acquireSemaphore,
    RHI::BackBufferSemaphoreOPtr presentSemaphore)
{
    assert(recording_);
    if(swapchainTexture_)
    {
        assert(swapchainTexture_->texture->GetRHIObject() == rhiTexture);
        return swapchainTexture_;
    }

    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<SwapchainTexture>(this, index);
    resource->texture = MakeRC<WrappedStatefulTexture>(Texture::FromRHIObject(std::move(rhiTexture)), TexSubrscState{});
    resource->acquireSemaphore = std::move(acquireSemaphore);
    resource->presentSemaphore = std::move(presentSemaphore);
    swapchainTexture_ = resource.get();
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return swapchainTexture_;
}

RGTexImpl *RenderGraph::RegisterSwapchainTexture(RHI::SwapchainOPtr swapchain)
{
    return RegisterSwapchainTexture(
        swapchain->GetRenderTarget(), swapchain->GetAcquireSemaphore(), swapchain->GetPresentSemaphore());
}

void RenderGraph::SetCompleteFence(RHI::FenceOPtr  fence)
{
    assert(recording_);
    completeFence_.Swap(fence);
}

void RenderGraph::PushPassGroup(std::string name)
{
    assert(recording_);
    labelStack_.Push(std::move(name));
}

void RenderGraph::PopPassGroup()
{
    assert(recording_);
    labelStack_.Pop();
}

void RenderGraph::BeginUAVOverlap()
{
    assert(recording_);
    if(currentUAVOverlapGroupDepth_ > 0)
    {
        ++currentUAVOverlapGroupDepth_;
    }
    else
    {
        currentUAVOverlapGroup_.groupIndex = nextUAVOverlapGroupIndex_++;
        currentUAVOverlapGroupDepth_ = 1;
    }
}

void RenderGraph::EndUAVOverlap()
{
    assert(recording_);
    assert(currentUAVOverlapGroupDepth_);
    if(!--currentUAVOverlapGroupDepth_)
    {
        currentUAVOverlapGroup_.groupIndex = RGUavOverlapGroup::InvalidGroupIndex;
    }
}

RGPassImpl *RenderGraph::CreatePass(std::string name)
{
    assert(recording_);
    labelStack_.Push(std::move(name));
    auto node = labelStack_.GetCurrentNode();
    labelStack_.Pop();

    const int index = static_cast<int>(passes_.size());
    auto pass = Box<RGPassImpl>(new RGPassImpl(index, node));
    pass->uavOverlapGroup_ = currentUAVOverlapGroup_;

    passes_.push_back(std::move(pass));
    return passes_.back().get();
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

RTRC_END
