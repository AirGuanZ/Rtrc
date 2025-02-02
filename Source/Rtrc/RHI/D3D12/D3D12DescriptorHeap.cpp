#include <mutex>
#include <ranges>

#include <Rtrc/RHI/D3D12/D3D12DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

D3D12ThreadedCPUDescriptorHeap::~D3D12ThreadedCPUDescriptorHeap()
{
    parentHeap_->FreeThreadedDescriptors(*this);
}

D3D12CPUDescriptor D3D12ThreadedCPUDescriptorHeap::Allocate()
{
    if(freeDescriptors_.empty())
    {
        parentHeap_->AllocateThreadedDescriptors(chunkSize_, freeDescriptors_);
        assert(!freeDescriptors_.empty());
    }
    const D3D12CPUDescriptor ret = freeDescriptors_.back();
    freeDescriptors_.pop_back();
    return ret;
}

void D3D12ThreadedCPUDescriptorHeap::Free(const D3D12CPUDescriptor &descriptor)
{
    freeDescriptors_.push_back(descriptor);
}

D3D12ThreadedCPUDescriptorHeap::D3D12ThreadedCPUDescriptorHeap(D3D12CPUDescriptorHeap *parentHeap, uint32_t chunkSize)
    : parentHeap_(parentHeap), chunkSize_(chunkSize)
{
    assert(parentHeap_);
}

D3D12CPUDescriptorHeap::D3D12CPUDescriptorHeap(
    ID3D12Device              *device,
    uint32_t                   nativeHeapSize,
    D3D12_DESCRIPTOR_HEAP_TYPE heapType)
    : device_(device)
    , nativeHeapSize_(nativeHeapSize)
    , heapType_(heapType)
    , firstDscriptorInNewHeap_{ }
    , nextIndexInNewDescriptorHeap_(0)
    , activeThreadedDescriptorHeapCount_(0)
{
    descriptorSize_ = device_->GetDescriptorHandleIncrementSize(heapType_);
}

D3D12CPUDescriptor D3D12CPUDescriptorHeap::Allocate()
{
    assert(!activeThreadedDescriptorHeapCount_);
    if(!freeDescriptors_.empty())
    {
        const auto ret = freeDescriptors_.back();
        freeDescriptors_.pop_back();
        return ret;
    }
    if(!newDescriptorHeap_ || nextIndexInNewDescriptorHeap_ >= nativeHeapSize_)
    {
        AllocateNewDescriptorHeap();
    }
    const size_t ptr = firstDscriptorInNewHeap_.ptr + descriptorSize_ * nextIndexInNewDescriptorHeap_++;
    return { { ptr } };
}

void D3D12CPUDescriptorHeap::Free(const D3D12CPUDescriptor &descriptor)
{
    assert(!activeThreadedDescriptorHeapCount_);
    freeDescriptors_.push_back(descriptor);
}

std::unique_ptr<D3D12ThreadedCPUDescriptorHeap> D3D12CPUDescriptorHeap::CreateThreadedHeap(uint32_t chunkSize)
{
    ++activeThreadedDescriptorHeapCount_;
    auto heap = new D3D12ThreadedCPUDescriptorHeap(this, chunkSize);
    return std::unique_ptr<D3D12ThreadedCPUDescriptorHeap>(heap);
}

void D3D12CPUDescriptorHeap::AllocateNewDescriptorHeap()
{
    const D3D12_DESCRIPTOR_HEAP_DESC desc =
    {
        .Type           = heapType_,
        .NumDescriptors = nativeHeapSize_,
        .Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask       = 0
    };

    ComPtr<ID3D12DescriptorHeap> heap;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap.GetAddressOf())),
        "Failed to create new d3d12 cpu descriptor heap");

    nextIndexInNewDescriptorHeap_ = 0;
    allDescriptorHeaps_.push_back(heap);
    newDescriptorHeap_ = std::move(heap);
}

void D3D12CPUDescriptorHeap::AllocateThreadedDescriptors(uint32_t count, std::vector<D3D12CPUDescriptor> &output)
{
    assert(activeThreadedDescriptorHeapCount_);
    assert(count);
    output.reserve(output.size() + count);

    std::lock_guard lock(mutex_);

    {
        const uint32_t countFromFreeDescriptors = (std::min<uint32_t>)(count, freeDescriptors_.size());
        const uint32_t copyOffset = freeDescriptors_.size() - countFromFreeDescriptors;
        std::ranges::copy(freeDescriptors_ | std::views::drop(copyOffset), std::back_inserter(output));
        freeDescriptors_.resize(copyOffset);
        count -= countFromFreeDescriptors;
    }

    while(count)
    {
        const uint32_t remainingCountInNewHeap = nativeHeapSize_ - nextIndexInNewDescriptorHeap_;
        if(remainingCountInNewHeap <= 0)
        {
            AllocateNewDescriptorHeap();
            continue;
        }

        const uint32_t countFromNewHeap = (std::min<uint32_t>)(count, remainingCountInNewHeap);
        for(uint32_t i = 0; i < countFromNewHeap; ++i)
        {
            const size_t ptr = firstDscriptorInNewHeap_.ptr + descriptorSize_ * (nextIndexInNewDescriptorHeap_ + i);
            output.push_back({ { ptr } });
        }
        nextIndexInNewDescriptorHeap_ += countFromNewHeap;
        count -= countFromNewHeap;
    }
}

void D3D12CPUDescriptorHeap::FreeThreadedDescriptors(D3D12ThreadedCPUDescriptorHeap &heap)
{
    assert(activeThreadedDescriptorHeapCount_);
    {
        std::lock_guard lock(mutex_);
        freeDescriptors_.reserve(freeDescriptors_.size() + heap.freeDescriptors_.size());
        std::ranges::copy(heap.freeDescriptors_, std::back_inserter(freeDescriptors_));
    }
    --activeThreadedDescriptorHeapCount_;
}

RTRC_RHI_D3D12_END
