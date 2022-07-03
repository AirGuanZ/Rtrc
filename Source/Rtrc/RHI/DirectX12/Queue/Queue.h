#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Queue : public Queue
{
public:

    DirectX12Queue(QueueType type, ComPtr<ID3D12CommandQueue> queue);

    QueueType GetType() const override;

    RC<CommandPool> CreateCommandPool() override;

    void WaitIdle() override;

    void Submit(const RC<BackBufferSemaphore> &waitBackBufferSemaphore, PipelineStage waitBackBufferStages, Span<RC<CommandBuffer>> commandBuffers, const RC<BackBufferSemaphore> &signalBackBufferSemaphore, PipelineStage signalBackBufferStages, const RC<Fence> &signalFence) override;

private:

    QueueType type_;
    ComPtr<ID3D12CommandQueue> queue_;
};

RTRC_RHI_DIRECTX12_END
