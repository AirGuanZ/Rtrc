#include <ranges>
#include <shared_mutex>

#include <Rtrc/RHI/D3D12/D3D12Buffer.h>
#include <Rtrc/RHI/D3D12/D3D12Device.h>

RTRC_RHI_D3D12_BEGIN

D3D12Buffer::D3D12Buffer(
    ObserverPtr<D3D12Device> device,
    ComPtr<ID3D12Resource>   resource,
    D3D12MemoryAllocation    allocation,
    const BufferDesc        &desc)
    : device_(device)
    , allocation_(std::move(allocation))
    , resource_(std::move(resource))
    , desc_(desc)
{
    
}

D3D12Buffer::~D3D12Buffer()
{
    for(auto &view : std::views::values(SRVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, view->GetDescriptor());
    }
    for(auto &view : std::views::values(UAVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, view->GetDescriptor());
    }
}

const BufferDesc &D3D12Buffer::GetDesc() const
{
    return desc_;
}

BufferSRVRef D3D12Buffer::GetSRV(const BufferSRVDesc &desc)
{
    {
        std::shared_lock lock(viewMapLock_);
        if(auto it = SRVMap.find(desc); it != SRVMap.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewMapLock_);
    if(auto it = SRVMap.find(desc); it != SRVMap.end())
    {
        return it->second;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC d3d12Desc = {};
    d3d12Desc.ViewDimension           = D3D12_SRV_DIMENSION_BUFFER;
    d3d12Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    d3d12Desc.Buffer.FirstElement     = desc.offset;
    d3d12Desc.Buffer.NumElements      = desc.count;

    if(desc.format != Format::Unknown) // typed buffer
    {
        assert(!desc.stride);
        d3d12Desc.Format = FormatToD3D12BufferFormat(desc.format);
    }
    else if(desc.stride) // structured buffer
    {
        d3d12Desc.Buffer.StructureByteStride = desc.stride;
    }
    else // raw buffer
    {
        d3d12Desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device_->GetNativeDevice()->CreateShaderResourceView(resource_.Get(), &d3d12Desc, descriptor.handle);

    auto SRV = MakeReferenceCountedPtr<D3D12BufferSRV>(this, descriptor);
    SRVMap.insert({ desc, SRV });
    return SRV;
}

BufferUAVRef D3D12Buffer::GetUAV(const BufferUAVDesc &desc)
{
    {
        std::shared_lock lock(viewMapLock_);
        if(auto it = UAVMap.find(desc); it != UAVMap.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewMapLock_);
    if(auto it = UAVMap.find(desc); it != UAVMap.end())
    {
        return it->second;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC d3d12Desc = {};
    d3d12Desc.ViewDimension           = D3D12_UAV_DIMENSION_BUFFER;
    d3d12Desc.Buffer.FirstElement     = desc.offset;
    d3d12Desc.Buffer.NumElements      = desc.count;

    if(desc.format != Format::Unknown) // typed buffer
    {
        assert(!desc.stride);
        d3d12Desc.Format = FormatToD3D12BufferFormat(desc.format);
    }
    else if(desc.stride) // structured buffer
    {
        d3d12Desc.Buffer.StructureByteStride = desc.stride;
    }
    else // raw buffer
    {
        d3d12Desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device_->GetNativeDevice()->CreateUnorderedAccessView(resource_.Get(), nullptr, &d3d12Desc, descriptor.handle);

    auto UAV = MakeReferenceCountedPtr<D3D12BufferUAV>(this, descriptor);
    UAVMap.insert({ desc, UAV });
    return UAV;
}

RTRC_RHI_D3D12_END
