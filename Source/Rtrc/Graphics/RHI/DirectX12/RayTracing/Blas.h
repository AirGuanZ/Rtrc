#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Blas, Blas)
{
public:

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE
    {
        return deviceAddress_;
    }

    void _internalSetBuffer(BufferDeviceAddress deviceAddress, BufferPtr buffer)
    {
        deviceAddress_ = deviceAddress;
        buffer_ = std::move(buffer);
    }
    
private:

    BufferDeviceAddress deviceAddress_;
    BufferPtr buffer_;
};

RTRC_RHI_D3D12_END
