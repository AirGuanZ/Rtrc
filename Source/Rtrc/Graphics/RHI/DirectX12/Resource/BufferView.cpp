#include <Rtrc/Graphics/RHI/DirectX12/Resource/BufferView.h>

RTRC_RHI_D3D12_BEGIN

DirectX12BufferSrv::DirectX12BufferSrv(
    const DirectX12Buffer *buffer, const BufferSrvDesc &desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
    : buffer_(buffer), desc_(desc), handle_(handle)
{
    
}

const BufferSrvDesc &DirectX12BufferSrv::GetDesc() const
{
    return desc_;
}

const DirectX12Buffer *DirectX12BufferSrv::_internalGetBuffer() const
{
    return buffer_;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12BufferSrv::_internalGetDescriptorHandle() const
{
    return handle_;
}

DirectX12BufferUav::DirectX12BufferUav(
    const DirectX12Buffer *buffer, const BufferUavDesc &desc, D3D12_CPU_DESCRIPTOR_HANDLE handle)
    : buffer_(buffer), desc_(desc), handle_(handle)
{
    
}

const BufferUavDesc &DirectX12BufferUav::GetDesc() const
{
    return desc_;
}

const DirectX12Buffer *DirectX12BufferUav::_internalGetBuffer() const
{
    return buffer_;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12BufferUav::_internalGetDescriptorHandle() const
{
    return handle_;
}

RTRC_RHI_D3D12_END
