#include <Rtrc/RenderGraph/Executable.h>
#include <Rtrc/RenderGraph/Graph.h>

RTRC_RG_BEGIN

const RHI::CommandBufferPtr &PassContext::GetRHICommandBuffer()
{
    return commandBuffer_;
}

RHI::BufferPtr PassContext::GetRHIBuffer(const BufferResource *resource)
{
    auto &result = resources_.indexToRHIBuffers[resource->GetResourceIndex()];
    assert(result);
    return result->GetRHIBuffer();
}

RHI::TexturePtr PassContext::GetRHITexture(const TextureResource *resource)
{
    auto &result = resources_.indexToRHITextures[resource->GetResourceIndex()];
    assert(result);
    return result->GetRHITexture();
}

PassContext::PassContext(const ExecutableResources &resources, RHI::CommandBufferPtr commandBuffer)
    : resources_(resources), commandBuffer_(std::move(commandBuffer))
{
    
}

void Connect(Pass *head, Pass *tail)
{
    head->succs_.insert(tail);
    tail->prevs_.insert(head);
}

void Pass::Use(BufferResource *buffer, RHI::PipelineStageFlag stages, RHI::ResourceAccessFlag accesses)
{
    assert(!bufferUsages_.contains(buffer));
    bufferUsages_.insert({ buffer, BufferUsage{ stages, accesses } });
}

void Pass::Use(
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
}

void Pass::Use(
    TextureResource               *texture,
    const RHI::TextureSubresource &subresource,
    RHI::TextureLayout             layout,
    RHI::PipelineStageFlag         stages,
    RHI::ResourceAccessFlag        accesses)
{
    TextureUsage &usageMap = textureUsages_[texture];
    assert(!usageMap(subresource.mipLevel, subresource.arrayLayer).has_value());
    usageMap(subresource.mipLevel, subresource.arrayLayer) = SubTexUsage{ layout, stages, accesses };
}

void Pass::SetCallback(Callback callback)
{
    callback_ = std::move(callback);
}

void Pass::SetSignalFence(RHI::FencePtr fence)
{
    signalFence_ = std::move(fence);
}

Pass::Pass(int index, std::string name, RHI::QueuePtr queue)
    : index_(index), name_(std::move(name)), queue_(std::move(queue))
{
    
}

RenderGraph::RenderGraph()
    : swapchainTexture_(nullptr)
{
    
}

BufferResource *RenderGraph::CreateBuffer(const RHI::BufferDesc &desc)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<InternalBufferResource>(index);
    resource->rhiDesc = desc;
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::CreateTexture2D(const RHI::Texture2DDesc &desc)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<InternalTexture2DResource>(index);
    resource->rhiDesc = desc;
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

BufferResource *RenderGraph::RegisterBuffer(RC<RHI::StatefulBuffer> buffer)
{
    const int index = static_cast<int>(buffers_.size());
    auto resource = MakeBox<ExternalBufferResource>(index);
    resource->buffer = std::move(buffer);
    buffers_.push_back(std::move(resource));
    textures_.push_back(nullptr);
    return buffers_.back().get();
}

TextureResource *RenderGraph::RegisterTexture(RC<RHI::StatefulTexture> texture)
{
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<ExternalTextureResource>(index);
    resource->texture = std::move(texture);
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return textures_.back().get();
}

TextureResource *RenderGraph::RegisterSwapchainTexture(
    RC<RHI::StatefulTexture>    texture,
    RHI::BackBufferSemaphorePtr acquireSemaphore,
    RHI::BackBufferSemaphorePtr presentSemaphore)
{
    assert(!swapchainTexture_);
    const int index = static_cast<int>(textures_.size());
    auto resource = MakeBox<SwapchainTexture>(index);
    resource->texture = std::move(texture);
    resource->acquireSemaphore = std::move(acquireSemaphore);
    resource->presentSemaphore = std::move(presentSemaphore);
    swapchainTexture_ = resource.get();
    textures_.push_back(std::move(resource));
    buffers_.push_back(nullptr);
    return swapchainTexture_;
}

Pass *RenderGraph::CreatePass(std::string name, RHI::QueuePtr queue)
{
    const int index = static_cast<int>(passes_.size());
    auto pass = Box<Pass>(new Pass(index, std::move(name), std::move(queue)));
    passes_.push_back(std::move(pass));
    return passes_.back().get();
}

uint32_t RenderGraph::ExternalTextureResource::GetMipLevels() const
{
    return texture->GetRHITexture()->GetMipLevels();
}

uint32_t RenderGraph::ExternalTextureResource::GetArraySize() const
{
    return texture->GetRHITexture()->GetArraySize();
}

uint32_t RenderGraph::InternalTexture2DResource::GetMipLevels() const
{
    return rhiDesc.mipLevels;
}

uint32_t RenderGraph::InternalTexture2DResource::GetArraySize() const
{
    return rhiDesc.arraySize;
}

RTRC_RG_END
