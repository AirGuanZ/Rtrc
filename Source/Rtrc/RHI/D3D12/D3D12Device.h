#pragma once

#include <tbb/enumerable_thread_specific.h>

#include <Rtrc/RHI/D3D12/D3D12DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

class D3D12Device : public Device
{
public:

    ReferenceCountedPtr<D3D12Device> Create(const DeviceDesc &desc);

    void BeginThreadedMode() RTRC_RHI_OVERRIDE;
    void EndThreadedMode() RTRC_RHI_OVERRIDE;

    QueueRef GetQueue(QueueType queue) RTRC_RHI_OVERRIDE;
    TextureRef GetSwapchainImage() RTRC_RHI_OVERRIDE;
    bool Present() RTRC_RHI_OVERRIDE;

    TextureRef CreateTexture(const TextureDesc &desc) RTRC_RHI_OVERRIDE;
    BufferRef CreateBuffer(const BufferDesc &desc) RTRC_RHI_OVERRIDE;

    FenceRef NewSemaphore() RTRC_RHI_OVERRIDE;

    D3D12CPUDescriptor AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type);
    void FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE type, const D3D12CPUDescriptor &descriptor);

    ID3D12Device *GetNativeDevice();

private:

    struct ThreadContext
    {
        std::unique_ptr<D3D12ThreadedCPUDescriptorHeap> CPUDescriptorHeap;
    };

    ComPtr<ID3D12Device>    device_;
    ComPtr<IDXGISwapChain3> swapchain_;
    std::vector<TextureRef> swapchainImages_;
    uint32_t                swapchainSyncInterval_      = 0;
    uint32_t                currentSwapchainImageIndex_ = 0;

    ComPtr<D3D12MA::Allocator> allocator_;

    QueueRef graphicsQueue_;
    QueueRef presentQueue_;
    QueueRef computeQueue_;
    QueueRef copyQueue_;

    std::unique_ptr<D3D12CPUDescriptorHeap> CPUDescriptorHeaps_[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
    
    bool isInThreadedMode_ = false;
    tbb::enumerable_thread_specific<ThreadContext> threadContexts_;
};

RTRC_RHI_D3D12_END
