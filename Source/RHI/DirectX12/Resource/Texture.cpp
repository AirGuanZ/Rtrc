#include <shared_mutex>
#include <ranges>

#include <RHI/DirectX12/Context/Device.h>
#include <RHI/DirectX12/Resource/Texture.h>
#include <RHI/DirectX12/Resource/TextureView.h>

RTRC_RHI_D3D12_BEGIN

DirectX12Texture::DirectX12Texture(
    const TextureDesc        &desc,
    DirectX12Device          *device,
    ComPtr<ID3D12Resource>    texture,
    DirectX12MemoryAllocation alloc)
    : desc_(desc), device_(device), alloc_(std::move(alloc)), texture_(std::move(texture))
{

}

DirectX12Texture::~DirectX12Texture()
{
    for(auto &srv : std::ranges::views::values(srvs_))
    {
        device_->_internalFreeCPUDescriptorHandle_CbvSrvUav(srv);
    }
    for(auto &uav : std::ranges::views::values(uavs_))
    {
        device_->_internalFreeCPUDescriptorHandle_CbvSrvUav(uav);
    }
    for(auto &rtv : std::ranges::views::values(rtvs_))
    {
        device_->_internalFreeCPUDescriptorHandle_Rtv(rtv);
    }
    for(auto &dsv : std::ranges::views::values(dsvs_))
    {
        device_->_internalFreeCPUDescriptorHandle_Dsv(dsv);
    }
}

RPtr<TextureRtv> DirectX12Texture::CreateRtv(const TextureRtvDesc &desc) const
{
    {
        std::shared_lock lock(viewMutex_);
        if(auto it = rtvs_.find(desc); it != rtvs_.end())
        {
            return RPtr<TextureRtv>(new DirectX12TextureRtv(this, desc, it->second));
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = rtvs_.find(desc); it != rtvs_.end())
    {
        return RPtr<TextureRtv>(new DirectX12TextureRtv(this, desc, it->second));
    }
    
    D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
    viewDesc.Format = TranslateFormat(desc.format != Format::Unknown ? desc.format : desc_.format);
    if(desc_.dim == TextureDimension::Tex2D)
    {
        if(desc_.sampleCount == 1)
        {
            if(desc_.arraySize == 1)
            {
                viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MipSlice = desc.mipLevel;
                viewDesc.Texture2D.PlaneSlice = 0;
            }
            else
            {
                viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MipSlice        = desc.mipLevel;
                viewDesc.Texture2DArray.PlaneSlice      = 0;
                viewDesc.Texture2DArray.FirstArraySlice = desc.arrayLayer;
                viewDesc.Texture2DArray.ArraySize       = 1;
            }
        }
        else
        {
            if(desc_.arraySize == 1)
            {
                viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
                viewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                // d3d12 doesn't support non-zero mipmap level in multi-sampled texture2d array rtv
                assert(desc.mipLevel == 0);
                viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
                viewDesc.Texture2DMSArray.FirstArraySlice = desc.arrayLayer;
                viewDesc.Texture2DMSArray.ArraySize       = 1;
            }
        }
    }
    else
    {
        assert(desc_.dim == TextureDimension::Tex3D);
        assert(desc_.sampleCount == 1);
        viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
        viewDesc.Texture3D.MipSlice    = desc.mipLevel;
        viewDesc.Texture3D.FirstWSlice = 0;
        viewDesc.Texture3D.WSize       = desc_.depth;
    }

    auto handle = device_->_internalAllocateCPUDescriptorHandle_Rtv();
    device_->_internalGetNativeDevice()->CreateRenderTargetView(texture_.Get(), &viewDesc, handle);
    rtvs_.insert({ desc, handle });
    return MakeRPtr<DirectX12TextureRtv>(this, desc, handle);
}

RPtr<TextureSrv> DirectX12Texture::CreateSrv(const TextureSrvDesc &desc) const
{
    {
        std::shared_lock lock(viewMutex_);
        if(auto it = srvs_.find(desc); it != srvs_.end())
        {
            return RPtr<TextureSrv>(new DirectX12TextureSrv(desc, it->second));
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = srvs_.find(desc); it != srvs_.end())
    {
        return RPtr<TextureSrv>(new DirectX12TextureSrv(desc, it->second));
    }

    uint32_t mipLevels = desc.levelCount;
    if(!mipLevels)
    {
        mipLevels = desc_.mipLevels - desc.baseMipLevel;
    }

    uint32_t arraySize = desc.layerCount;
    if(!arraySize && desc_.dim != TextureDimension::Tex3D)
    {
        arraySize = desc_.arraySize - desc.baseArrayLayer;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc;
    viewDesc.Format = TranslateFormat(desc.format != Format::Unknown ? desc.format : desc_.format);
    if(viewDesc.Format == DXGI_FORMAT_D24_UNORM_S8_UINT)
    {
        viewDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    }
    else if(viewDesc.Format == DXGI_FORMAT_D32_FLOAT)
    {
        viewDesc.Format = DXGI_FORMAT_R32_FLOAT;
    }
    viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    if(desc_.dim == TextureDimension::Tex2D)
    {
        if(desc_.sampleCount == 1)
        {
            if(desc.isArray)
            {
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MostDetailedMip     = desc.baseMipLevel;
                viewDesc.Texture2DArray.MipLevels           = mipLevels;
                viewDesc.Texture2DArray.FirstArraySlice     = desc.baseArrayLayer;
                viewDesc.Texture2DArray.ArraySize           = arraySize;
                viewDesc.Texture2DArray.PlaneSlice          = 0;
                viewDesc.Texture2DArray.ResourceMinLODClamp = 0;
            }
            else
            {
                assert(desc.baseArrayLayer == 0);
                assert(arraySize == 1);
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MostDetailedMip     = desc.baseMipLevel;
                viewDesc.Texture2D.MipLevels           = mipLevels;
                viewDesc.Texture2D.PlaneSlice          = 0;
                viewDesc.Texture2D.ResourceMinLODClamp = 0;
            }
        }
        else
        {
            if(desc.isArray)
            {
                assert(!desc.baseMipLevel);
                assert(mipLevels == desc_.mipLevels);
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                viewDesc.Texture2DMSArray.FirstArraySlice = desc.baseArrayLayer;
                viewDesc.Texture2DMSArray.ArraySize       = arraySize;
            }
            else
            {
                assert(!desc.baseMipLevel);
                assert(mipLevels == desc_.mipLevels);
                assert(desc.baseArrayLayer == 0);
                assert(arraySize == 1);
                viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                viewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
        }
    }
    else
    {
        assert(desc_.dim == TextureDimension::Tex3D);
        assert(desc_.sampleCount == 1);
        assert(desc.baseArrayLayer == 0);
        assert(arraySize == 1);
        viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
        viewDesc.Texture3D.MostDetailedMip     = desc.baseMipLevel;
        viewDesc.Texture3D.MipLevels           = mipLevels;
        viewDesc.Texture3D.ResourceMinLODClamp = 0;
    }

    auto handle = device_->_internalAllocateCPUDescriptorHandle_CbvSrvUav();
    device_->_internalGetNativeDevice()->CreateShaderResourceView(texture_.Get(), &viewDesc, handle);
    srvs_.insert({ desc, handle });
    return MakeRPtr<DirectX12TextureSrv>(desc, handle);
}

RPtr<TextureUav> DirectX12Texture::CreateUav(const TextureUavDesc &desc) const
{
    {
        std::shared_lock lock(viewMutex_);
        if(auto it = uavs_.find(desc); it != uavs_.end())
        {
            return RPtr<TextureUav>(new DirectX12TextureUav(desc, it->second));
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = uavs_.find(desc); it != uavs_.end())
    {
        return RPtr<TextureUav>(new DirectX12TextureUav(desc, it->second));
    }

    uint32_t arraySize = desc.layerCount;
    if(!arraySize)
    {
        arraySize = desc_.arraySize - desc.baseArrayLayer;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc;
    viewDesc.Format = TranslateFormat(desc.format != Format::Unknown ? desc.format : desc_.format);
    if(desc_.dim == TextureDimension::Tex2D)
    {
        if(desc_.sampleCount == 1)
        {
            if(desc_.arraySize == 1)
            {
                assert(desc.baseArrayLayer == 0 && arraySize == 1);
                viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                viewDesc.Texture2D.MipSlice   = desc.mipLevel;
                viewDesc.Texture2D.PlaneSlice = 0;
            }
            else
            {
                viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                viewDesc.Texture2DArray.MipSlice        = desc.mipLevel;
                viewDesc.Texture2DArray.FirstArraySlice = desc.baseArrayLayer;
                viewDesc.Texture2DArray.ArraySize       = arraySize;
                viewDesc.Texture2DArray.PlaneSlice      = 0;
            }
        }
        else
        {
            if(desc_.arraySize == 1)
            {
                assert(desc.baseArrayLayer == 0 && arraySize == 1);
                assert(desc.mipLevel == 0);
                viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
                viewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                assert(desc.mipLevel == 0);
                viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                viewDesc.Texture2DMSArray.FirstArraySlice = desc.baseArrayLayer;
                viewDesc.Texture2DMSArray.ArraySize       = arraySize;
            }
        }
    }
    else
    {
        assert(desc_.dim == TextureDimension::Tex3D);
        assert(arraySize == 0 && desc.baseArrayLayer == 0);
        viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        viewDesc.Texture3D.MipSlice    = desc.mipLevel;
        viewDesc.Texture3D.FirstWSlice = 0;
        viewDesc.Texture3D.WSize       = desc_.depth;
    }

    auto handle = device_->_internalAllocateCPUDescriptorHandle_CbvSrvUav();
    device_->_internalGetNativeDevice()->CreateUnorderedAccessView(texture_.Get(), nullptr, &viewDesc, handle);
    uavs_.insert({ desc, handle });
    return MakeRPtr<DirectX12TextureUav>(desc, handle);
}

RPtr<TextureDsv> DirectX12Texture::CreateDsv(const TextureDsvDesc &desc) const
{
    {
        std::shared_lock lock(viewMutex_);
        if(auto it = dsvs_.find(desc); it != dsvs_.end())
        {
            return RPtr<TextureDsv>(new DirectX12TextureDsv(this, desc, it->second));
        }
    }

    std::lock_guard lock(viewMutex_);
    if(auto it = dsvs_.find(desc); it != dsvs_.end())
    {
        return RPtr<TextureDsv>(new DirectX12TextureDsv(this, desc, it->second));
    }
    
    D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc;
    viewDesc.Format = TranslateFormat(desc.format != Format::Unknown ? desc.format : desc_.format);
    viewDesc.Flags = D3D12_DSV_FLAG_NONE;

    assert(desc_.dim == TextureDimension::Tex2D);
    if(desc_.sampleCount == 1)
    {
        if(desc_.arraySize == 1)
        {
            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
            viewDesc.Texture2D.MipSlice = desc.mipLevel;
        }
        else
        {
            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
            viewDesc.Texture2DArray.MipSlice        = desc.mipLevel;
            viewDesc.Texture2DArray.FirstArraySlice = desc.arrayLayer;
            viewDesc.Texture2DArray.ArraySize       = 1;
        }
    }
    else
    {
        if(desc_.arraySize == 1)
        {
            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
            viewDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
        }
        else
        {
            // d3d12 doesn't support non-zero mipmap level in multi-sampled texture2d array rtv
            assert(desc.mipLevel == 0);
            viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
            viewDesc.Texture2DMSArray.FirstArraySlice = desc.arrayLayer;
            viewDesc.Texture2DMSArray.ArraySize       = 1;
        }
    }

    auto handle = device_->_internalAllocateCPUDescriptorHandle_Dsv();
    device_->_internalGetNativeDevice()->CreateDepthStencilView(texture_.Get(), &viewDesc, handle);
    dsvs_.insert({ desc, handle });
    return MakeRPtr<DirectX12TextureDsv>(this, desc, handle);
}

RTRC_RHI_D3D12_END
