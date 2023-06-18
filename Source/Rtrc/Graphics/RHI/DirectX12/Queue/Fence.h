#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Fence, Fence)
{
public:

    RTRC_D3D12_IMPL_SET_NAME(fence_)

    explicit DirectX12Fence(ComPtr<ID3D12Fence> fence, bool signaled)
        : fence_(std::move(fence))
    {
        signalValue_ = signaled ? 0 : 1;
    }

    void Reset() RTRC_RHI_OVERRIDE
    {
        ++signalValue_;
    }

    void Wait() RTRC_RHI_OVERRIDE;

    const ComPtr<ID3D12Fence> &_internalGetNativeFence() const
    {
        return fence_;
    }

    uint64_t GetSignalValue() const
    {
        return signalValue_;
    }

private:
    
    ComPtr<ID3D12Fence> fence_;
    uint64_t            signalValue_;
};

RTRC_RHI_D3D12_END
