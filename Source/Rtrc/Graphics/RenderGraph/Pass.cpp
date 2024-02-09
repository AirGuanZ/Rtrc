#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>

RTRC_BEGIN

namespace GraphDetail
{

    thread_local RGPassContext *gCurrentPassContext = nullptr;

} // namespace GraphDetail

CommandBuffer &RGPassContext::GetCommandBuffer()
{
    return *commandBuffer_;
}

RC<Buffer> RGPassContext::Get(const RGBufImpl *resource)
{
    auto &result = resources_->indexToBuffer[resource->GetResourceIndex()].buffer;
    assert(result);
#if RTRC_RG_DEBUG
    if(!declaredResources_->contains(resource))
    {
        throw Exception(fmt::format(
            "Cannot use rg buffer resource '{}' without declaring usage", result->GetRHIObject()->GetName()));
    }
#endif
    return result;
}

const RC<Tlas> &RGPassContext::Get(const RGTlasImpl *resource)
{
    return resource->Get();
}

RC<Texture> RGPassContext::Get(const RGTexImpl *resource)
{
    auto &result = resources_->indexToTexture[resource->GetResourceIndex()].texture;
    assert(result);
#if RTRC_RG_DEBUG
    if(!resource->SkipDeclarationCheck() && !declaredResources_->contains(resource))
    {
        throw Exception(fmt::format(
            "Cannot use rg texture resource '{}' without declaring usage", result->GetRHIObject()->GetName()));
    }
#endif
    return result;
}

RGPassContext::RGPassContext(const RGExecutableResources &resources, CommandBuffer &commandBuffer)
    : resources_(resources), commandBuffer_(commandBuffer)
{
    assert(!GraphDetail::gCurrentPassContext);
    GraphDetail::gCurrentPassContext = this;
}

RGPassContext::~RGPassContext()
{
    assert(GraphDetail::gCurrentPassContext == this);
    GraphDetail::gCurrentPassContext = nullptr;
}

RGPassContext &RGGetPassContext()
{
    return *GraphDetail::gCurrentPassContext;
}

CommandBuffer &RGGetCommandBuffer()
{
    return RGGetPassContext().GetCommandBuffer();
}

void Connect(RGPassImpl *head, RGPassImpl *tail)
{
    assert(head->isExecuted_ || !tail->isExecuted_);
    if(head->isExecuted_)
    {
        return;
    }
    head->succs_.insert(tail);
    tail->prevs_.insert(head);
}

RGPassImpl *RGPassImpl::Use(RGBufImpl *buffer, const RGUseInfo &info)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    auto &usage = bufferUsages_[buffer];
    usage.stages |= info.stages;
    usage.accesses |= info.accesses;
    return this;
}

RGPassImpl *RGPassImpl::Use(RGTexImpl *texture, const RGUseInfo &info)
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

RGPassImpl *RGPassImpl::Use(RGTexImpl *texture, const TexSubrsc &subrsc, const RGUseInfo &info)
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

RGPassImpl *RGPassImpl::Use(RGTlasImpl *tlas, const RGUseInfo &info)
{
    return Use(tlas->GetInternalBuffer(), info);
}

RGPassImpl *RGPassImpl::SetCallback(Callback callback)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    callback_ = std::move(callback);
    return this;
}

RGPassImpl *RGPassImpl::SetCallback(LegacyCallback callback)
{
    return SetCallback([c = std::move(callback)] () mutable { c(RGGetPassContext()); });
}

RGPass RGPassImpl::SyncQueueBeforeExecution()
{
    assert(!syncBeforeExec_ && "SyncQueueBeforeExecution can only be called once on each render graph pass");
    syncBeforeExec_ = true;
    return this;
}

RGPassImpl *RGPassImpl::SetSignalFence(RHI::FenceRPtr fence)
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    signalFence_ = std::move(fence);
    return this;
}

RGPassImpl *RGPassImpl::ClearUAVOverlapGroup()
{
    assert(!isExecuted_ && "Can not setup already executed pass");
    uavOverlapGroup_ = {};
    return this;
}

RGPassImpl::RGPassImpl(int index, const RGLabelStack::Node *node)
    : index_(index), nameNode_(node), syncBeforeExec_(false), isExecuted_(false)
{

}

RTRC_END
