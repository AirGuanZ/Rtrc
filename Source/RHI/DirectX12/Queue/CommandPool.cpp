#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Queue/CommandBuffer.h>
#include <RHI/DirectX12/Queue/CommandPool.h>
#include <Core/Unreachable.h>

RTRC_RHI_D3D12_BEGIN

DirectX12CommandPool::DirectX12CommandPool(
    DirectX12Device               *device,
    QueueType                      type,
    ComPtr<ID3D12CommandAllocator> allocator)
    : device_(device), type_(type), allocator_(std::move(allocator)), nextFreeCommandBufferIndex_(0)
{
    
}

DirectX12CommandPool::~DirectX12CommandPool()
{
    commandBuffers_.clear();
}

void DirectX12CommandPool::Reset()
{
    RTRC_D3D12_FAIL_MSG(allocator_->Reset(), "Fail to reset directx12 command pool");
}

QueueType DirectX12CommandPool::GetType() const
{
    return type_;
}

Ptr<CommandBuffer> DirectX12CommandPool::NewCommandBuffer()
{
    if(nextFreeCommandBufferIndex_ >= commandBuffers_.size())
    {
        CreateCommandBuffer();
    }
    auto ret = commandBuffers_[nextFreeCommandBufferIndex_++];
    return MakePtr<DirectX12CommandBuffer>(device_, this, std::move(ret));
}

void DirectX12CommandPool::CreateCommandBuffer()
{
    const D3D12_COMMAND_LIST_TYPE type = TranslateCommandListType(type_);
    ComPtr<ID3D12GraphicsCommandList7> commandList;
    RTRC_D3D12_FAIL_MSG(
        device_->_internalGetNativeDevice()->CreateCommandList(
            0, type, allocator_.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf())),
        "Fail to create directx12 command list");
    commandList->Close();
    commandBuffers_.push_back(std::move(commandList));
}

RTRC_RHI_D3D12_END
