#include <Rtrc/RHI/D3D12/D3D12Device.h>

RTRC_RHI_BEGIN

DeviceRef Device::Create(const DeviceDesc &desc)
{
    return D3D12::D3D12Device::Create(desc);
}

RTRC_RHI_END
