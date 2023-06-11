#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Swapchain.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Texture.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Swapchain::DirectX12Swapchain(
    DirectX12Device        *device,
    const TextureDesc      &imageDesc,
    ComPtr<IDXGISwapChain3> swapchain,
    int                     syncInterval)
    : device_(device)
    , swapchain_(std::move(swapchain))
    , syncInterval_(syncInterval)
    , imageCount_(0)
    , frameIndex_(0)
    , imageDesc_(imageDesc)
{
    DXGI_SWAP_CHAIN_DESC swapchainDesc;
    swapchain_->GetDesc(&swapchainDesc);
    imageCount_ = swapchainDesc.BufferCount;

    acquireSemaphore_ = MakePtr<DirectX12BackBufferSemaphore>();
    presentSemaphore_ = acquireSemaphore_;
    //acquireSemaphore_->type = DirectX12BackBufferSemaphore::Acquire;
    //acquireSemaphore_->fenceValue = 0;

    //presentSemaphores_.resize(imageCount_);
    //for(auto &s : presentSemaphores_)
    //{
    //    s = MakePtr<DirectX12BackBufferSemaphore>();
    //    s->type = DirectX12BackBufferSemaphore::Present;
    //    s->fenceValue = 0;
    //    RTRC_D3D12_FAIL_MSG(
    //        device_->_internalGetNativeDevice()->CreateFence(
    //            0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(s->fence.GetAddressOf())),
    //        "Fail to create directx12 fence for swapchain image presentation");
    //}

    images_.resize(imageCount_);
    for(uint32_t i = 0; i < imageCount_; ++i)
    {
        ComPtr<ID3D12Resource> image;
        RTRC_D3D12_FAIL_MSG(
            swapchain_->GetBuffer(i, IID_PPV_ARGS(image.GetAddressOf())),
            "Fail to get directx12 swapchain image");
        images_[i] = MakePtr<DirectX12Texture>(
            imageDesc, device_, std::move(image), DirectX12MemoryAllocation{ nullptr, {} });
        images_[i]->SetName(fmt::format("SwapChainImage{}", i));
    }

    imageIndex_ = swapchain_->GetCurrentBackBufferIndex();
}

bool DirectX12Swapchain::Acquire()
{
    frameIndex_ = (frameIndex_ + 1) % imageCount_;
    imageIndex_ = swapchain_->GetCurrentBackBufferIndex();
    return true;
}

Ptr<BackBufferSemaphore> DirectX12Swapchain::GetAcquireSemaphore()
{
    return acquireSemaphore_;
}

Ptr<BackBufferSemaphore> DirectX12Swapchain::GetPresentSemaphore()
{
    return acquireSemaphore_;
    //return presentSemaphores_[frameIndex_];
}

bool DirectX12Swapchain::Present()
{
    const HRESULT hr = swapchain_->Present(syncInterval_, 0);
    return SUCCEEDED(hr);
}

int DirectX12Swapchain::GetRenderTargetCount() const
{
    return static_cast<int>(imageCount_);
}

const TextureDesc &DirectX12Swapchain::GetRenderTargetDesc() const
{
    return imageDesc_;
}

Ptr<Texture> DirectX12Swapchain::GetRenderTarget() const
{
    return images_[imageIndex_];
}

RTRC_RHI_D3D12_END
