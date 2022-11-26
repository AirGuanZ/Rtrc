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
    RTRC_SWAP_MEMBERS(*this, other, manager_, queueType_, rhiCommandBuffer_, pool_)
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
}

void CommandBuffer::BeginDebugEvent(std::string name, const std::optional<Vector4f> &color)
{
    CheckThreadID();
    RHI::DebugLabel debugLabel{ std::move(name), color };
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

void CommandBuffer::BindPipeline(const RC<GraphicsPipeline> &graphicsPipeline)
{
    CheckThreadID();
    rhiCommandBuffer_->BindPipeline(graphicsPipeline->GetRHIObject());
}

void CommandBuffer::BindPipeline(const RC<ComputePipeline> &computePipeline)
{
    CheckThreadID();
    rhiCommandBuffer_->BindPipeline(computePipeline->GetRHIObject());
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
#if RTRC_DEBUG
    std::unique_lock lock(threadToActivePoolDataMutex_);
#endif

    for(PerThreadPoolData &poolData : std::ranges::views::values(threadToActivePoolData_))
    {
        if(poolData.activePool)
        {
            assert(poolData.activePool->activeUserCount == 0 && "Cross frame command buffer is not allowed");
            sync_.OnFrameComplete([p = std::move(poolData.activePool->rhiPool), &fps = freePools_]() mutable
            {
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
