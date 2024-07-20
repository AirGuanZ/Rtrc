#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12CommandPool, CommandPool)
{
public:

    DirectX12CommandPool(DirectX12Device *device, QueueType type, ComPtr<ID3D12CommandAllocator> allocator);

    ~DirectX12CommandPool() override;

    void Reset() RTRC_RHI_OVERRIDE;

    QueueType GetType() const RTRC_RHI_OVERRIDE;

    RPtr<CommandBuffer> NewCommandBuffer() RTRC_RHI_OVERRIDE;

    const ComPtr<ID3D12CommandAllocator> &_internalGetNativeAllocator() const { return allocator_; }

private:

    void CreateCommandBuffer();

    DirectX12Device               *device_;
    QueueType                      type_;
    ComPtr<ID3D12CommandAllocator> allocator_;

    size_t nextFreeCommandBufferIndex_;
    std::vector<ComPtr<ID3D12GraphicsCommandList10>> commandBuffers_;
};

RTRC_RHI_D3D12_END
