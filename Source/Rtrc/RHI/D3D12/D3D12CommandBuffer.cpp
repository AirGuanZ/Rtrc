#include <Rtrc/RHI/D3D12/D3D12CommandBuffer.h>

RTRC_RHI_D3D12_BEGIN

D3D12CommandBuffer::D3D12CommandBuffer(
    ComPtr<ID3D12GraphicsCommandList> commandList,
    ObserverPtr<D3D12CommandAllocator> allocator)
    : commandList_(std::move(commandList))
    , allocator_(allocator)
{
    
}

D3D12CommandAllocator *D3D12CommandBuffer::_internalGetCommandAllocator()
{
    return allocator_.Get();
}

ID3D12GraphicsCommandList *D3D12CommandBuffer::_internalGetCommandList()
{
    return commandList_.Get();
}

RTRC_RHI_D3D12_END
