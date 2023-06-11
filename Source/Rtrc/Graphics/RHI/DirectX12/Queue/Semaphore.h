#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Semaphore, Semaphore)
{
public:

    explicit DirectX12Semaphore(ComPtr<ID3D12Fence> fence)
        : fence_(std::move(fence))
    {

    }

    uint64_t GetValue() const RTRC_RHI_OVERRIDE
    {
        return fence_->GetCompletedValue();
    }

    const ComPtr<ID3D12Fence> &_internalGetNativeFence() const
    {
        return fence_;
    }

private:

    ComPtr<ID3D12Fence> fence_;
};

RTRC_RHI_D3D12_END
