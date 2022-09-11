#include <Rtrc/Graphics/RHI/DirectX12/Queue/Queue.h>

RTRC_RHI_DIRECTX12_BEGIN

DirectX12Queue::DirectX12Queue(QueueType type, ComPtr<ID3D12CommandQueue> queue)
    : type_(type), queue_(std::move(queue))
{
    
}

QueueType DirectX12Queue::GetType() const
{
    return type_;
}

void DirectX12Queue::Submit(
    BackBufferSemaphoreDependency waitBackBufferSemaphore,
    Span<SemaphoreDependency>     waitSemaphores,
    Span<Ptr<CommandBuffer>>      commandBuffers,
    BackBufferSemaphoreDependency signalBackBufferSemaphore,
    Span<SemaphoreDependency>     signalSemaphores,
    const Ptr<Fence>             &signalFence)
{
    // TODO
}

void DirectX12Queue::WaitIdle()
{
    // TODO
}

RTRC_RHI_DIRECTX12_END
