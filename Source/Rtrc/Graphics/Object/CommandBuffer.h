#pragma once

#include <tbb/concurrent_queue.h>
#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Object/Pipeline.h>

RTRC_BEGIN

class Buffer;
class Texture2D;
class CommandBuffer;
class CommandBufferManager;
class KeywordValueContext;
class SubMaterialInstance;

using AttachmentLoadOp = RHI::AttachmentLoadOp;
using AttachmentStoreOp = RHI::AttachmentStoreOp;
using ColorClearValue = RHI::ColorClearValue;

using Viewport = RHI::Viewport;
using Scissor = RHI::Scissor;

using ColorAttachment = RHI::RenderPassColorAttachment;

class BarrierBatch
{
public:

    BarrierBatch() = default;

    template<typename A, typename B, typename...Args>
    BarrierBatch(const A &a, const B &b, const Args...args)
    {
        operator()(a, b, args...);
    }

    BarrierBatch &operator()(
        const RC<Buffer>       &buffer,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    BarrierBatch &operator()(
        const RC<Texture2D>    &texture,
        RHI::TextureLayout      layout,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    BarrierBatch &operator()(
        const RC<Texture2D>    &texture,
        uint32_t                arrayLayer,
        uint32_t                mipLevel,
        RHI::TextureLayout      layout,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

private:

    friend class CommandBuffer;

    std::vector<RHI::BufferTransitionBarrier> BT_;
    std::vector<RHI::TextureTransitionBarrier> TT_;
};

class CommandBuffer : public Uncopyable
{
public:

    CommandBuffer();
    ~CommandBuffer();

    CommandBuffer(CommandBuffer &&other) noexcept;
    CommandBuffer &operator=(CommandBuffer &&other) noexcept;

    void Swap(CommandBuffer &other) noexcept;

    RHI::QueueType GetQueueType() const;

    const RHI::CommandBufferPtr &GetRHIObject() const;

    // Once begin, the command buffer object is bound with current thread, and cannot be used in any other thread.
    void Begin();
    void End();

    void CopyBuffer(Buffer &dst, size_t dstOffset, const Buffer &src, size_t srcOffset, size_t size);

    void BeginRenderPass(Span<ColorAttachment> colorAttachments);
    void EndRenderPass();

    void BindPipeline(const RC<GraphicsPipeline> &graphicsPipeline);

    void BindGraphicsGroup(int index, const RC<BindingGroup> &group);
    void BindGraphicsSubMaterial(const SubMaterialInstance *subMatInst, const KeywordValueContext &keywords);
    void BindComputeSubMaterial(const SubMaterialInstance *subMatInst, const KeywordValueContext &keywords);

    void SetViewports(Span<Viewport> viewports);
    void SetScissors(Span<Scissor> scissors);

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);

    void ExecuteBarriers(const BarrierBatch &barriers);

private:

    friend class CommandBufferManager;

    struct Pool
    {
        RHI::CommandPoolPtr rhiPool;
        uint32_t historyUserCount = 0;
        std::atomic<uint32_t> activeUserCount = 0;
    };

    void CheckThreadID() const;

    CommandBufferManager *manager_;
    RHI::QueueType queueType_;
    RHI::CommandBufferPtr rhiCommandBuffer_;
    std::list<Pool>::iterator pool_;
#if RTRC_DEBUG
    std::thread::id threadID_;
#endif
};

class CommandBufferManager : public Uncopyable
{
public:

    CommandBufferManager(HostSynchronizer &hostSync, RHI::DevicePtr device);

    CommandBuffer Create();

    void _rtrcAllocateInternal(CommandBuffer &cmdBuf);
    void _rtrcFreeInternal(CommandBuffer &cmdBuf);

private:

    static constexpr int MAX_COMMAND_BUFFER_IN_SINGLE_POOL = 8;

    struct PerThreadActivePools
    {
        std::list<CommandBuffer::Pool> pools;
    };

    struct PendingReleasePool
    {
        RHI::CommandPoolPtr rhiCommandPool;
    };

    RHI::DevicePtr device_;
    RHI::QueuePtr queue_;

    std::map<std::thread::id, PerThreadActivePools> threadToActivePools_;
    tbb::spin_rw_mutex threadToActivePoolsMutex_;

    BatchReleaseHelper<PendingReleasePool> batchRelease_;
    tbb::concurrent_queue<RHI::CommandPoolPtr> freePools_;
};

inline RHI::QueueType CommandBuffer::GetQueueType() const
{
    return queueType_;
}

inline void CommandBuffer::CheckThreadID() const
{
#if RTRC_DEBUG
    assert(threadID_ == std::thread::id() || threadID_ == std::this_thread::get_id());
#endif
}

RTRC_END
