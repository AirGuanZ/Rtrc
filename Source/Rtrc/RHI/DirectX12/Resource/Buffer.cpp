#include <ranges>
#include <shared_mutex>

#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/RHI/DirectX12/Resource/BufferView.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Buffer::DirectX12Buffer(
    const BufferDesc         &desc,
    DirectX12Device          *device,
    ComPtr<ID3D12Resource>    buffer,
    DirectX12MemoryAllocation alloc)
    : desc_(desc), device_(device), alloc_(std::move(alloc)), buffer_(std::move(buffer))
{
    deviceAddress_ = buffer_->GetGPUVirtualAddress();
}

DirectX12Buffer::~DirectX12Buffer()
{
    for(auto &v : std::views::values(srvs_))
    {
        device_->_internalFreeCPUDescriptorHandle_CbvSrvUav(v);
    }
    for(auto &v : std::views::values(uavs_))
    {
        device_->_internalFreeCPUDescriptorHandle_CbvSrvUav(v);
    }
}

const BufferDesc &DirectX12Buffer::GetDesc() const
{
    return desc_;
}

RPtr<BufferSrv> DirectX12Buffer::CreateSrv(const BufferSrvDesc &desc) const
{
    assert(desc.stride == 0 || desc.offset % desc.stride == 0);

    {
        std::shared_lock lock(viewMutex_);
        if(auto it = srvs_.find(desc); it != srvs_.end())
        {
            return MakeRPtr<DirectX12BufferSrv>(this, desc, it->second);
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = srvs_.find(desc); it != srvs_.end())
    {
        return MakeRPtr<DirectX12BufferSrv>(this, desc, it->second);
    }

    const bool isRawBufferView = desc.format == Format::Unknown && desc.stride == 0;
    const size_t actualStride = desc.stride != 0 ? desc.stride : (isRawBufferView ? 4 : GetTexelSize(desc.format));

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
    viewDesc.Format                     = isRawBufferView ? DXGI_FORMAT_R32_TYPELESS : TranslateFormat(desc.format);
    viewDesc.Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    viewDesc.ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
    viewDesc.Buffer.FirstElement        = desc.offset / actualStride;
    viewDesc.Buffer.NumElements         = desc.range / actualStride;
    viewDesc.Buffer.StructureByteStride = desc.stride;
    viewDesc.Buffer.Flags               = isRawBufferView ? D3D12_BUFFER_SRV_FLAG_RAW : D3D12_BUFFER_SRV_FLAG_NONE;

    auto handle = device_->_internalAllocateCPUDescriptorHandle_CbvSrvUav();
    device_->_internalGetNativeDevice()->CreateShaderResourceView(buffer_.Get(), &viewDesc, handle);
    srvs_.insert({ desc, handle });
    return MakeRPtr<DirectX12BufferSrv>(this, desc, handle);
}

RPtr<BufferUav> DirectX12Buffer::CreateUav(const BufferUavDesc &desc) const
{
    assert((desc.stride != 0) != (desc.format != Format::Unknown));
    assert(desc.stride == 0 || desc.offset % desc.stride == 0);

    {
        std::shared_lock lock(viewMutex_);
        if(auto it = uavs_.find(desc); it != uavs_.end())
        {
            return MakeRPtr<DirectX12BufferUav>(this, desc, it->second);
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = uavs_.find(desc); it != uavs_.end())
    {
        return MakeRPtr<DirectX12BufferUav>(this, desc, it->second);
    }
    
    const bool isRawBufferView = desc.format == Format::Unknown && desc.stride == 0;
    const size_t actualStride = desc.stride != 0 ? desc.stride : (isRawBufferView ? 4 : GetTexelSize(desc.format));

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    viewDesc.Format                      = isRawBufferView ? DXGI_FORMAT_R32_TYPELESS : TranslateFormat(desc.format);
    viewDesc.ViewDimension               = D3D12_UAV_DIMENSION_BUFFER;
    viewDesc.Buffer.FirstElement         = desc.offset / actualStride;
    viewDesc.Buffer.NumElements          = desc.range / actualStride;
    viewDesc.Buffer.StructureByteStride  = desc.stride;
    viewDesc.Buffer.Flags                = isRawBufferView ? D3D12_BUFFER_UAV_FLAG_RAW : D3D12_BUFFER_UAV_FLAG_NONE;;
    viewDesc.Buffer.CounterOffsetInBytes = 0;

    auto handle = device_->_internalAllocateCPUDescriptorHandle_CbvSrvUav();
    device_->_internalGetNativeDevice()->CreateUnorderedAccessView(buffer_.Get(), nullptr, &viewDesc, handle);
    uavs_.insert({ desc, handle });
    return MakeRPtr<DirectX12BufferUav>(this, desc, handle);
}

void *DirectX12Buffer::Map(size_t offset, size_t size, const BufferReadRange &range, bool invalidate)
{
    assert(alloc_.allocation);

    D3D12_RANGE readRange, *pReadRange;
    if(range == READ_WHOLE_BUFFER)
    {
        pReadRange = nullptr;
    }
    else
    {
        pReadRange = &readRange;
        if(range.size == 0 || desc_.hostAccessType != BufferHostAccessType::Readback)
        {
            readRange = { 0, 0 };
        }
        else
        {
            readRange = { range.offset, range.offset + range.size };
        }
    }

    void *result;
    RTRC_D3D12_FAIL_MSG(
        buffer_->Map(0, pReadRange, &result),
        fmt::format("Fail to map directx12 buffer {}", GetName()));

    return result;
}

void DirectX12Buffer::Unmap(size_t offset, size_t size, bool flush)
{
    D3D12_RANGE writtenRange;
    if(desc_.hostAccessType == BufferHostAccessType::Upload)
    {
        writtenRange = { offset, offset + size };
    }
    else
    {
        writtenRange = { offset, offset };
    }
    buffer_->Unmap(0, &writtenRange);
}

void DirectX12Buffer::InvalidateBeforeRead(size_t offset, size_t size)
{
    // On current gpus for pc, 'HOST_VISIBLE' memories are also 'HOST_COHERENT'.
    // So we don't need to do anything here.
}

void DirectX12Buffer::FlushAfterWrite(size_t offset, size_t size)
{
    // Same as InvalidateBeforeRead
}

BufferDeviceAddress DirectX12Buffer::GetDeviceAddress() const
{
    return { deviceAddress_ };
}

RTRC_RHI_D3D12_END
