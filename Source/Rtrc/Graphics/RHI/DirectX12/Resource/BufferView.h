#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Resource/Buffer.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12BufferSrv, BufferSrv)
{
public:

    DirectX12BufferSrv(const DirectX12Buffer * buffer, const BufferSrvDesc & desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);

    const BufferSrvDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    const DirectX12Buffer *_internalGetBuffer() const;

    D3D12_CPU_DESCRIPTOR_HANDLE _internalGetDescriptorHandle() const;

private:

    const DirectX12Buffer *buffer_;
    BufferSrvDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE handle_;
};

RTRC_RHI_IMPLEMENT(DirectX12BufferUav, BufferUav)
{
public:

    DirectX12BufferUav(const DirectX12Buffer *buffer, const BufferUavDesc &desc, D3D12_CPU_DESCRIPTOR_HANDLE handle);

    const BufferUavDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    const DirectX12Buffer *_internalGetBuffer() const;

    D3D12_CPU_DESCRIPTOR_HANDLE _internalGetDescriptorHandle() const;

private:

    const DirectX12Buffer *buffer_;
    BufferUavDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE handle_;
};

RTRC_RHI_D3D12_END
