#include <ranges>

#include <Rtrc/Core/Atomic.h>
#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/RHI/D3D12/D3D12CommandBuffer.h>
#include <Rtrc/RHI/D3D12/D3D12Queue.h>

#include "D3D12Device.h"

RTRC_RHI_D3D12_BEGIN
    D3D12CommandAllocator::D3D12CommandAllocator(ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type)
    : device_(device), type_(type)
{
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandAllocator(type_, IID_PPV_ARGS(allocator_.GetAddressOf())),
        "Failed to create new D3D12 command allocator");
}

CommandBufferRef D3D12CommandAllocator::AllocateCommandBuffer()
{
    if(!freeCommandBuffers_.empty())
    {
        auto ret = std::move(freeCommandBuffers_.back());
        freeCommandBuffers_.pop_back();
        usedCommandBuffers_.push_back(ret);
        static_cast<D3D12CommandBuffer *>(ret.Get())
            ->_internalGetCommandList()->Reset(allocator_.Get(), nullptr);
        return ret;
    }

    ComPtr<ID3D12GraphicsCommandList> commandList;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandList(0, type_, allocator_.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())),
        "Failed to create new D3D12 command list");

    auto ret = MakeReferenceCountedPtr<D3D12CommandBuffer>(std::move(commandList), this);
    usedCommandBuffers_.push_back(ret);
    return ret;
}

void D3D12CommandAllocator::Reset()
{
    allocator_->Reset();
    freeCommandBuffers_ = std::move(usedCommandBuffers_);
    syncPoint_ = 0;
}

void D3D12CommandAllocator::AddSyncPoint(uint64_t index)
{
    AtomicMax(syncPoint_, index);
}

uint64_t D3D12CommandAllocator::GetLatestSyncPoint() const
{
    return syncPoint_.load();
}

D3D12Queue::D3D12Queue(D3D12Device *device, ComPtr<ID3D12CommandQueue> queue, QueueType type)
    : device_(device), type_(type), queue_(std::move(queue)), nextStamp_(1)
{
    currentCommandAllocator_ = std::make_unique<D3D12CommandAllocator>(
        device_->GetNativeDevice(), QueueTypeToD3D12CommandListType(type));
}

CommandBufferRef D3D12Queue::NewCommandBuffer()
{
    return currentCommandAllocator_->AllocateCommandBuffer();
}

void D3D12Queue::Submit(Span<CommandBufferRef> commandBuffers)
{
    auto syncPoint = NewSyncPoint();
    thread_local std::vector<ID3D12CommandList *> commandLists;
    commandLists.clear();
    for(auto& commandBuffer : commandBuffers)
    {
        auto d3d12CommandBuffer = static_cast<D3D12CommandBuffer *>(commandBuffer.Get());
        d3d12CommandBuffer->_internalGetCommandAllocator()->AddSyncPoint(syncPoint->stamp);
        commandLists.push_back((d3d12CommandBuffer->_internalGetCommandList()));
    }
    queue_->ExecuteCommandLists(commandLists.size(), commandLists.data());
}

void D3D12Queue::SignalFence(FenceRef fence)
{
    auto d3d12Fence = static_cast<D3D12Fence*>(fence.Get());
    queue_->Signal(d3d12Fence->fence_.Get(), d3d12Fence->signalValue_);

    assert(!d3d12Fence->signaler_);
    assert(!d3d12Fence->signaledQueueSyncPoint_);
    d3d12Fence->signaler_ = this;
    d3d12Fence->signaledQueueSyncPoint_ = NewSyncPoint();
}

void D3D12Queue::SignalSemaphore(SemaphoreRef semaphore)
{
    auto d3d12Semaphore = static_cast<D3D12Semaphore*>(semaphore.Get());
    queue_->Signal(d3d12Semaphore->fence_.Get(), d3d12Semaphore->signalValue_);

    assert(!d3d12Semaphore->signaler_ && !d3d12Semaphore->signaledQueueSyncPoint_);
    d3d12Semaphore->signaler_ = this;
    d3d12Semaphore->signaledQueueSyncPoint_ = NewSyncPoint();
}

void D3D12Queue::WaitSemaphore(SemaphoreRef semaphore)
{
    auto d3d12Semaphore = static_cast<D3D12Semaphore*>(semaphore.Get());
    queue_->Wait(d3d12Semaphore->fence_.Get(), d3d12Semaphore->signalValue_);

    assert(d3d12Semaphore->signaler_ && d3d12Semaphore->signaledQueueSyncPoint_);
    assert(d3d12Semaphore->signaler_.Get() != this);
    NewSyncPoint()->parent = d3d12Semaphore->signaledQueueSyncPoint_;
}

void D3D12Queue::WaitIdle()
{
    auto fence = device_->CreateQueueFence();
    SignalFence(fence);
    fence->Wait();
}

void D3D12Queue::NewCommandAllocator()
{
    flightCommandAllocators_.push_back(std::move(currentCommandAllocator_));
    currentCommandAllocator_.reset();

    // Try to find a free allocator in flightCommandAllocators_

    for(size_t index = 0; index < flightCommandAllocators_.size(); ++index)
    {
        if(IsSynchronized(flightCommandAllocators_[index]->GetLatestSyncPoint()))
        {
            const size_t lastIndex = flightCommandAllocators_.size() - 1;
            std::swap(flightCommandAllocators_[index], flightCommandAllocators_[lastIndex]);
            currentCommandAllocator_ = std::move(flightCommandAllocators_[lastIndex]);
            flightCommandAllocators_.pop_back();
            currentCommandAllocator_->Reset();
            return;
        }
    }

    // All existing allocators are on-the-flight. Create a new one.

    currentCommandAllocator_ = std::make_unique<D3D12CommandAllocator>(
        device_->GetNativeDevice(), QueueTypeToD3D12CommandListType(type_));
}

void D3D12Queue::MarkAsSynchronized(const QueueSyncPoint &point)
{
    assert(point.queue.Get() == this);
    if(syncPoints_.empty() || point.stamp < syncPoints_[0]->stamp)
    {
        assert(IsSynchronized(point.stamp));
        return;
    }

    size_t removedPointCount = syncPoints_.size();
    for(size_t i = 0; i < syncPoints_.size(); ++i)
    {
        if(syncPoints_[i]->stamp > point.stamp)
        {
            removedPointCount = i;
            break;
        }
    }

    if(removedPointCount)
    {
        syncPoints_ = syncPoints_
            | std::views::drop(removedPointCount)
            | std::ranges::to<std::vector<Ref<QueueSyncPoint>>>();
    }

    if(point.parent)
    {
        point.parent->queue->MarkAsSynchronized(*point.parent);
    }
}

bool D3D12Queue::IsSynchronized(uint64_t stamp) const
{
    if(!syncPoints_.empty())
    {
        assert(stamp < nextStamp_);
        return true;
    }
    return stamp < syncPoints_[0]->stamp;
}

ID3D12CommandQueue *D3D12Queue::GetNativeQueue()
{
    return queue_.Get();
}

Ref<QueueSyncPoint> D3D12Queue::NewSyncPoint()
{
    auto syncPoint = MakeReferenceCountedPtr<QueueSyncPoint>();
    syncPoint->stamp = nextStamp_++;
    syncPoint->queue = this;
    syncPoints_.emplace_back(syncPoint);
    return syncPoint;
}

RTRC_RHI_D3D12_END
