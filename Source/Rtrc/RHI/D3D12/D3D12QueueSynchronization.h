#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/RHI/D3D12/D3D12Common.h>

RTRC_RHI_D3D12_BEGIN

class D3D12FenceSemaphoreManager;
class D3D12Queue;

struct QueueSyncPoint : ReferenceCounted
{
    uint64_t stamp = UINT64_MAX;
    ObserverPtr<D3D12Queue> queue;
    Ref<QueueSyncPoint> parent;
};

class D3D12Fence : public Fence
{
public:

    D3D12Fence(D3D12FenceSemaphoreManager *manager, ComPtr<ID3D12Fence> fence, uint64_t signalValue);

    void Wait() RTRC_RHI_OVERRIDE;

private:

    friend class D3D12FenceSemaphoreManager;
    friend class D3D12Queue;

    D3D12FenceSemaphoreManager *manager_;

    ComPtr<ID3D12Fence> fence_;
    uint64_t            signalValue_;

    ObserverPtr<D3D12Queue> signaler_;
    Ref<QueueSyncPoint>     signaledQueueSyncPoint_;
};

class D3D12Semaphore : public Semaphore
{
public:

    D3D12Semaphore(D3D12FenceSemaphoreManager *manager, ComPtr<ID3D12Fence> fence, uint64_t signalValue);

private:

    friend class D3D12FenceSemaphoreManager;
    friend class D3D12Queue;

    D3D12FenceSemaphoreManager *manager_;

    ComPtr<ID3D12Fence> fence_;
    uint64_t            signalValue_;

    ObserverPtr<D3D12Queue> signaler_;
    Ref<QueueSyncPoint>     signaledQueueSyncPoint_;
};

class D3D12FenceSemaphoreManager : public Uncopyable
{
public:

    explicit D3D12FenceSemaphoreManager(ID3D12Device *device);

    ReferenceCountedPtr<D3D12Fence>     NewFence();
    ReferenceCountedPtr<D3D12Semaphore> NewSemaphore();

    void Recycle();

private:

    struct PendingFence
    {
        ComPtr<ID3D12Fence>     fence;
        uint64_t                signalValue;
        ObserverPtr<D3D12Queue> signaler;
        Ref<QueueSyncPoint>     syncPoint;
    };

    struct FreeFence
    {
        ComPtr<ID3D12Fence> fence;
        uint64_t            signalValue;
    };

    ID3D12Device *device_;

    std::vector<PendingFence> pendingFences_;
    std::vector<FreeFence> freeFences_;
};

RTRC_RHI_D3D12_END
