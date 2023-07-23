#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Resource/Buffer.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Tlas, Tlas)
{
public:

    DirectX12Tlas(
        DirectX12Device            *device,
        BufferDeviceAddress         deviceAddress,
        BufferPtr                   buffer,
        D3D12_CPU_DESCRIPTOR_HANDLE srv)
    {
        device_        = device;
        deviceAddress_ = deviceAddress;
        buffer_        = std::move(buffer);
        srv_           = srv;
    }
    
    ~DirectX12Tlas() override;

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE
    {
        return deviceAddress_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetSrv() const
    {
        return srv_;
    }

private:

    DirectX12Device            *device_;
    BufferDeviceAddress         deviceAddress_;
    BufferPtr                   buffer_;
    D3D12_CPU_DESCRIPTOR_HANDLE srv_ = {};
};

RTRC_RHI_D3D12_END
