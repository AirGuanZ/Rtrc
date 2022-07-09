#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Instance : public Instance
{
public:

    Ptr<Device> CreateDevice(const DeviceDesc &desc) override;
};

RTRC_RHI_DIRECTX12_END
