#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>

RTRC_RG_BEGIN

namespace GraphDetail
{

    thread_local PassContext *gCurrentPassContext = nullptr;

} // namespace GraphDetail

CommandBuffer &PassContext::GetCommandBuffer()
{
    return *commandBuffer_;
}

RC<Buffer> PassContext::Get(const BufferResource *resource)
{
    auto &result = resources_->indexToBuffer[resource->GetResourceIndex()].buffer;
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
    return resource->Get();
}

RC<Texture> PassContext::Get(const TextureResource *resource)
{
    auto &result = resources_->indexToTexture[resource->GetResourceIndex()].texture;
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
    assert(!GraphDetail::gCurrentPassContext);
    GraphDetail::gCurrentPassContext = this;
}

PassContext::~PassContext()
{
    assert(GraphDetail::gCurrentPassContext == this);
    GraphDetail::gCurrentPassContext = nullptr;
}

PassContext &GetCurrentPassContext()
{
    return *GraphDetail::gCurrentPassContext;
}

CommandBuffer &GetCurrentCommandBuffer()
{
    return GetCurrentPassContext().GetCommandBuffer();
}

void Connect(Pass *head, Pass *tail)
{
    assert(head->isExecuted_ || !tail->isExecuted_);
    if(head->isExecuted_)
    {
        return;
    }
    head->succs_.insert(tail);
    tail->prevs_.insert(head);
}

Pass *Pass::Use(const BufferResource *buffer, const UseInfo &info)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    auto &usage = bufferUsages_[buffer];
    usage.stages |= info.stages;
    usage.accesses |= info.accesses;
    return this;
}

Pass *Pass::Use(const TextureResource *texture, const UseInfo &info)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    const uint32_t mipLevels = texture->GetMipLevels();
    const uint32_t arraySize = texture->GetArraySize();
    for(uint32_t mi = 0; mi < mipLevels; ++mi)
    {
        for(uint32_t ai = 0; ai < arraySize; ++ai)
        {
            Use(texture, TexSubrsc{ mi, ai }, info);
        }
    }
    return this;
}

Pass *Pass::Use(const TextureResource *texture, const TexSubrsc &subrsc, const UseInfo &info)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    TextureUsage &usageMap = textureUsages_[texture];
    if(!usageMap.GetArrayLayerCount())
    {
        usageMap = TextureUsage(texture->GetMipLevels(), texture->GetArraySize());
    }
    auto &usage = usageMap(subrsc.mipLevel, subrsc.arrayLayer);
    if(usage.has_value())
    {
        assert(usage->layout == info.layout);
        usage->stages |= info.stages;
        usage->accesses |= info.accesses;
    }
    else
    {
        usage = SubTexUsage(RHI::INITIAL_QUEUE_SESSION_ID, info.layout, info.stages, info.accesses);
    }
    return this;
}

Pass *Pass::Use(const TlasResource *tlas, const UseInfo &info)
{
    return Use(tlas->GetInternalBuffer(), info);
}

Pass *Pass::SetCallback(Callback callback)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    callback_ = std::move(callback);
    return this;
}

Pass *Pass::SetCallback(LegacyCallback callback)
{
    return SetCallback([c = std::move(callback)] () mutable { c(GetCurrentPassContext()); });
}

Pass *Pass::SetSignalFence(RHI::FenceRPtr fence)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    signalFence_ = std::move(fence);
    return this;
}

Pass::Pass(int index, const LabelStack::Node *node)
    : index_(index), nameNode_(node), isExecuted_(false)
{

}

RTRC_RG_END
