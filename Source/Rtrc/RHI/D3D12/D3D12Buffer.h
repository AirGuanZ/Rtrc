#pragma once

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/RHI/D3D12/D3D12DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

class D3D12Device;
class D3D12Buffer;

template<typename Base>
class D3D12BufferView : public Base
{
public:

    D3D12BufferView(ObserverPtr<D3D12Buffer> buffer, const D3D12CPUDescriptor &descriptor)
        : buffer_(buffer), descriptor_(descriptor)
    {
        
    }

    const D3D12CPUDescriptor &GetDescriptor() const { return descriptor_; }

private:

    ObserverPtr<D3D12Buffer> buffer_;
    D3D12CPUDescriptor descriptor_;
};

using D3D12BufferSRV = D3D12BufferView<BufferSRV>;
using D3D12BufferUAV = D3D12BufferView<BufferUAV>;

class D3D12Buffer : public Buffer
{
public:

    D3D12Buffer(
        ObserverPtr<D3D12Device> device,
        ComPtr<ID3D12Resource> resource,
        D3D12MemoryAllocation allocation,
        const BufferDesc &desc);
    ~D3D12Buffer() RTRC_RHI_OVERRIDE;

    const BufferDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    BufferSRVRef GetSRV(const BufferSRVDesc &desc) RTRC_RHI_OVERRIDE;
    BufferUAVRef GetUAV(const BufferUAVDesc &desc) RTRC_RHI_OVERRIDE;

private:

    ObserverPtr<D3D12Device> device_;
    D3D12MemoryAllocation    allocation_;
    ComPtr<ID3D12Resource>   resource_;
    BufferDesc               desc_;

    tbb::spin_rw_mutex viewMapLock_;
    std::map<BufferSRVDesc, ReferenceCountedPtr<D3D12BufferSRV>> SRVMap;
    std::map<BufferUAVDesc, ReferenceCountedPtr<D3D12BufferUAV>> UAVMap;
};

RTRC_RHI_D3D12_END
