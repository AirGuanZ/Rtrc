#include <RHI/DirectX12/Context/BackBufferSemaphore.h>
#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Queue/CommandBuffer.h>
#include <RHI/DirectX12/Queue/Fence.h>
#include <RHI/DirectX12/Queue/Queue.h>
#include <RHI/DirectX12/Queue/Semaphore.h>
#include <RHI/DirectX12/Resource/Sampler.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Queue::DirectX12Queue(DirectX12Device *device, ComPtr<ID3D12CommandQueue> queue, QueueType type)
    : device_(device), queue_(std::move(queue)), type_(type), fenceValue_(1)
{
    RTRC_D3D12_FAIL_MSG(
        device_->_internalGetNativeDevice()->CreateFence(
            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf())),
        "Fail to create directx12 fence for directx12 queue");
}

QueueType DirectX12Queue::GetType() const
{
    return type_;
}

void DirectX12Queue::WaitIdle()
{
    queue_->Signal(fence_.Get(), ++fenceValue_);
    
    const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    RTRC_D3D12_FAIL_MSG(
        fence_->SetEventOnCompletion(fenceValue_, eventHandle),
        "Fail to set event on completion for directx12 queue fence");
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);
}

void DirectX12Queue::Submit(
    BackBufferSemaphoreDependency waitBackBufferSemaphore,
    Span<SemaphoreDependency>     waitSemaphores,
    Span<Ptr<CommandBuffer>>      commandBuffers,
    BackBufferSemaphoreDependency signalBackBufferSemaphore,
    Span<SemaphoreDependency>     signalSemaphores,
    const Ptr<Fence>             &signalFence)
{
    /*assert(
        waitBackBufferSemaphore.semaphore == nullptr ||
        static_cast<const DirectX12BackBufferSemaphore *>(waitBackBufferSemaphore.semaphore.Get())
            ->type == DirectX12BackBufferSemaphore::Acquire);*/

    for(auto &s : waitSemaphores)
    {
        auto ds = static_cast<DirectX12Semaphore *>(s.semaphore.Get());
        RTRC_D3D12_FAIL_MSG(
            queue_->Wait(ds->_internalGetNativeFence().Get(), s.value),
            "Fail to call ID3D12CommandQueue::Wait");
    }

    if(!commandBuffers.IsEmpty())
    {
        std::vector<ID3D12CommandList *> d3dCommandLists;
        for(auto &c : commandBuffers)
        {
            d3dCommandLists.push_back(static_cast<DirectX12CommandBuffer *>(c.Get())->_internalGetCommandList());
        }
        queue_->ExecuteCommandLists(static_cast<UINT>(d3dCommandLists.size()), d3dCommandLists.data());
    }

    for(auto &s : signalSemaphores)
    {
        auto ds = static_cast<DirectX12Semaphore *>(s.semaphore.Get());
        RTRC_D3D12_FAIL_MSG(
            queue_->Signal(ds->_internalGetNativeFence().Get(), s.value),
            "Fail to call ID3D12CommandQueue::Signal on semaphore");
    }

    if(signalFence)
    {
        auto df = static_cast<DirectX12Fence *>(signalFence.Get());
        RTRC_D3D12_FAIL_MSG(
            queue_->Signal(df->_internalGetNativeFence().Get(), df->GetSignalValue()),
            "Fail to call ID3D12CommandQueue::Signal on fence");
    }
}

RTRC_RHI_D3D12_END
