#include <RHI/DirectX12/Context/CPUDescriptorHandleManager.h>
#include <RHI/DirectX12/Context/Device.h>

RTRC_RHI_D3D12_BEGIN

CPUDescriptorHandleManager::CPUDescriptorHandleManager(
    DirectX12Device *device, uint32_t chunkSize, D3D12_DESCRIPTOR_HEAP_TYPE type)
    : device_(device), chunkSize_(chunkSize), type_(type)
{
    assert(chunkSize > 0);
    incSize_ = device->_internalGetNativeDevice()->GetDescriptorHandleIncrementSize(type);
}

CPUDescriptorHandleManager::~CPUDescriptorHandleManager()
{
    assert(heaps_.size() * chunkSize_ == freeHandles_.unsafe_size());
}

D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHandleManager::Allocate()
{
    if(D3D12_CPU_DESCRIPTOR_HANDLE handle; freeHandles_.try_pop(handle))
    {
        return handle;
    }

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
    heapDesc.Type           = type_;
    heapDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask       = 0;
    heapDesc.NumDescriptors = chunkSize_;

    ComPtr<ID3D12DescriptorHeap> heap;
    RTRC_D3D12_FAIL_MSG(
        device_->_internalGetNativeDevice()->CreateDescriptorHeap(
            &heapDesc, IID_PPV_ARGS(heap.GetAddressOf())),
        "Fail to create directx12 cpu descriptor heap");

    {
        std::lock_guard lock(heapMutex_);
        heaps_.push_back(heap);
    }

    const auto start = heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE handle = start;
    for(uint32_t i = 1; i < chunkSize_; ++i)
    {
        handle.ptr += incSize_;
        freeHandles_.push(handle);
    }

    return start;
}

void CPUDescriptorHandleManager::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    freeHandles_.push(handle);
}

RTRC_RHI_D3D12_END
