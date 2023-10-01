#pragma once

#include <tbb/concurrent_queue.h>

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

class CPUDescriptorHandleManager : public Uncopyable
{
public:

    CPUDescriptorHandleManager(DirectX12Device *device, uint32_t chunkSize, D3D12_DESCRIPTOR_HEAP_TYPE type);
    ~CPUDescriptorHandleManager();

    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

private:

    DirectX12Device *device_;
    uint32_t         chunkSize_;

    D3D12_DESCRIPTOR_HEAP_TYPE type_;
    size_t                     incSize_;

    tbb::concurrent_queue<D3D12_CPU_DESCRIPTOR_HANDLE> freeHandles_;

    std::vector<ComPtr<ID3D12DescriptorHeap>> heaps_;
    std::mutex heapMutex_;
};

RTRC_RHI_D3D12_END
