#include <shared_mutex>

#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Material/MaterialInstance.h>
#include <Rtrc/Graphics/Object/CommandBuffer.h>

RTRC_BEGIN

CommandBuffer::CommandBuffer()
    : manager_(nullptr), queueType_(RHI::QueueType::Graphics)
{
    
}

CommandBuffer::~CommandBuffer()
{
    if(rhiCommandBuffer_)
    {
        manager_->_rtrcFreeInternal(*this);
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
    std::swap(manager_, other.manager_);
    rhiCommandBuffer_.Swap(other.rhiCommandBuffer_);
    std::swap(queueType_, other.queueType_);
    std::swap(pool_, other.pool_);
#if RTRC_DEBUG
    std::swap(threadID_, other.threadID_);
#endif
}

const RHI::CommandBufferPtr &CommandBuffer::GetRHIObject() const
{
    CheckThreadID();
    return rhiCommandBuffer_;
}

void CommandBuffer::Begin()
{
    manager_->_rtrcAllocateInternal(*this);
    assert(rhiCommandBuffer_);
    rhiCommandBuffer_->Begin();
}

void CommandBuffer::End()
{
    CheckThreadID();
    rhiCommandBuffer_->End();
}

void CommandBuffer::CopyBuffer(Buffer &dst, size_t dstOffset, const Buffer &src, size_t srcOffset, size_t size)
{
    CheckThreadID();
    rhiCommandBuffer_->CopyBuffer(dst.GetRHIObject(), dstOffset, src.GetRHIObject(), srcOffset, size);
}

void CommandBuffer::BeginRenderPass(Span<ColorAttachment> colorAttachments)
{
    CheckThreadID();
    rhiCommandBuffer_->BeginRenderPass(colorAttachments);
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

void CommandBuffer::BindGraphicsGroup(int index, const RC<BindingGroup> &group)
{
    CheckThreadID();
    rhiCommandBuffer_->BindGroupToGraphicsPipeline(index, group->GetRHIObject());
}

void CommandBuffer::BindGraphicsSubMaterial(const SubMaterialInstance *subMatInst, const KeywordValueContext &keywords)
{
    subMatInst->BindGraphicsProperties(keywords, *this);
}

void CommandBuffer::BindComputeSubMaterial(const SubMaterialInstance *subMatInst, const KeywordValueContext &keywords)
{
    subMatInst->BindComputeProperties(keywords, *this);
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

void CommandBuffer::Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance)
{
    CheckThreadID();
    rhiCommandBuffer_->Draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

CommandBufferManager::CommandBufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device)
    : device_(std::move(device)), batchRelease_(hostSync)
{
    queue_ = hostSync.GetQueue();

    using ReleaseDataList = BatchReleaseHelper<PendingReleasePool>::DataList;
    batchRelease_.SetReleaseCallback([this](int, ReleaseDataList &dataList)
    {
        for(auto &d : dataList)
        {
            freePools_.push(std::move(d.rhiCommandPool));
        }
    });
    batchRelease_.SetPreNewBatchCallback([this]
    {
        // Add pools to old release batch

        for(auto it = threadToActivePools_.begin(); it != threadToActivePools_.end();)
        {
            auto &activePools = it->second;

            for(auto jt = activePools.pools.begin(); jt != activePools.pools.end();)
            {
                if(jt->activeUserCount)
                {
                    ++jt;
                }
                else
                {
                    batchRelease_.AddToCurrentBatch({ std::move(jt->rhiPool) });
                    jt = activePools.pools.erase(jt);
                }
            }

            if(activePools.pools.empty())
            {
                it = threadToActivePools_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    });
}

CommandBuffer CommandBufferManager::Create()
{
    CommandBuffer result;
    result.manager_ = this;
    result.queueType_ = queue_->GetType();
    return result;
}

void CommandBufferManager::_rtrcAllocateInternal(CommandBuffer &cmdBuf)
{
    // Get thread-local pools

    const std::thread::id thisThreadID = std::this_thread::get_id();
    PerThreadActivePools *tls = nullptr;
    {
        std::shared_lock lock(threadToActivePoolsMutex_);
        if(auto it = threadToActivePools_.find(thisThreadID); it != threadToActivePools_.end())
        {
            tls = &it->second;
        }
    }
    if(!tls)
    {
        std::unique_lock lock(threadToActivePoolsMutex_);
        tls = &threadToActivePools_[thisThreadID];
    }

    // Allocate pool
    
    if(tls->pools.empty() ||
       tls->pools.back().historyUserCount == MAX_COMMAND_BUFFER_IN_SINGLE_POOL)
    {
        auto &newPool = tls->pools.emplace_front();
        if(!freePools_.try_pop(newPool.rhiPool))
        {
            newPool.rhiPool = device_->CreateCommandPool(device_->GetQueue(cmdBuf.queueType_));
        }
    }

    // Allocate command buffer

    cmdBuf.pool_ = tls->pools.begin();
    cmdBuf.rhiCommandBuffer_ = tls->pools.front().rhiPool->NewCommandBuffer();
#if RTRC_DEBUG
    cmdBuf.threadID_ = thisThreadID;
#endif
    ++tls->pools.front().historyUserCount;
}

void CommandBufferManager::_rtrcFreeInternal(CommandBuffer &cmdBuf)
{
    assert(cmdBuf.manager_ == this);
    assert(cmdBuf.rhiCommandBuffer_);
    --cmdBuf.pool_->activeUserCount;
    cmdBuf.rhiCommandBuffer_ = {};
    cmdBuf.pool_ = {};
#if RTRC_DEBUG
    cmdBuf.threadID_ = {};
#endif
}

RTRC_END
