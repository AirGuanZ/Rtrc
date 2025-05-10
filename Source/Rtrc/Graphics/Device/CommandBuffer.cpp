#include <ranges>
#include <shared_mutex>

#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Core/Enumerate.h>

RTRC_BEGIN

BarrierBatch &BarrierBatch::operator()(
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    if(!G_)
    {
        G_ = RHI::GlobalMemoryBarrier{
            .beforeStages   = RHI::PipelineStage::None,
            .beforeAccesses = RHI::ResourceAccess::None,
            .afterStages    = RHI::PipelineStage::None,
            .afterAccesses  = RHI::ResourceAccess::None
        };
    }
    G_->beforeStages   |= prevStages;
    G_->beforeAccesses |= prevAccesses;
    G_->afterStages    |= succStages;
    G_->afterAccesses  |= succAccesses;
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<Buffer>       &buffer,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    BT_.push_back(RHI::BufferTransitionBarrier
    {
        .buffer         = buffer->GetRHIObject().Get(),
        .beforeStages   = prevStages,
        .beforeAccesses = prevAccesses,
        .afterStages    = succStages,
        .afterAccesses  = succAccesses
    });
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<Texture>      &texture,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    assert(!DynamicCast<StatefulTexture>(texture));
    for(auto [m, a] : EnumerateSubTextures(texture->GetDesc()))
        operator()(texture, a, m, prevLayout, prevStages, prevAccesses, succLayout, succStages, succAccesses);
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<Texture>      &texture,
    uint32_t                arrayLayer,
    uint32_t                mipLevel,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    assert(!DynamicCast<StatefulTexture>(texture));
    TT_.push_back(RHI::TextureTransitionBarrier
    {
        .texture        = texture->GetRHIObject().Get(),
        .subresources   = TexSubrscs{ mipLevel, 1, arrayLayer, 1 },
        .beforeStages   = prevStages,
        .beforeAccesses = prevAccesses,
        .beforeLayout   = prevLayout,
        .afterStages    = succStages,
        .afterAccesses  = succAccesses,
        .afterLayout    = succLayout
    });
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<Texture> &texture,
    RHI::TextureLayout prevLayout,
    RHI::TextureLayout succLayout)
{
    assert(!DynamicCast<StatefulTexture>(texture));
    return operator()(
        texture,
        prevLayout, RHI::PipelineStage::All, RHI::ResourceAccess::All,
        succLayout, RHI::PipelineStage::All, RHI::ResourceAccess::All);
}

BarrierBatch &BarrierBatch::Add(std::vector<RHI::BufferTransitionBarrier> &&bufferBarriers)
{
    if(BT_.empty())
    {
        BT_ = std::move(bufferBarriers);
    }
    else
    {
        std::ranges::copy(bufferBarriers, std::back_inserter(BT_));
    }
    return *this;
}

BarrierBatch &BarrierBatch::Add(std::vector<RHI::TextureTransitionBarrier> &&textureBarriers)
{
    if(TT_.empty())
    {
        TT_ = std::move(textureBarriers);
    }
    else
    {
        std::ranges::copy(textureBarriers, std::back_inserter(TT_));
    }
    return *this;
}

BarrierBatch &BarrierBatch::Add(const std::vector<RHI::BufferTransitionBarrier> &bufferBarriers)
{
    std::ranges::copy(bufferBarriers, std::back_inserter(BT_));
    return *this;
}

BarrierBatch &BarrierBatch::Add(const std::vector<RHI::TextureTransitionBarrier> &textureBarriers)
{
    std::ranges::copy(textureBarriers, std::back_inserter(TT_));
    return *this;
}

BarrierBatch &BarrierBatch::Add(const std::vector<RHI::TextureReleaseBarrier> &textureBarriers)
{
    TR_.append_range(textureBarriers);
    return *this;
}

BarrierBatch &BarrierBatch::Add(const std::vector<RHI::TextureAcquireBarrier> &textureBarriers)
{
    TA_.append_range(textureBarriers);
    return *this;
}

CommandBuffer::CommandBuffer()
    : device_(nullptr), manager_(nullptr), queueType_(RHI::QueueType::Graphics), pool_(nullptr)
{
    
}

CommandBuffer::~CommandBuffer()
{
    if(rhiCommandBuffer_)
    {
        manager_->_internalRelease(*this);
    }
}

CommandBuffer::CommandBuffer(CommandBuffer &&other) noexcept
    : CommandBuffer()
{
    Swap(other);
}

CommandBuffer &CommandBuffer::operator=(CommandBuffer &&other) noexcept
{
    Swap(other);
    return *this;
}

void CommandBuffer::Swap(CommandBuffer &other) noexcept
{
    RTRC_SWAP_MEMBERS(*this, other, device_, manager_, queueType_, rhiCommandBuffer_, pool_);
#if RTRC_DEBUG
    std::swap(threadID_, other.threadID_);
#endif
    RTRC_SWAP_MEMBERS(
        *this, other,
        currentGraphicsPipeline_, currentComputePipeline_, currentRayTracingPipeline_, currentWorkGraphPipeline_,
        isInRenderPass_, currentPassRenderTargetSize_, currentPassColorFormats_, currentPassDepthStencilFormat_,
        holdingObjects_);
}

RHI::QueueType CommandBuffer::GetQueueType() const
{
    return queueType_;
}

const RHI::CommandBufferRPtr &CommandBuffer::GetRHIObject() const
{
    CheckThreadID();
    return rhiCommandBuffer_;
}

Ref<Device> CommandBuffer::GetDevice() const
{
    return device_;
}

void CommandBuffer::Begin()
{
    manager_->_internalAllocate(*this);
    assert(rhiCommandBuffer_);
    rhiCommandBuffer_->Begin();
}

void CommandBuffer::End()
{
    CheckThreadID();
    rhiCommandBuffer_->End();

    assert(!isInRenderPass_);
    currentGraphicsPipeline_ = {};
    currentComputePipeline_ = {};
    currentRayTracingPipeline_ = {};
}

void CommandBuffer::BeginDebugEvent(std::string name, const std::optional<Vector4f> &color)
{
    CheckThreadID();
    const RHI::DebugLabel debugLabel{ std::move(name), color };
    rhiCommandBuffer_->BeginDebugEvent(debugLabel);
}

void CommandBuffer::EndDebugEvent()
{
    CheckThreadID();
    rhiCommandBuffer_->EndDebugEvent();
}

void CommandBuffer::CopyBuffer(const RC<Buffer> &dst, size_t dstOffset, const RC<Buffer> &src, size_t srcOffset, size_t size)
{
    CheckThreadID();
    AddHoldObject(dst);
    AddHoldObject(src);
    rhiCommandBuffer_->CopyBuffer(dst->GetRHIObject(), dstOffset, src->GetRHIObject(), srcOffset, size);
}

void CommandBuffer::CopyColorTexture(
    const RC<Texture> &dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
    const RC<Texture> &src, uint32_t srcMipLevel, uint32_t srcArrayLayer)
{
    CheckThreadID();
    AddHoldObject(dst);
    AddHoldObject(src);
    rhiCommandBuffer_->CopyColorTexture(
        dst->GetRHIObject(), dstMipLevel, dstArrayLayer, src->GetRHIObject(), srcMipLevel, srcArrayLayer);
}

void CommandBuffer::CopyColorTexture(
    const RC<Texture> &dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u &dstOffset,
    const RC<Texture> &src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u &srcOffset,
    const Vector3u &size)
{
    CheckThreadID();
    AddHoldObject(dst);
    AddHoldObject(src);
    assert(size.x >= 1 && size.y >= 1 && size.z >= 1);
    const auto minDim = (std::min)(std::to_underlying(src->GetDimension()), std::to_underlying(dst->GetDimension()));
    assert(minDim > std::to_underlying(RHI::TextureDimension::Tex1D) || size.y == 1);
    assert(minDim > std::to_underlying(RHI::TextureDimension::Tex2D) || size.z == 1);
    rhiCommandBuffer_->CopyColorTexture(
        dst->GetRHIObject(), dstMipLevel, dstArrayLayer, dstOffset,
        src->GetRHIObject(), srcMipLevel, srcArrayLayer, srcOffset, size);
}

void CommandBuffer::CopyColorTexture2DToBuffer(
    const RC<Buffer> &dst, size_t dstOffset, size_t dstRowBytes,
    const RC<Texture> &src, uint32_t arrayLayer, uint32_t mipLevel)
{
    CheckThreadID();
    AddHoldObject(dst);
    AddHoldObject(src);
    rhiCommandBuffer_->CopyTextureToBuffer(
        dst->GetRHIObject(), dstOffset, dstRowBytes,
        src->GetRHIObject(), mipLevel, arrayLayer);
}

void CommandBuffer::CopyBuffer(
    RGBuffer dst, size_t dstOffset, RGBuffer src, size_t srcOffset, size_t size)
{
    CopyBuffer(dst->Get(), dstOffset, src->Get(), srcOffset, size);
}

void CommandBuffer::CopyColorTexture(
    RGTexture dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
    RGTexture src, uint32_t srcMipLevel, uint32_t srcArrayLayer)
{
    CopyColorTexture(dst->Get(), dstMipLevel, dstArrayLayer, src->Get(), srcMipLevel, srcArrayLayer);
}

void CommandBuffer::CopyColorTexture(
    RGTexture dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u &dstOffset,
    RGTexture src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u &srcOffset,
    const Vector3u &size)
{
    CheckThreadID();
    CopyColorTexture(
        dst->Get(), dstMipLevel, dstArrayLayer, dstOffset,
        src->Get(), srcMipLevel, srcArrayLayer, srcOffset, size);
}

void CommandBuffer::CopyColorTexture2DToBuffer(
    RGBuffer dst, size_t dstOffset, size_t dstRowBytes,
    RGTexture src, uint32_t arrayLayer, uint32_t mipLevel)
{
    CopyColorTexture2DToBuffer(dst->Get(), dstOffset, dstRowBytes, src->Get(), arrayLayer, mipLevel);
}

void CommandBuffer::BeginRenderPass(Span<RenderTargetBinding> colorAttachments, bool setViewportAndScissor)
{
    BeginRenderPass(colorAttachments, {}, setViewportAndScissor);
}

void CommandBuffer::BeginRenderPass(const DepthStencilBinding &depthStencilAttachment, bool setViewportAndScissor)
{
    BeginRenderPass({}, depthStencilAttachment, setViewportAndScissor);
}

void CommandBuffer::BeginRenderPass(
    Span<RenderTargetBinding>  colorAttachments,
    const DepthStencilBinding &depthStencilAttachment,
    bool                    setViewportAndScissor)
{
    StaticVector<RHI::ColorAttachment, 16> rhiColorAttachments;
    for (auto& colorAttachment : colorAttachments)
    {
        rhiColorAttachments.push_back(RHI::ColorAttachment
        {
            .renderTargetView = colorAttachment.RTV.GetRHIObject(),
            .loadOp = colorAttachment.loadOp,
            .storeOp = colorAttachment.storeOp,
            .clearValue = colorAttachment.clearValue
        });
        AddHoldObject(colorAttachment.RTV);
    }

    const RHI::DepthStencilAttachment rhiDepthStencilAttachment =
    {
        .depthStencilView = depthStencilAttachment.DSV.GetRHIObject(),
        .loadOp = depthStencilAttachment.loadOp,
        .storeOp = depthStencilAttachment.storeOp,
        .clearValue = depthStencilAttachment.clearValue
    };
    AddHoldObject(depthStencilAttachment.DSV);

    CheckThreadID();
    rhiCommandBuffer_->BeginRenderPass(rhiColorAttachments, rhiDepthStencilAttachment);

    currentPassColorFormats_.Resize(colorAttachments.size());
    for(size_t i = 0; i < colorAttachments.size(); ++i)
    {
        auto rtv = rhiColorAttachments[i].renderTargetView;
        currentPassColorFormats_[i] = rtv->GetDesc().format;
    }
    if(!colorAttachments.IsEmpty())
    {
        currentPassRenderTargetSize_ = rhiColorAttachments[0].renderTargetView->GetSize();
    }
    else if(rhiDepthStencilAttachment.depthStencilView)
    {
        currentPassRenderTargetSize_ = rhiDepthStencilAttachment.depthStencilView->GetSize();
    }
    else
    {
        currentPassRenderTargetSize_ = {};
    }

    if(rhiDepthStencilAttachment.depthStencilView)
    {
        currentPassDepthStencilFormat_ = rhiDepthStencilAttachment.depthStencilView->GetDesc().format;
    }
    else
    {
        currentPassDepthStencilFormat_ = RHI::Format::Unknown;
    }

    assert(!isInRenderPass_);
    isInRenderPass_ = true;

    if(setViewportAndScissor)
    {
        SetViewportAndScissorAutomatically();
    }
}

void CommandBuffer::EndRenderPass()
{
    CheckThreadID();
    rhiCommandBuffer_->EndRenderPass();
    assert(isInRenderPass_);
    isInRenderPass_ = false;
}

void CommandBuffer::BindGraphicsPipeline(const RC<GraphicsPipeline> &graphicsPipeline)
{
    CheckThreadID();
    AddHoldObject(graphicsPipeline);
    rhiCommandBuffer_->BindPipeline(graphicsPipeline->GetRHIObject());
    currentGraphicsPipeline_ = graphicsPipeline;
    if(int index = graphicsPipeline->GetShaderInfo()->GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindGraphicsGroup(index, graphicsPipeline->GetShaderInfo()->GetBindingGroupForInlineSamplers());
    }
}

void CommandBuffer::BindComputePipeline(const RC<ComputePipeline> &computePipeline)
{
    CheckThreadID();
    AddHoldObject(computePipeline);
    rhiCommandBuffer_->BindPipeline(computePipeline->GetRHIObject());
    currentComputePipeline_ = computePipeline;
    if(int index = computePipeline->GetShaderInfo()->GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindComputeGroup(index, computePipeline->GetShaderInfo()->GetBindingGroupForInlineSamplers());
    }
}

void CommandBuffer::BindRayTracingPipeline(const RC<RayTracingPipeline> &rayTracingPipeline)
{
    auto &bindingLayoutInfo = rayTracingPipeline->GetBindingLayoutInfo();

    CheckThreadID();
    AddHoldObject(rayTracingPipeline);
    rhiCommandBuffer_->BindPipeline(rayTracingPipeline->GetRHIObject());
    currentRayTracingPipeline_ = rayTracingPipeline;
    if(int index = bindingLayoutInfo.GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindRayTracingGroup(index, bindingLayoutInfo.GetBindingGroupForInlineSamplers());
    }
}

void CommandBuffer::BindWorkGraphPipeline(
    const RC<WorkGraphPipeline> &workGraphPipeline,
    RHI::BufferDeviceAddress     backingBuffer,
    size_t                       backBufferSize,
    bool                         initializeBackingBuffer)
{
    auto &bindingLayoutInfo = workGraphPipeline->GetBindingLayoutInfo();

    CheckThreadID();
    AddHoldObject(workGraphPipeline);
    AddHoldObject(backingBuffer);

    rhiCommandBuffer_->BindPipeline(
        workGraphPipeline->GetRHIObject(), backingBuffer, backBufferSize, initializeBackingBuffer);
    currentWorkGraphPipeline_ = workGraphPipeline;
    if(int index = bindingLayoutInfo.GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindWorkGraphGroup(index, bindingLayoutInfo.GetBindingGroupForInlineSamplers());
    }
}

const GraphicsPipeline *CommandBuffer::GetCurrentGraphicsPipeline() const
{
    return currentGraphicsPipeline_.get();
}

const ComputePipeline *CommandBuffer::GetCurrentComputePipeline() const
{
    return currentComputePipeline_.get();
}

const RayTracingPipeline *CommandBuffer::GetCurrentRayTracingPipeline() const
{
    return currentRayTracingPipeline_.get();
}

const WorkGraphPipeline *CommandBuffer::GetCurrentWorkGraphPipeline() const
{
    return currentWorkGraphPipeline_.get();
}

void CommandBuffer::BindGraphicsGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    AddHoldObject(group);
#if RTRC_DEBUG
    auto bindingGroupLayoutInShader = currentGraphicsPipeline_->GetShaderInfo()->GetBindingGroupLayoutByIndex(index);
    if(bindingGroupLayoutInShader != group->GetLayout())
    {
        LogError("Binding group defined in shader is");
        DumpBindingGroupLayoutDesc(bindingGroupLayoutInShader->GetRHIObject()->GetDesc());
        LogError("Binding group being bound is");
        DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
        throw Exception("Unmatched binding group layout for graphics pipeline!");
    }
#endif
    rhiCommandBuffer_->BindGroupToGraphicsPipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindComputeGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    AddHoldObject(group);
#if RTRC_DEBUG
    auto bindingGroupLayoutInShader = currentComputePipeline_->GetShaderInfo()->GetBindingGroupLayoutByIndex(index);
    if(bindingGroupLayoutInShader != group->GetLayout())
    {
        LogError("Binding group defined in shader is");
        DumpBindingGroupLayoutDesc(bindingGroupLayoutInShader->GetRHIObject()->GetDesc());
        LogError("Binding group being bound is");
        DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
        throw Exception("Unmatched binding group layout for compute pipeline!");
    }
#endif
    rhiCommandBuffer_->BindGroupToComputePipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindRayTracingGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    AddHoldObject(group);
#if RTRC_DEBUG
    auto bindingGroupLayoutInShader = currentRayTracingPipeline_->GetBindingLayoutInfo().GetBindingGroupLayoutByIndex(index);
    if(bindingGroupLayoutInShader != group->GetLayout())
    {
        LogError("Binding group defined in shader is");
        DumpBindingGroupLayoutDesc(bindingGroupLayoutInShader->GetRHIObject()->GetDesc());
        LogError("Binding group being bound is");
        DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
        throw Exception("Unmatched binding group layout for ray tracing pipeline!");
    }
#endif
    rhiCommandBuffer_->BindGroupToRayTracingPipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindWorkGraphGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    AddHoldObject(group);
#if RTRC_DEBUG
    auto bindingGroupLayoutInShader = currentWorkGraphPipeline_->GetBindingLayoutInfo().GetBindingGroupLayoutByIndex(index);
    if(bindingGroupLayoutInShader != group->GetLayout())
    {
        LogError("Binding group defined in shader is");
        DumpBindingGroupLayoutDesc(bindingGroupLayoutInShader->GetRHIObject()->GetDesc());
        LogError("Binding group being bound is");
        DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
        throw Exception("Unmatched binding group layout for ray tracing pipeline!");
    }
#endif
    rhiCommandBuffer_->BindGroupToWorkGraphPipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindGraphicsGroups(Span<RC<BindingGroup>> groups)
{
    CheckThreadID();
    for(auto &group : groups)
    {
        AddHoldObject(group);
    }
    auto &shaderInfo = currentGraphicsPipeline_->GetShaderInfo();
    for(auto &&[groupIndex, group] : Enumerate(groups))
    {
#if RTRC_DEBUG
        int foundIndex = -1;
#endif
        for(int i = 0; i < shaderInfo->GetBindingGroupCount(); ++i)
        {
            if(shaderInfo->GetBindingGroupLayoutByIndex(i) == group->GetLayout())
            {
                BindGraphicsGroup(i, group);
#if RTRC_DEBUG
                if(foundIndex >= 0)
                {
                    throw Exception(std::format(
                        "CommandBuffer::BindGraphicsGroups: group {} and {} have the same group layout",
                        foundIndex, i));
                }
                foundIndex = i;
#else
                break;
#endif
            }
        }
#if RTRC_DEBUG
        if(foundIndex < 0)
        {
            LogError("Layout of given binding group is:");
            DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
            LogError("Binding group layouts defined in shader are:");
            for(int i = 0; i < shaderInfo->GetBindingGroupCount(); ++i)
            {
                DumpBindingGroupLayoutDesc(shaderInfo->GetBindingGroupLayoutByIndex(i)->GetRHIObject()->GetDesc());
            }
            throw Exception(std::format("CommandBuffer::BindGraphicsGroups: group {} is unbounded", groupIndex));
        }
#endif
    }
}

void CommandBuffer::BindComputeGroups(Span<RC<BindingGroup>> groups)
{
    CheckThreadID();
    for(auto &group : groups)
    {
        AddHoldObject(group);
    }
    auto &shaderInfo = currentComputePipeline_->GetShaderInfo();
    for(auto &&[groupIndex, group] : Enumerate(groups))
    {
#if RTRC_DEBUG
        int foundIndex = -1;
#endif
        for(int i = 0; i < shaderInfo->GetBindingGroupCount(); ++i)
        {
            if(shaderInfo->GetBindingGroupLayoutByIndex(i) == group->GetLayout())
            {
                BindComputeGroup(i, group);
#if RTRC_DEBUG
                if(foundIndex >= 0)
                {
                    throw Exception(std::format(
                        "CommandBuffer::BindComputeGroups: group {} and {} have the same group layout",
                        foundIndex, i));
                }
                foundIndex = i;
#else
                break;
#endif
            }
        }
#if RTRC_DEBUG
        if(foundIndex < 0)
        {
            LogError("Layout of given binding group is:");
            DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
            LogError("Binding group layouts defined in shader are:");
            for(int i = 0; i < shaderInfo->GetBindingGroupCount(); ++i)
            {
                DumpBindingGroupLayoutDesc(shaderInfo->GetBindingGroupLayoutByIndex(i)->GetRHIObject()->GetDesc());
            }
            throw Exception(std::format("CommandBuffer::BindComputeGroups: group {} is unbounded", groupIndex));
        }
#endif
    }
}

void CommandBuffer::BindRayTracingGroups(Span<RC<BindingGroup>> groups)
{
    CheckThreadID();
    for(auto &group : groups)
    {
        AddHoldObject(group);
    }
    auto &layoutInfo = currentRayTracingPipeline_->GetBindingLayoutInfo();
    for(auto &&[groupIndex, group] : Enumerate(groups))
    {
#if RTRC_DEBUG
        int foundIndex = -1;
#endif
        for(int i = 0; i < layoutInfo.GetBindingGroupCount(); ++i)
        {
            if(layoutInfo.GetBindingGroupLayoutByIndex(i) == group->GetLayout())
            {
                BindRayTracingGroup(i, group);
#if RTRC_DEBUG
                if(foundIndex >= 0)
                {
                    throw Exception(std::format(
                        "CommandBuffer::BindRayTracingGroups: group {} and {} have the same group layout",
                        foundIndex, i));
                }
                foundIndex = i;
#else
                break;
#endif
            }
        }
#if RTRC_DEBUG
        if(foundIndex < 0)
        {
            LogError("Layout of given binding group is:");
            DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
            LogError("Binding group layouts defined in shader are:");
            for(int i = 0; i < layoutInfo.GetBindingGroupCount(); ++i)
            {
                DumpBindingGroupLayoutDesc(layoutInfo.GetBindingGroupLayoutByIndex(i)->GetRHIObject()->GetDesc());
            }
            throw Exception(std::format("CommandBuffer::BindRayTracingGroups: group {} is unbounded", groupIndex));
        }
#endif
    }
}

void CommandBuffer::BindWorkGraphGroups(Span<RC<BindingGroup>> groups)
{
    CheckThreadID();
    for(auto &group : groups)
    {
        AddHoldObject(group);
    }
    auto &layoutInfo = currentWorkGraphPipeline_->GetBindingLayoutInfo();
    for(auto &&[groupIndex, group] : Enumerate(groups))
    {
#if RTRC_DEBUG
        int foundIndex = -1;
#endif
        for(int i = 0; i < layoutInfo.GetBindingGroupCount(); ++i)
        {
            if(layoutInfo.GetBindingGroupLayoutByIndex(i) == group->GetLayout())
            {
                BindWorkGraphGroup(i, group);
#if RTRC_DEBUG
                if(foundIndex >= 0)
                {
                    throw Exception(std::format(
                        "CommandBuffer::BindWorkGraphGroups: group {} and {} have the same group layout",
                        foundIndex, i));
                }
                foundIndex = i;
#else
                break;
#endif
            }
        }
#if RTRC_DEBUG
        if(foundIndex < 0)
        {
            LogError("Layout of given binding group is:");
            DumpBindingGroupLayoutDesc(group->GetLayout()->GetRHIObject()->GetDesc());
            LogError("Binding group layouts defined in shader are:");
            for(int i = 0; i < layoutInfo.GetBindingGroupCount(); ++i)
            {
                DumpBindingGroupLayoutDesc(layoutInfo.GetBindingGroupLayoutByIndex(i)->GetRHIObject()->GetDesc());
            }
            throw Exception(std::format("CommandBuffer::BindWorkGraphGroups: group {} is unbounded", groupIndex));
        }
#endif
    }
}

void CommandBuffer::SetViewports(Span<RHI::Viewport> viewports)
{
    CheckThreadID();
    rhiCommandBuffer_->SetViewports(viewports);
}

void CommandBuffer::SetScissors(Span<RHI::Scissor> scissors)
{
    CheckThreadID();
    rhiCommandBuffer_->SetScissors(scissors);
}

void CommandBuffer::SetViewportAndScissorAutomatically()
{
    CheckThreadID();
    assert(isInRenderPass_);
    SetViewports(RHI::Viewport
                 {
                     .topLeftCorner = {},
                     .size = currentPassRenderTargetSize_.To<float>(),
                     .minDepth = 0,
                     .maxDepth = 1,
                 });
    SetScissors(RHI::Scissor
                {
                    .topLeftCorner = {},
                    .size = currentPassRenderTargetSize_.To<int>(),
                });
}

void CommandBuffer::SetVertexBuffer(int slot, const RC<SubBuffer> &buffer, size_t stride)
{
    CheckThreadID();
    AddHoldObject(buffer);
    SetVertexBuffers(slot, Span(buffer), Span(stride));
}

void CommandBuffer::SetVertexBuffers(int slot, Span<RC<SubBuffer>> buffers, Span<size_t> byteStrides)
{
    CheckThreadID();
    for(auto &buffer : buffers)
    {
        AddHoldObject(buffer);
    }
    std::vector<RHI::BufferRPtr> rhiBuffers(buffers.size());
    std::vector<size_t> rhiByteOffsets(buffers.size());
    for(size_t i = 0; i < buffers.size(); ++i)
    {
        rhiBuffers[i] = buffers[i]->GetFullBufferRHIObject();
        rhiByteOffsets[i] = buffers[i]->GetSubBufferOffset();
    }
    rhiCommandBuffer_->SetVertexBuffer(slot, rhiBuffers, rhiByteOffsets, byteStrides);
}

void CommandBuffer::SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format)
{
    CheckThreadID();
    AddHoldObject(buffer);
    rhiCommandBuffer_->SetIndexBuffer(buffer->GetFullBufferRHIObject(), buffer->GetSubBufferOffset(), format);
}

void CommandBuffer::SetStencilReferenceValue(uint8_t value)
{
    CheckThreadID();
    rhiCommandBuffer_->SetStencilReferenceValue(value);
}

void CommandBuffer::SetGraphicsPushConstants(uint32_t rangeIndex, uint32_t offset, uint32_t size, const void *data)
{
    CheckThreadID();
    rhiCommandBuffer_->SetGraphicsPushConstants(rangeIndex, offset, size, data);
}

void CommandBuffer::SetComputePushConstants(uint32_t rangeIndex, uint32_t offset, uint32_t size, const void *data)
{
    CheckThreadID();
    rhiCommandBuffer_->SetComputePushConstants(rangeIndex, offset, size, data);
}

void CommandBuffer::SetGraphicsPushConstants(uint32_t rangeIndex, const void *data)
{
    assert(
        currentGraphicsPipeline_ &&
        rangeIndex < currentGraphicsPipeline_->GetShaderInfo()->GetPushConstantRanges().GetSize());
    const auto &range = currentGraphicsPipeline_->GetShaderInfo()->GetPushConstantRanges()[rangeIndex];
    SetGraphicsPushConstants(rangeIndex, 0, range.size, data);
}

void CommandBuffer::SetComputePushConstants(uint32_t rangeIndex, const void *data)
{
    assert(
        currentComputePipeline_ &&
        rangeIndex < currentComputePipeline_->GetShaderInfo()->GetPushConstantRanges().GetSize());
    const auto &range = currentComputePipeline_->GetShaderInfo()->GetPushConstantRanges()[rangeIndex];
    SetComputePushConstants(rangeIndex, 0, range.size, data);
}

void CommandBuffer::ClearColorTexture2D(const RC<Texture> &tex, const Vector4f &color)
{
    CheckThreadID();
    AddHoldObject(tex);
    rhiCommandBuffer_->ClearColorTexture2D(
        tex->GetRHIObject().Get(), RHI::ColorClearValue{ color.x, color.y, color.z, color.w });
}

void CommandBuffer::ClearDepthStencil(const RC<Texture>& tex, float depth, uint8_t stencil)
{
    CheckThreadID();
    AddHoldObject(tex);
    rhiCommandBuffer_->ClearDepthStencilTexture(
        tex->GetRHIObject().Get(), RHI::DepthStencilClearValue{ depth, stencil }, true, true);
}

void CommandBuffer::ClearDepth(const RC<Texture>& tex, float depth)
{
    CheckThreadID();
    AddHoldObject(tex);
    rhiCommandBuffer_->ClearDepthStencilTexture(
        tex->GetRHIObject().Get(), RHI::DepthStencilClearValue{ depth, 0 }, true, false);
}

void CommandBuffer::ClearStencil(const RC<Texture>& tex, uint8_t stencil)
{
    CheckThreadID();
    AddHoldObject(tex);
    rhiCommandBuffer_->ClearDepthStencilTexture(
        tex->GetRHIObject().Get(), RHI::DepthStencilClearValue{ 0, stencil }, false, true);
}

void CommandBuffer::Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
{
    CheckThreadID();
    rhiCommandBuffer_->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance)
{
    CheckThreadID();
    rhiCommandBuffer_->DrawIndexed(indexCount, instanceCount, firstIndex, firstVertex, firstInstance);
}

void CommandBuffer::DispatchMesh(unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ)
{
    CheckThreadID();
    rhiCommandBuffer_->DispatchMesh(static_cast<int>(groupCountX), static_cast<int>(groupCountY), static_cast<int>(groupCountZ));
}

void CommandBuffer::Dispatch(unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ)
{
    CheckThreadID();
    rhiCommandBuffer_->Dispatch(static_cast<int>(groupCountX), static_cast<int>(groupCountY), static_cast<int>(groupCountZ));
}

void CommandBuffer::Dispatch(const Vector3u &groupCount)
{
    this->Dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void CommandBuffer::DispatchWithThreadCount(unsigned countX, unsigned countY, unsigned countZ)
{
    const Vector3u groupCount = currentComputePipeline_->GetShaderInfo()
        ->ComputeThreadGroupCount({ countX, countY, countZ });
    Dispatch(groupCount);
}

void CommandBuffer::DispatchWithThreadCount(unsigned countX, unsigned countY)
{
    DispatchWithThreadCount(countX, countY, 1u);
}

void CommandBuffer::DispatchWithThreadCount(unsigned countX)
{
    DispatchWithThreadCount(countX, 1u, 1u);
}

void CommandBuffer::DispatchWithThreadCount(const Vector3i &count)
{
    DispatchWithThreadCount(count.x, count.y, count.z);
}

void CommandBuffer::DispatchWithThreadCount(const Vector3u &count)
{
    DispatchWithThreadCount(count.x, count.y, count.z);
}

void CommandBuffer::DispatchWithThreadCount(const Vector2i &count)
{
    DispatchWithThreadCount(count.x, count.y, 1);
}

void CommandBuffer::DispatchWithThreadCount(const Vector2u &count)
{
    DispatchWithThreadCount(count.x, count.y, 1u);
}

void CommandBuffer::Trace(
    int                                  rayCountX,
    int                                  rayCountY,
    int                                  rayCountZ,
    const RHI::ShaderBindingTableRegion &raygenSbt,
    const RHI::ShaderBindingTableRegion &missSbt,
    const RHI::ShaderBindingTableRegion &hitSbt,
    const RHI::ShaderBindingTableRegion &callableSbt)
{
    CheckThreadID();
    rhiCommandBuffer_->TraceRays(rayCountX, rayCountY, rayCountZ, raygenSbt, missSbt, hitSbt, callableSbt);
}

void CommandBuffer::Trace(
    unsigned int                         rayCountX,
    unsigned int                         rayCountY,
    unsigned int                         rayCountZ,
    const RHI::ShaderBindingTableRegion &raygenSbt,
    const RHI::ShaderBindingTableRegion &missSbt,
    const RHI::ShaderBindingTableRegion &hitSbt,
    const RHI::ShaderBindingTableRegion &callableSbt)
{
    this->Trace(
        static_cast<int>(rayCountX),
        static_cast<int>(rayCountY),
        static_cast<int>(rayCountZ),
        raygenSbt, missSbt, hitSbt, callableSbt);
}

void CommandBuffer::DispatchIndirect(const RC<SubBuffer> &buffer, size_t byteOffset)
{
    CheckThreadID();
    AddHoldObject(buffer);
    byteOffset += buffer->GetSubBufferOffset();
    auto &rhiBuffer = buffer->GetFullBufferRHIObject();
    rhiCommandBuffer_->DispatchIndirect(rhiBuffer, byteOffset);
}

void CommandBuffer::DispatchNode(uint32_t entryIndex, uint32_t recordCount, uint32_t recordStride, const void *records)
{
    CheckThreadID();
    rhiCommandBuffer_->DispatchNode(entryIndex, recordCount, recordStride, records);
}

void CommandBuffer::DrawIndexedIndirect(
    const RC<SubBuffer> &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride)
{
    CheckThreadID();
    byteOffset += buffer->GetSubBufferOffset();
    auto &rhiBuffer = buffer->GetFullBufferRHIObject();
    rhiCommandBuffer_->DrawIndexedIndirect(rhiBuffer, drawCount, byteOffset, byteStride);
}

void CommandBuffer::BuildBlas(
    const RC<Blas>                                &blas,
    Span<RHI::RayTracingGeometryDesc>              geometries,
    RHI::RayTracingAccelerationStructureBuildFlags flags,
    const RC<SubBuffer>                           &scratchBuffer)
{
    auto prebuildInfo = device_->CreateBlasPrebuildinfo(geometries, flags);
    BuildBlas(blas, geometries, *prebuildInfo, scratchBuffer);
}

void CommandBuffer::BuildBlas(
    const RC<Blas>                   &blas,
    Span<RHI::RayTracingGeometryDesc> geometries,
    const BlasPrebuildInfo           &prebuildInfo,
    const RC<SubBuffer>              &scratchBuffer)
{
    CheckThreadID();
    AddHoldObject(blas);
    AddHoldObject(scratchBuffer);

    if(!blas->GetBuffer())
    {
        auto newBuffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = prebuildInfo.GetAccelerationStructureBufferSize(),
            .usage          = RHI::BufferUsage::AccelerationStructure,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        newBuffer->SetName("BlasBuffer");
        blas->SetBuffer(std::move(newBuffer));
    }

    const uint64_t scratchAlignement = device_->GetAccelerationStructureScratchBufferAlignment();

    SubBuffer *actualScratchBuffer = scratchBuffer.get();
    RC<SubBuffer> temporaryScratchBuffer;
    if(!actualScratchBuffer)
    {
        temporaryScratchBuffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = prebuildInfo.GetBuildScratchBufferSize() + scratchAlignement,
            .usage          = RHI::BufferUsage::AccelerationStructureScratch,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        temporaryScratchBuffer->GetFullBuffer()->SetName("BuildBlasScratchBuffer");
        actualScratchBuffer = temporaryScratchBuffer.get();
    }

    auto scratchDeviceAddress = actualScratchBuffer->GetDeviceAddress();
    scratchDeviceAddress.address = UpAlignTo(scratchDeviceAddress.address, scratchAlignement);

    rhiCommandBuffer_->BuildBlas(
        prebuildInfo.GetRHIObject(), geometries, blas->GetRHIObject(), scratchDeviceAddress);
}

void CommandBuffer::BuildTlas(
    const RC<Tlas>                                &tlas,
    const RHI::RayTracingInstanceArrayDesc        &instances,
    RHI::RayTracingAccelerationStructureBuildFlags flags,
    const RC<SubBuffer>                           &scratchBuffer)
{
    auto prebuildInfo = device_->CreateTlasPrebuildInfo(instances, flags);
    return BuildTlas(tlas, instances, *prebuildInfo, scratchBuffer);
}

void CommandBuffer::BuildTlas(
    const RC<Tlas>                         &tlas,
    const RHI::RayTracingInstanceArrayDesc &instances,
    const TlasPrebuildInfo                 &prebuildInfo,
    const RC<SubBuffer>                    &scratchBuffer)
{
    CheckThreadID();
    AddHoldObject(tlas);
    AddHoldObject(scratchBuffer);

    if(!tlas->GetBuffer())
    {
        auto newBuffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = prebuildInfo.GetAccelerationStructureBufferSize(),
            .usage          = RHI::BufferUsage::AccelerationStructure,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        newBuffer->SetName("TlasBuffer");
        tlas->SetBuffer(std::move(newBuffer));
    }

    const uint64_t scratchAlignement = device_->GetAccelerationStructureScratchBufferAlignment();

    SubBuffer *actualScratchBuffer = scratchBuffer.get();
    RC<SubBuffer> temporaryScratchBuffer;
    if(!actualScratchBuffer)
    {
        temporaryScratchBuffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = prebuildInfo.GetBuildScratchBufferSize() + scratchAlignement,
            .usage          = RHI::BufferUsage::AccelerationStructureScratch,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        temporaryScratchBuffer->GetFullBuffer()->SetName("BuildTlasScratchBuffer");
        actualScratchBuffer = temporaryScratchBuffer.get();
    }

    auto scratchDeviceAddress = actualScratchBuffer->GetDeviceAddress();
    scratchDeviceAddress.address = UpAlignTo(scratchDeviceAddress.address, scratchAlignement);

    rhiCommandBuffer_->BuildTlas(
        prebuildInfo.GetRHIObject(), instances, tlas->GetRHIObject(), scratchDeviceAddress);
}

void CommandBuffer::ExecuteBarriers(const BarrierBatch &barriers)
{
    CheckThreadID();
    if(barriers.G_ || !barriers.BT_.empty() || !barriers.TT_.empty() || !barriers.TA_.empty() || !barriers.TT_.empty())
    {
        if(barriers.G_)
        {
            rhiCommandBuffer_->ExecuteBarriers(*barriers.G_, barriers.BT_, barriers.TT_, barriers.TA_, barriers.TT_);
        }
        else
        {
            rhiCommandBuffer_->ExecuteBarriers(barriers.BT_, barriers.TT_, barriers.TA_, barriers.TT_);
        }
    }
}

void CommandBuffer::CheckThreadID() const
{
#if RTRC_DEBUG
    assert(threadID_ == std::thread::id() || threadID_ == std::this_thread::get_id());
#endif
}

//SingleThreadCommandBufferManager::SingleThreadCommandBufferManager(Device* device, RHI::QueueRPtr queue)
//    : device_(device), queue_(std::move(queue))
//{
//    
//}

DeviceCommandBufferManager::DeviceCommandBufferManager(Device *device, DeviceSynchronizer &sync)
    : device_(device), sync_(sync)
{
    
}

DeviceCommandBufferManager::~DeviceCommandBufferManager()
{
    for(PerThreadPoolData &poolData : std::views::values(threadToActivePoolData_))
    {
        if(poolData.activePool)
        {
            assert(poolData.activePool->activeUserCount == 0 && "Not all command buffers are destroyed");
            delete poolData.activePool;
        }
    }
}

CommandBuffer DeviceCommandBufferManager::Create()
{
    CommandBuffer ret;
    ret.device_ = device_;
    ret.manager_ = this;
    ret.queueType_ = sync_.GetQueue()->GetType();
    return ret;
}

void DeviceCommandBufferManager::_internalEndFrame()
{
    std::unique_lock lock(threadToActivePoolDataMutex_);
    for(PerThreadPoolData &poolData : std::views::values(threadToActivePoolData_))
    {
        if(poolData.activePool)
        {
            assert(poolData.activePool->activeUserCount == 0 && "Cross frame command buffer is not allowed");
            sync_.OnFrameComplete([p = std::move(poolData.activePool->rhiPool), &fps = freePools_]() mutable
            {
                p->Reset();
                fps.Push(std::move(p));
            });
            delete poolData.activePool;
        }
    }
    threadToActivePoolData_.clear();
}

void DeviceCommandBufferManager::_internalAllocate(CommandBuffer &commandBuffer)
{
    assert(!commandBuffer.rhiCommandBuffer_);
#if RTRC_DEBUG
    assert(commandBuffer.threadID_ == std::thread::id());
#endif

    PerThreadPoolData &perThreadData = GetPerThreadPoolData();
    if(!perThreadData.activePool)
    {
        perThreadData.activePool = new CommandBuffer::Pool;
        perThreadData.activePool->rhiPool = GetFreeCommandPool();
    }

    commandBuffer.rhiCommandBuffer_ = perThreadData.activePool->rhiPool->NewCommandBuffer();
    commandBuffer.pool_ = perThreadData.activePool;
#if RTRC_DEBUG
    commandBuffer.threadID_ = std::this_thread::get_id();
#endif

    ++perThreadData.activePool->activeUserCount;
    if(++perThreadData.activePool->historyUserCount >= MAX_COMMAND_BUFFERS_IN_SINGLE_POOL)
    {
        perThreadData.activePool = nullptr;
    }
}

void DeviceCommandBufferManager::_internalRelease(CommandBuffer &commandBuffer)
{
    if(!commandBuffer.rhiCommandBuffer_)
    {
        return;
    }

    assert(commandBuffer.pool_);
    if(!--commandBuffer.pool_->activeUserCount)
    {
        if(commandBuffer.pool_->historyUserCount >= MAX_COMMAND_BUFFERS_IN_SINGLE_POOL)
        {
            sync_.OnFrameComplete([p = std::move(commandBuffer.pool_->rhiPool), &fps = freePools_]() mutable
            {
                p->Reset();
                fps.Push(std::move(p));
            });
            delete commandBuffer.pool_;
        }
    }
}

DeviceCommandBufferManager::PerThreadPoolData &DeviceCommandBufferManager::GetPerThreadPoolData()
{
    const std::thread::id id = std::this_thread::get_id();
    {
        std::shared_lock lock(threadToActivePoolDataMutex_);
        if(auto it = threadToActivePoolData_.find(id); it != threadToActivePoolData_.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(threadToActivePoolDataMutex_);
    return threadToActivePoolData_[id];
}

RHI::CommandPoolUPtr DeviceCommandBufferManager::GetFreeCommandPool()
{
    RHI::CommandPoolUPtr ret;
    if(!freePools_.TryPop(ret))
    {
        ret = device_->GetRawDevice()->CreateCommandPool(sync_.GetQueue());
    }
    return ret;
}

RTRC_END
