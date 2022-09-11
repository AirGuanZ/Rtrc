#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Queue : public Queue
{
public:

    DirectX12Queue(QueueType type, ComPtr<ID3D12CommandQueue> queue);

    QueueType GetType() const override;

    void WaitIdle() override;

    void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<Ptr<CommandBuffer>>      commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        const Ptr<Fence>             &signalFence) override;

private:

    QueueType type_;
    ComPtr<ID3D12CommandQueue> queue_;
};

RTRC_RHI_DIRECTX12_END
