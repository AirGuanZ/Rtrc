#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Queue, Queue)
{
public:

#if RTRC_STATIC_RHI
    RTRC_RHI_QUEUE_COMMON
#endif

    DirectX12Queue(DirectX12Device *device, ComPtr<ID3D12CommandQueue> queue, QueueType type);

    QueueType GetType() const RTRC_RHI_OVERRIDE;

    void WaitIdle() RTRC_RHI_OVERRIDE;

    void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<RPtr<CommandBuffer>>     commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        OPtr<Fence>                   signalFence) RTRC_RHI_OVERRIDE;

    QueueSessionID GetCurrentSessionID() RTRC_RHI_OVERRIDE;
    QueueSessionID GetSynchronizedSessionID() RTRC_RHI_OVERRIDE;

    ID3D12CommandQueue *_internalGetCommandQueue() const { return queue_.Get(); }

private:

    DirectX12Device            *device_;
    ComPtr<ID3D12CommandQueue>  queue_;
    QueueType                   type_;
    std::mutex                  waitIdleMutex_;
    ComPtr<ID3D12Fence>         waitIdleFence_;
    uint64_t                    waitIdleFenceValue_;
    std::atomic<QueueSessionID> currentSessionID_;
    std::atomic<QueueSessionID> synchronizedSessionID_;
};

RTRC_RHI_D3D12_END
