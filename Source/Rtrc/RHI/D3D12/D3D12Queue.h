#pragma once

#include <Rtrc/RHI/D3D12/D3D12QueueSynchronization.h>

RTRC_RHI_D3D12_BEGIN

class D3D12CommandAllocator : public Uncopyable
{
public:

    D3D12CommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type);

    CommandBufferRef AllocateCommandBuffer();

    void AddSyncPoint(uint64_t index);
    uint64_t GetLatestSyncPoint() const;

    void Reset();

private:

    ID3D12Device           *device_;
    D3D12_COMMAND_LIST_TYPE type_;

    std::atomic<uint64_t>          syncPoint_;
    ComPtr<ID3D12CommandAllocator> allocator_;
    std::vector<CommandBufferRef>  usedCommandBuffers_;
    std::vector<CommandBufferRef>  freeCommandBuffers_;
};

class D3D12Queue : public Queue
{
public:

    D3D12Queue(ID3D12Device *device, ComPtr<ID3D12CommandQueue> queue, QueueType type);

    CommandBufferRef NewCommandBuffer() RTRC_RHI_OVERRIDE;
    void Submit(Span<CommandBufferRef> commandBuffer) RTRC_RHI_OVERRIDE;

    void SignalFence(FenceRef fence) RTRC_RHI_OVERRIDE;

    void SignalSemaphore(SemaphoreRef semaphore) RTRC_RHI_OVERRIDE;
    void WaitSemaphore(SemaphoreRef semaphore) RTRC_RHI_OVERRIDE;

    void NewCommandAllocator();

    // Mark all tasks submitted before the given sync point as synchronized by the CPU
    void MarkAsSynchronized(const QueueSyncPoint &point);

    bool IsSynchronized(uint64_t stamp) const;

    ID3D12CommandQueue *GetNativeQueue();

private:

    Ref<QueueSyncPoint> NewSyncPoint();

    ID3D12Device *device_;
    QueueType type_;
    ComPtr<ID3D12CommandQueue> queue_;

    std::vector<Ref<QueueSyncPoint>> syncPoints_;
    uint64_t nextStamp_;

    std::vector<std::unique_ptr<D3D12CommandAllocator>> flightCommandAllocators_;
    std::unique_ptr<D3D12CommandAllocator> currentCommandAllocator_;
};

RTRC_RHI_D3D12_END
