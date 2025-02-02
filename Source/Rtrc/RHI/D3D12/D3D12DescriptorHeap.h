#pragma once

#include <tbb/spin_mutex.h>

#include <Rtrc/RHI/D3D12/D3D12Common.h>

RTRC_RHI_D3D12_BEGIN

class D3D12CPUDescriptorHeap;

struct D3D12CPUDescriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
};

class D3D12ThreadedCPUDescriptorHeap : public Uncopyable
{
public:

    ~D3D12ThreadedCPUDescriptorHeap();

    D3D12CPUDescriptor Allocate();
    void Free(const D3D12CPUDescriptor &descriptor);

private:

    friend class D3D12CPUDescriptorHeap;

    D3D12ThreadedCPUDescriptorHeap(D3D12CPUDescriptorHeap *parentHeap, uint32_t chunkSize);

    D3D12CPUDescriptorHeap *parentHeap_;
    uint32_t                chunkSize_;

    std::vector<D3D12CPUDescriptor> freeDescriptors_;
};

class D3D12CPUDescriptorHeap : public Uncopyable
{
public:

    D3D12CPUDescriptorHeap(
        ID3D12Device              *device,
        uint32_t                   nativeHeapSize,
        D3D12_DESCRIPTOR_HEAP_TYPE heapType);

    // Note: Allocate/Free are not thread-safe. Only use them when no threaded heap is alive.

    D3D12CPUDescriptor Allocate();
    void Free(const D3D12CPUDescriptor &descriptor);

    std::unique_ptr<D3D12ThreadedCPUDescriptorHeap> CreateThreadedHeap(uint32_t chunkSize);

private:

    friend class D3D12ThreadedCPUDescriptorHeap;

    void AllocateNewDescriptorHeap();
    void AllocateThreadedDescriptors(uint32_t count, std::vector<D3D12CPUDescriptor> &output);
    void FreeThreadedDescriptors(D3D12ThreadedCPUDescriptorHeap &heap);

    ID3D12Device              *device_;
    uint32_t                   nativeHeapSize_;
    D3D12_DESCRIPTOR_HEAP_TYPE heapType_;
    uint32_t                   descriptorSize_;

    std::vector<ComPtr<ID3D12DescriptorHeap>> allDescriptorHeaps_;
    std::vector<D3D12CPUDescriptor>           freeDescriptors_;

    ComPtr<ID3D12DescriptorHeap> newDescriptorHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE  firstDscriptorInNewHeap_;
    uint32_t                     nextIndexInNewDescriptorHeap_;

    tbb::spin_mutex       mutex_;
    std::atomic<uint32_t> activeThreadedDescriptorHeapCount_;
};

RTRC_RHI_D3D12_END
