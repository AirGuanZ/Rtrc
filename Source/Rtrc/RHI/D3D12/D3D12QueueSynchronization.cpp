#include <Rtrc/RHI/D3D12/D3D12Queue.h>

RTRC_RHI_D3D12_BEGIN

D3D12Fence::D3D12Fence(D3D12FenceSemaphoreManager *manager, ComPtr<ID3D12Fence> fence, uint64_t signalValue)
    : manager_(manager), fence_(std::move(fence)), signalValue_(signalValue)
{
    
}

void D3D12Fence::Wait()
{
    const HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
    RTRC_D3D12_FAIL_MSG(
        fence_->SetEventOnCompletion(signalValue_, eventHandle),
        "Fail to set event on completion for directx12 queue fence");
    WaitForSingleObject(eventHandle, INFINITE);
    CloseHandle(eventHandle);

    assert(signaler_);
    signaler_->MarkAsSynchronized(*signaledQueueSyncPoint_);
}

D3D12Semaphore::D3D12Semaphore(D3D12FenceSemaphoreManager *manager, ComPtr<ID3D12Fence> fence, uint64_t signalValue)
    : manager_(manager), fence_(std::move(fence)), signalValue_(signalValue)
{
    
}

D3D12FenceSemaphoreManager::D3D12FenceSemaphoreManager(ID3D12Device *device)
    : device_(device)
{
    
}

ReferenceCountedPtr<D3D12Semaphore> D3D12FenceSemaphoreManager::NewSemaphore()
{
    if(freeFences_.empty())
    {
        ComPtr<ID3D12Fence> d3d12Fence;
        RTRC_D3D12_FAIL_MSG(
            device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(d3d12Fence.GetAddressOf())),
            "Failed to create d3d12 fence");

        return MakeReferenceCountedPtr<D3D12Semaphore>(this, std::move(d3d12Fence), 1);
    }

    auto freeFence = freeFences_.back();
    freeFences_.pop_back();

    auto semaphore = MakeReferenceCountedPtr<D3D12Semaphore>(this, std::move(freeFence.fence), freeFence.signalValue);
    return semaphore;
}

ReferenceCountedPtr<D3D12Fence> D3D12FenceSemaphoreManager::NewFence()
{
    if(freeFences_.empty())
    {
        ComPtr<ID3D12Fence> d3d12Fence;
        RTRC_D3D12_FAIL_MSG(
            device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(d3d12Fence.GetAddressOf())),
            "Failed to create d3d12 fence");

        return MakeReferenceCountedPtr<D3D12Fence>(this, std::move(d3d12Fence), 1);
    }

    auto freeFence = freeFences_.back();
    freeFences_.pop_back();

    auto fence = MakeReferenceCountedPtr<D3D12Fence>(this, std::move(freeFence.fence), freeFence.signalValue);
    return fence;
}

void D3D12FenceSemaphoreManager::Recycle()
{
    auto IsFree = [this](const PendingFence &pendingFence)
    {
        if(!pendingFence.signaler)
        {
            // The pending fence haven't been and can never be signaled. Can reuse it safely.
            return true;
        }
        return pendingFence.signaler->IsSynchronized(pendingFence.syncPoint->stamp);
    };

    for(size_t index = 0; index < pendingFences_.size();)
    {
        if(auto &pendingFence = pendingFences_[index]; IsFree(pendingFence))
        {
            auto &freeFence = freeFences_.emplace_back();
            freeFence.fence = std::move(pendingFence.fence);
            freeFence.signalValue = pendingFence.signalValue + 1;

            std::swap(pendingFences_[index], pendingFences_.back());
            pendingFences_.pop_back();
        }
        else
        {
            ++index;
        }
    }
}

RTRC_RHI_D3D12_END
