#include <ranges>
#include <shared_mutex>

#include <Rtrc/Graphics/Mesh/Mesh.h>
#include <Rtrc/Graphics/Device/Buffer/Buffer.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/Device/CommandBuffer.h>

RTRC_BEGIN

BarrierBatch &BarrierBatch::operator()(
    const RC<StatefulBuffer> &buffer,
    RHI::PipelineStageFlag    stages,
    RHI::ResourceAccessFlag   accesses)
{
    BT_.push_back(RHI::BufferTransitionBarrier
    {
        .buffer         = buffer->GetRHIObject().Get(),
        .beforeStages   = buffer->GetState().stages,
        .beforeAccesses = buffer->GetState().accesses,
        .afterStages    = stages,
        .afterAccesses  = accesses
    });
    buffer->SetState({ stages, accesses });
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    for(auto [m, a] : EnumerateSubTextures(texture->GetDesc()))
        operator()(texture, a, m, layout, stages, accesses);
    return *this;
}

BarrierBatch &BarrierBatch::operator()(
    const RC<StatefulTexture> &texture,
    uint32_t                   arrayLayer,
    uint32_t                   mipLevel,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    TT_.push_back(RHI::TextureTransitionBarrier
    {
        .texture        = texture->GetRHIObject().Get(),
        .subresources   = RHI::TextureSubresources{ mipLevel, 1, arrayLayer, 1 },
        .beforeStages   = texture->GetState(mipLevel, arrayLayer).stages,
        .beforeAccesses = texture->GetState(mipLevel, arrayLayer).accesses,
        .beforeLayout   = texture->GetState(mipLevel, arrayLayer).layout,
        .afterStages    = stages,
        .afterAccesses  = accesses,
        .afterLayout    = layout
    });
    texture->SetState(mipLevel, arrayLayer, { layout, stages, accesses });
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
    TT_.push_back(RHI::TextureTransitionBarrier
    {
        .texture        = texture->GetRHIObject().Get(),
        .subresources   = RHI::TextureSubresources{ mipLevel, 1, arrayLayer, 1 },
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
    return operator()(
        texture,
        prevLayout, RHI::PipelineStage::All, RHI::ResourceAccess::All,
        succLayout, RHI::PipelineStage::All, RHI::ResourceAccess::All);
}

BarrierBatch &BarrierBatch::operator()(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         prevLayout,
    RHI::TextureLayout         succLayout)
{
    throw Exception("Invalid barrier operation on stateful texture");
}

CommandBuffer::CommandBuffer()
    : manager_(nullptr), queueType_(RHI::QueueType::Graphics), pool_(nullptr)
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
    RTRC_SWAP_MEMBERS(*this, other, manager_, queueType_, rhiCommandBuffer_, pool_);
#if RTRC_DEBUG
    std::swap(threadID_, other.threadID_);
#endif
}

RHI::QueueType CommandBuffer::GetQueueType() const
{
    return queueType_;
}

const RHI::CommandBufferPtr &CommandBuffer::GetRHIObject() const
{
    CheckThreadID();
    return rhiCommandBuffer_;
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

    currentGraphicsPipeline_ = {};
    currentComputePipeline_ = {};
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

void CommandBuffer::CopyBuffer(Buffer &dst, size_t dstOffset, const Buffer &src, size_t srcOffset, size_t size)
{
    CheckThreadID();
    rhiCommandBuffer_->CopyBuffer(dst.GetRHIObject(), dstOffset, src.GetRHIObject(), srcOffset, size);
}

void CommandBuffer::CopyColorTexture2DToBuffer(
    Buffer &dst, size_t dstOffset, Texture &src, uint32_t arrayLayer, uint32_t mipLevel)
{
    CheckThreadID();
    rhiCommandBuffer_->CopyColorTexture2DToBuffer(
        dst.GetRHIObject(), dstOffset, src.GetRHIObject(), mipLevel, arrayLayer);
}

void CommandBuffer::BeginRenderPass(Span<ColorAttachment> colorAttachments)
{
    BeginRenderPass(colorAttachments, {});
}

void CommandBuffer::BeginRenderPass(const DepthStencilAttachment &depthStencilAttachment)
{
    BeginRenderPass({}, depthStencilAttachment);
}

void CommandBuffer::BeginRenderPass(
    Span<ColorAttachment> colorAttachments, const DepthStencilAttachment &depthStencilAttachment)
{
    CheckThreadID();
    rhiCommandBuffer_->BeginRenderPass(colorAttachments, depthStencilAttachment);
}

void CommandBuffer::EndRenderPass()
{
    CheckThreadID();
    rhiCommandBuffer_->EndRenderPass();
}

void CommandBuffer::BindGraphicsPipeline(const RC<GraphicsPipeline> &graphicsPipeline)
{
    CheckThreadID();
    rhiCommandBuffer_->BindPipeline(graphicsPipeline->GetRHIObject());
    if(int index = graphicsPipeline->GetShaderInfo()->GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindGraphicsGroup(index, graphicsPipeline->GetShaderInfo()->GetBindingGroupForInlineSamplers());
    }
    currentGraphicsPipeline_ = graphicsPipeline;
}

void CommandBuffer::BindComputePipeline(const RC<ComputePipeline> &computePipeline)
{
    CheckThreadID();
    rhiCommandBuffer_->BindPipeline(computePipeline->GetRHIObject());
    if(int index = computePipeline->GetShaderInfo()->GetBindingGroupIndexForInlineSamplers(); index >= 0)
    {
        BindComputeGroup(index, computePipeline->GetShaderInfo()->GetBindingGroupForInlineSamplers());
    }
    currentComputePipeline_ = computePipeline;
}

const GraphicsPipeline *CommandBuffer::GetCurrentGraphicsPipeline() const
{
    return currentGraphicsPipeline_.get();
}

const ComputePipeline *CommandBuffer::GetCurrentComputePipeline() const
{
    return currentComputePipeline_.get();
}

void CommandBuffer::BindGraphicsGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    rhiCommandBuffer_->BindGroupToGraphicsPipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindComputeGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    rhiCommandBuffer_->BindGroupToComputePipeline(index, group->GetRHIObject());
}

void CommandBuffer::SetViewports(Span<Viewport> viewports)
{
    CheckThreadID();
    rhiCommandBuffer_->SetViewports(viewports);
}

void CommandBuffer::SetScissors(Span<Scissor> scissors)
{
    CheckThreadID();
    rhiCommandBuffer_->SetScissors(scissors);
}

void CommandBuffer::SetVertexBuffers(int slot, Span<RC<Buffer>> buffers, Span<size_t> byteOffsets)
{
    CheckThreadID();
    static constexpr std::array<size_t, 128> EMPTY_BYTE_OFFSETS = {};
    std::vector<RHI::BufferPtr> rhiBuffers(buffers.size());
    for(size_t i = 0; i < buffers.size(); ++i)
    {
        rhiBuffers[i] = buffers[i]->GetRHIObject();
    }
    rhiCommandBuffer_->SetVertexBuffer(
        slot, rhiBuffers, byteOffsets.IsEmpty() ? Span(EMPTY_BYTE_OFFSETS.data(), buffers.GetSize()) : byteOffsets);
}

void CommandBuffer::SetIndexBuffer(const RC<Buffer> &buffer, RHI::IndexBufferFormat format, size_t byteOffset)
{
    CheckThreadID();
    rhiCommandBuffer_->SetIndexBuffer(buffer->GetRHIObject(), byteOffset, format);
}

void CommandBuffer::BindMesh(const Mesh &mesh)
{
    CheckThreadID();
    mesh.Bind(*this);
}

void CommandBuffer::SetStencilReferenceValue(uint8_t value)
{
    CheckThreadID();
    rhiCommandBuffer_->SetStencilReferenceValue(value);
}

void CommandBuffer::SetGraphicsPushConstantRange(
    RHI::ShaderStageFlag stages, uint32_t offset, uint32_t size, const void *data)
{
    CheckThreadID();
    assert(currentGraphicsPipeline_);
    rhiCommandBuffer_->SetPushConstants(
        currentGraphicsPipeline_->GetRHIObject()->GetBindingLayout(), stages, offset, size, data);
}

void CommandBuffer::SetComputePushConstantRange(
    RHI::ShaderStageFlag stages, uint32_t offset, uint32_t size, const void *data)
{
    CheckThreadID();
    assert(currentComputePipeline_);
    rhiCommandBuffer_->SetPushConstants(
        currentComputePipeline_->GetRHIObject()->GetBindingLayout(), stages, offset, size, data);
}

void CommandBuffer::SetGraphicsPushConstantRange(uint32_t rangeIndex, const void *data)
{
    assert(
        currentGraphicsPipeline_ &&
        rangeIndex < currentGraphicsPipeline_->GetShaderInfo()->GetPushConstantRanges().GetSize());
    const RHI::PushConstantRange &range = currentGraphicsPipeline_->GetShaderInfo()->GetPushConstantRanges()[rangeIndex];
    SetGraphicsPushConstantRange(range.stages, range.offset, range.size, data);
}

void CommandBuffer::SetComputePushConstantRange(uint32_t rangeIndex, const void *data)
{
    assert(
        currentComputePipeline_ &&
        rangeIndex < currentComputePipeline_->GetShaderInfo()->GetPushConstantRanges().GetSize());
    const RHI::PushConstantRange &range = currentComputePipeline_->GetShaderInfo()->GetPushConstantRanges()[rangeIndex];
    SetComputePushConstantRange(range.stages, range.offset, range.size, data);
}

void CommandBuffer::SetGraphicsPushConstants(const void *data, uint32_t size)
{
    const GraphicsPipeline *pipeline = GetCurrentGraphicsPipeline();
    assert(pipeline);
    const Span<Shader::PushConstantRange> pushConstantRanges = pipeline->GetShaderInfo()->GetPushConstantRanges();
    for(const Shader::PushConstantRange &range : pushConstantRanges)
    {
        const uint32_t clampedEnd = std::min(range.offset + range.size, size);
        const uint32_t actualSize = clampedEnd - range.offset;
        if(actualSize > 0)
        {
            SetGraphicsPushConstantRange(
                range.stages, range.offset, range.size, static_cast<const unsigned char*>(data) + range.offset);
        }
        else
        {
            break;
        }
    }
}

void CommandBuffer::SetComputePushConstants(const void *data, uint32_t size)
{
    const ComputePipeline *pipeline = GetCurrentComputePipeline();
    assert(pipeline);
    const Span<Shader::PushConstantRange> pushConstantRanges = pipeline->GetShaderInfo()->GetPushConstantRanges();
    for(const Shader::PushConstantRange &range : pushConstantRanges)
    {
        const uint32_t clampedEnd = std::min(range.offset + range.size, size);
        const uint32_t actualSize = clampedEnd - range.offset;
        if(actualSize > 0)
        {
            SetComputePushConstantRange(
                range.stages, range.offset, range.size, static_cast<const unsigned char *>(data) + range.offset);
        }
        else
        {
            break;
        }
    }
}

void CommandBuffer::SetGraphicsPushConstants(Span<unsigned char> data)
{
    if(!data.IsEmpty())
    {
        SetGraphicsPushConstants(data.GetData(), data.GetSize());
    }
}

void CommandBuffer::SetComputePushConstants(Span<unsigned char> data)
{
    if(!data.IsEmpty())
    {
        SetComputePushConstants(data.GetData(), data.GetSize());
    }
}

void CommandBuffer::ClearColorTexture2D(const RC<Texture> &tex, const Vector4f &color)
{
    CheckThreadID();
    rhiCommandBuffer_->ClearColorTexture2D(
        tex->GetRHIObject().Get(), ColorClearValue{ color.x, color.y, color.z, color.w });
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

void CommandBuffer::Dispatch(int groupCountX, int groupCountY, int groupCountZ)
{
    CheckThreadID();
    rhiCommandBuffer_->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void CommandBuffer::Dispatch(const Vector3i &groupCount)
{
    this->Dispatch(groupCount.x, groupCount.y, groupCount.z);
}

void CommandBuffer::ExecuteBarriers(const BarrierBatch &barriers)
{
    CheckThreadID();
    if(!barriers.BT_.empty() || !barriers.TT_.empty())
    {
        rhiCommandBuffer_->ExecuteBarriers(barriers.BT_, barriers.TT_);
    }
}

void CommandBuffer::CheckThreadID() const
{
#if RTRC_DEBUG
    assert(threadID_ == std::thread::id() || threadID_ == std::this_thread::get_id());
#endif
}

CommandBufferManager::CommandBufferManager(RHI::DevicePtr device, DeviceSynchronizer &sync)
    : device_(std::move(device)), sync_(sync)
{
    
}

CommandBufferManager::~CommandBufferManager()
{
    for(PerThreadPoolData &poolData : std::ranges::views::values(threadToActivePoolData_))
    {
        if(poolData.activePool)
        {
            assert(poolData.activePool->activeUserCount == 0 && "Not all command buffers are destroyed");
            delete poolData.activePool;
        }
    }
}

CommandBuffer CommandBufferManager::Create()
{
    CommandBuffer ret;
    ret.manager_ = this;
    ret.queueType_ = sync_.GetQueue()->GetType();
    return ret;
}

void CommandBufferManager::_internalEndFrame()
{
    std::unique_lock lock(threadToActivePoolDataMutex_);
    for(PerThreadPoolData &poolData : std::ranges::views::values(threadToActivePoolData_))
    {
        if(poolData.activePool)
        {
            assert(poolData.activePool->activeUserCount == 0 && "Cross frame command buffer is not allowed");
            sync_.OnFrameComplete([p = std::move(poolData.activePool->rhiPool), &fps = freePools_]() mutable
            {
                p->Reset();
                fps.push(std::move(p));
            });
            delete poolData.activePool;
        }
    }
    threadToActivePoolData_.clear();
}

void CommandBufferManager::_internalAllocate(CommandBuffer &commandBuffer)
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

void CommandBufferManager::_internalRelease(CommandBuffer &commandBuffer)
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
                fps.push(std::move(p));
            });
            delete commandBuffer.pool_;
        }
    }
}

CommandBufferManager::PerThreadPoolData &CommandBufferManager::GetPerThreadPoolData()
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

RHI::CommandPoolPtr CommandBufferManager::GetFreeCommandPool()
{
    RHI::CommandPoolPtr ret;
    if(!freePools_.try_pop(ret))
    {
        ret = device_->CreateCommandPool(sync_.GetQueue());
    }
    return ret;
}

RTRC_END
