#pragma once

#include <RHI/DirectX12/Resource/Buffer.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Blas, Blas)
{
public:

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE
    {
        return deviceAddress_;
    }

    void _internalSetBuffer(BufferDeviceAddress deviceAddress, BufferRPtr buffer)
    {
        deviceAddress_ = deviceAddress;
        buffer_ = std::move(buffer);
    }

    BufferOPtr _internalGetBuffer() const
    {
        return buffer_;
    }
    
private:

    BufferDeviceAddress deviceAddress_;
    BufferRPtr buffer_;
};

RTRC_RHI_D3D12_END
