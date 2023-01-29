#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Buffer, Buffer)
{
public:

    DirectX12Buffer(
        const BufferDesc         &desc,
        ID3D12Resource           *buffer,
        DirectX12MemoryAllocation alloc,
        ResourceOwnership         ownership);

    ~DirectX12Buffer() override;

    const BufferDesc &GetDesc() const RTRC_RHI_OVERRIDE;
};

RTRC_RHI_D3D12_END
