#pragma once

#include <map>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Buffer, Buffer)
{
public:

    RTRC_D3D12_IMPL_SET_NAME(buffer_)

    DirectX12Buffer(
        const BufferDesc         &desc,
        DirectX12Device          *device,
        ComPtr<ID3D12Resource>    buffer,
        DirectX12MemoryAllocation alloc);

    ~DirectX12Buffer() override;

    const BufferDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    Ptr<BufferSrv> CreateSrv(const BufferSrvDesc &desc) const RTRC_RHI_OVERRIDE;
    Ptr<BufferUav> CreateUav(const BufferUavDesc &desc) const RTRC_RHI_OVERRIDE;

    void *Map(size_t offset, size_t size, const BufferReadRange &readRange, bool invalidate = false) RTRC_RHI_OVERRIDE;
    void Unmap(size_t offset, size_t size, bool flush = false) RTRC_RHI_OVERRIDE;

    void InvalidateBeforeRead(size_t offset, size_t size) RTRC_RHI_OVERRIDE;
    void FlushAfterWrite(size_t offset, size_t size) RTRC_RHI_OVERRIDE;

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE;

    const ComPtr<ID3D12Resource> &_internalGetNativeBuffer() const { return buffer_; }

private:

    BufferDesc       desc_;
    DirectX12Device *device_;

    DirectX12MemoryAllocation alloc_;
    ComPtr<ID3D12Resource>    buffer_;

    D3D12_GPU_VIRTUAL_ADDRESS deviceAddress_;

    mutable std::map<BufferSrvDesc, D3D12_CPU_DESCRIPTOR_HANDLE> srvs_;
    mutable std::map<BufferSrvDesc, D3D12_CPU_DESCRIPTOR_HANDLE> uavs_;
    mutable tbb::spin_rw_mutex viewMutex_;
};

RTRC_RHI_D3D12_END
