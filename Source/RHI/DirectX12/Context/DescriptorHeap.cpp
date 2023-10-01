#include <RHI/DirectX12/Context/DescriptorHeap.h>
#include <RHI/DirectX12/Context/Device.h>

RTRC_RHI_D3D12_BEGIN

DescriptorHeap::DescriptorHeap(DirectX12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, bool gpuVisible, uint32_t size)
{
    D3D12_DESCRIPTOR_HEAP_DESC desc;
    desc.Type           = type;
    desc.NodeMask       = 0;
    desc.Flags          = gpuVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    desc.NumDescriptors = size;
    RTRC_D3D12_FAIL_MSG(
        device->_internalGetNativeDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(heap_.GetAddressOf())),
        "Fail to create directx12 descriptor heap");

    cpuHandle_ = heap_->GetCPUDescriptorHandleForHeapStart();
    gpuHandle_ = gpuVisible ? heap_->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{};
    handleSize_ = device->_internalGetNativeDevice()->GetDescriptorHandleIncrementSize(type);

    D3D12MA::VIRTUAL_BLOCK_DESC blockDesc = {};
    blockDesc.Size                 = size;
    blockDesc.pAllocationCallbacks = RTRC_D3D12_ALLOC;
    RTRC_D3D12_FAIL_MSG(
        CreateVirtualBlock(&blockDesc, block_.GetAddressOf()),
        "Fail to create D3D12MA virtual block");
}

DescriptorHeap::DescriptorHeap(DescriptorHeap &&other) noexcept
    : DescriptorHeap()
{
    Swap(other);
}

DescriptorHeap &DescriptorHeap::operator=(DescriptorHeap &&other) noexcept
{
    Swap(other);
    return *this;
}

void DescriptorHeap::Swap(DescriptorHeap &other) noexcept
{
    std::swap(heap_, other.heap_);
    std::swap(cpuHandle_, other.cpuHandle_);
    std::swap(gpuHandle_, other.gpuHandle_);
    std::swap(handleSize_, other.handleSize_);
    std::swap(block_, other.block_);
}

bool DescriptorHeap::Allocate(uint32_t count, DescriptorTable &range)
{
    UINT64 offset;
    const D3D12MA::VIRTUAL_ALLOCATION_DESC desc = { .Size = count };
    if(block_->Allocate(&desc, &range.alloc, &offset) != S_OK)
    {
        return false;
    }
    range.count = count;
    range.cpuHandle = { cpuHandle_.ptr + handleSize_ * offset };
    range.gpuHandle = { gpuHandle_.ptr + handleSize_ * offset };
    return true;
}

void DescriptorHeap::Free(DescriptorTable range)
{
    block_->FreeAllocation(range.alloc);
}

RTRC_RHI_D3D12_END
