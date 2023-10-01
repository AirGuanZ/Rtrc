#pragma once

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

struct DescriptorTable
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = {};
    uint32_t                    count     = 0;
    D3D12MA::VirtualAllocation  alloc     = {};
};

class DescriptorHeap : public Uncopyable
{
public:

    DescriptorHeap(DirectX12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool gpuVisible, uint32_t size);
    
    DescriptorHeap(DescriptorHeap &&other) noexcept;
    DescriptorHeap &operator=(DescriptorHeap &&other) noexcept;

    void Swap(DescriptorHeap &other) noexcept;

    bool Allocate(uint32_t count, DescriptorTable &range);
    void Free(DescriptorTable range);

    size_t GetHandleSize() const { return handleSize_; }
    ID3D12DescriptorHeap *GetNativeHeap() const { return heap_.Get(); }

private:

    DescriptorHeap() = default;

    ComPtr<ID3D12DescriptorHeap> heap_;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_  = {};
    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle_  = {};
    size_t                      handleSize_ = 0;

    ComPtr<D3D12MA::VirtualBlock> block_;
};

RTRC_RHI_D3D12_END
