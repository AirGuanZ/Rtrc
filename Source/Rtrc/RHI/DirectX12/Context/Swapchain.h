#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Swapchain : public Swapchain
{
public:

    DirectX12Swapchain(const SwapchainDesc &desc, ComPtr<IDXGISwapChain> swapchain);

    bool Acquire() override;

    RC<BackBufferSemaphore> GetAcquireSemaphore() override;

    RC<BackBufferSemaphore> GetPresentSemaphore() override;

    void Present() override;

    RC<Texture> GetRenderTarget() const override;

    const Texture2DDesc &GetRenderTargetDesc() const override;

    int GetRenderTargetCount() const override;

private:

    SwapchainDesc desc_;
    ComPtr<IDXGISwapChain> swapchain_;
};

RTRC_RHI_DIRECTX12_END