#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Context/BackBufferSemaphore.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Queue.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Swapchain, Swapchain)
{
public:

    DirectX12Swapchain(
        DirectX12Device         *device,
        const TextureDesc       &imageDesc,
        ComPtr<IDXGISwapChain3>  swapchain,
        int                      syncInterval);

    bool Acquire() override;

    Ptr<BackBufferSemaphore> GetAcquireSemaphore() override;
    Ptr<BackBufferSemaphore> GetPresentSemaphore() override;

    bool Present() override;

    int                GetRenderTargetCount() const override;
    const TextureDesc &GetRenderTargetDesc() const override;
    Ptr<Texture>       GetRenderTarget() const override;

private:

    DirectX12Device *device_;

    ComPtr<IDXGISwapChain3> swapchain_;
    int syncInterval_;

    uint32_t imageCount_;
    uint32_t imageIndex_;
    uint32_t frameIndex_;

    TextureDesc imageDesc_;
    std::vector<Ptr<Texture>> images_;

    Ptr<DirectX12BackBufferSemaphore> acquireSemaphore_; // Dummy semaphore
    Ptr<DirectX12BackBufferSemaphore> presentSemaphore_; // Dummy semaphore. TODO: optional separate present queue
    //std::vector<Ptr<DirectX12BackBufferSemaphore>> presentSemaphores_;
};

RTRC_RHI_D3D12_END
