#include <Rtrc/RHI/DirectX12/Queue/Queue.h>

RTRC_RHI_DIRECTX12_BEGIN

DirectX12Queue::DirectX12Queue(QueueType type, ComPtr<ID3D12CommandQueue> queue)
    : type_(type), queue_(std::move(queue))
{
    
}

QueueType DirectX12Queue::GetType() const
{
    return type_;
}

Ptr<CommandPool> DirectX12Queue::CreateCommandPool()
{
    // TODO
    return {};
}

void DirectX12Queue::Submit(
    const Ptr<BackBufferSemaphore> &waitBackBufferSemaphore,
    PipelineStage                   waitBackBufferStages,
    Span<Ptr<CommandBuffer>>        commandBuffers,
    const Ptr<BackBufferSemaphore> &signalBackBufferSemaphore,
    PipelineStage                   signalBackBufferStages,
    const Ptr<Fence>               &signalFence)
{
    // TODO
}

void DirectX12Queue::WaitIdle()
{
    // TODO
}

RTRC_RHI_DIRECTX12_END
