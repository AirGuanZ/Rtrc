#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/RayTracing/Tlas.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Tlas::~DirectX12Tlas()
{
    assert(srv_.ptr);
    device_->_internalFreeCPUDescriptorHandle_CbvSrvUav(srv_);
}

RTRC_RHI_D3D12_END
