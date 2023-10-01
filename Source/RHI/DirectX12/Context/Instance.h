#pragma once

#include <RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Instance, Instance)
{
public:

    explicit DirectX12Instance(DirectX12InstanceDesc desc);

    Ptr<Device> CreateDevice(const DeviceDesc &desc) RTRC_RHI_OVERRIDE;

private:

    DirectX12InstanceDesc desc_;
};

RTRC_RHI_D3D12_END
