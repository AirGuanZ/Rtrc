#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

RC<Buffer> BufferResource::Get() const
{
    if(!graph_->executableResource_)
    {
        throw Exception("BufferResource::GetRHI can not be called during graph execution");
    }
    return graph_->executableResource_->indexToBuffer[GetResourceIndex()].buffer;
}

RC<Texture> TextureResource::Get() const
{
    if(!graph_->executableResource_)
    {
        throw Exception("TextureResource::GetRHI can not be called during graph execution");
    }
    return graph_->executableResource_->indexToTexture[GetResourceIndex()].texture;
}

CommandBuffer &PassContext::GetCommandBuffer()
{
    return commandBuffer_;
}

RC<Buffer> PassContext::GetBuffer(const BufferResource *resource)
{
    auto &result = resources_.indexToBuffer[resource->GetResourceIndex()].buffer;
    assert(result);
    return result;
}

RC<Texture> PassContext::GetTexture2D(const TextureResource *resource)
{
    auto &result = resources_.indexToTexture[resource->GetResourceIndex()].texture;
    assert(result);
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

Pass *Pass::Use(BufferResource *buffer, RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
{
    assert(!bufferUsages_.contains(buffer));
    bufferUsages_.insert({ buffer, BufferUsage{ stages, accesses } });
    return this;
}

Pass *Pass::Use(
    TextureResource        *texture,
    RHI::TextureLayout      layout,
    RHI::PipelineStageFlag  stages,
    RHI::ResourceAccessFlag accesses)
{
    const uint32_t mipLevels = texture->GetMipLevels();
    const uint32_t arraySize = texture->GetArraySize();
    for(uint32_t mi = 0; mi < mipLevels; ++mi)
    {
        for(uint32_t ai = 0; ai < arraySize; ++ai)
        {
            Use(texture, RHI::TextureSubresource{ mi, ai }, layout, stages, accesses);
        }
    }
    return this;
}

Pass *Pass::Use(
    TextureResource               *texture,
    const RHI::TextureSubresource &subresource,
    RHI::TextureLayout             layout,
    RHI::PipelineStageFlag         stages,
    RHI::ResourceAccessFlag        accesses)
{
    TextureUsage &usageMap = textureUsages_[texture];
    if(!usageMap.GetArrayLayerCount())
    {
        usageMap = TextureUsage(texture->GetMipLevels(), texture->GetArraySize());
    }
    assert(!usageMap(subresource.mipLevel, subresource.arrayLayer).has_value());
    usageMap(subresource.mipLevel, subresource.arrayLayer) = SubTexUsage::Create(stages, accesses, layout);
    return this;
}

Pass *Pass::Use(BufferResource *buffer, const UseInfo &info)
{
    Use(buffer, info.stages, info.accesses);
    return this;
}

Pass *Pass::Use(TextureResource *texture, const UseInfo &info)
{
    Use(texture, info.layout, info.stages, info.accesses);
    return this;
}

Pass *Pass::Use(TextureResource *texture, const RHI::TextureSubresource &subrsc, const UseInfo &info)
{
    Use(texture, subrsc, info.layout, info.stages, info.accesses);
    return this;
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

Pass::Pass(int index, std::string name)
    : index_(index), name_(std::move(name))
{
    
}

RenderGraph::RenderGraph(Queue queue)
    : queue_(std::move(queue)), swapchainTexture_(nullptr), executableResource_(nullptr)
{
    
}

void RenderGraph::SetQueue(Queue queue)
{
    queue_ = std::move(queue);
}

BufferResource *RenderGraph::CreateBuffer(const RHI::BufferDesc &desc)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<InternalBufferResource>(this, index);
    resource->rhiDesc = desc;
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::CreateTexture2D(const RHI::TextureDesc &desc)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<InternalTextureResource>(this, index);
    resource->rhiDesc = desc;
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

BufferResource *RenderGraph::RegisterBuffer(RC<Buffer> buffer)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<ExternalBufferResource>(this, index);
    resource->buffer = std::move(buffer);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::RegisterTexture(RC<Texture> texture)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<ExternalTextureResource>(this, index);
    resource->texture = std::move(texture);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

TextureResource *RenderGraph::RegisterSwapchainTexture(
    RHI::TexturePtr             rhiTexture,
    RHI::BackBufferSemaphorePtr acquireSemaphore,
    RHI::BackBufferSemaphorePtr presentSemaphore)
{
    assert(!swapchainTexture_);
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<SwapchainTexture>(this, index);
    resource->texture = MakeRC<Texture>(Texture::FromRHIObject(std::move(rhiTexture)));
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

Pass *RenderGraph::CreatePass(std::string name)
{
    const int index = static_cast<int>(passes_.size());
    auto pass = Box<Pass>(new Pass(index, std::move(name)));
    passes_.push_back(std::move(pass));
    return passes_.back().get();
}

const RHI::TextureDesc &RenderGraph::ExternalTextureResource::GetDesc() const
{
    return texture->GetRHIObjectDesc();
}

const RHI::TextureDesc &RenderGraph::InternalTextureResource::GetDesc() const
{
    return rhiDesc;
}

RTRC_RG_END