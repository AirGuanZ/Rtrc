#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/RHI/D3D12/D3D12Common.h>

RTRC_RHI_D3D12_BEGIN

class D3D12CommandAllocator;

class D3D12CommandBuffer : public CommandBuffer
{
public:

    D3D12CommandBuffer(ComPtr<ID3D12GraphicsCommandList> commandList, ObserverPtr<D3D12CommandAllocator> allocator);

    D3D12CommandAllocator *_internalGetCommandAllocator();
    ID3D12GraphicsCommandList *_internalGetCommandList();

private:

    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ObserverPtr<D3D12CommandAllocator> allocator_;
};

RTRC_RHI_D3D12_END
