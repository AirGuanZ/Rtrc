#include <ranges>
#include <shared_mutex>

#include <Rtrc/RHI/D3D12/D3D12Device.h>
#include <Rtrc/RHI/D3D12/D3D12Texture.h>

RTRC_RHI_D3D12_BEGIN

D3D12Texture::D3D12Texture(
    ObserverPtr<D3D12Device> device,
    ComPtr<ID3D12Resource> resource,
    D3D12MemoryAllocation allocation,
    const TextureDesc &desc)
    : device_(device)
    , allocation_(std::move(allocation))
    , resource_(std::move(resource))
    , desc_(desc)
{
    
}

D3D12Texture::~D3D12Texture()
{
    for(auto &view : std::views::values(SRVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, view->GetDescriptor());
    }
    for(auto &view : std::views::values(UAVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, view->GetDescriptor());
    }
    for(auto &view : std::views::values(RTVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, view->GetDescriptor());
    }
    for(auto &view : std::views::values(DSVMap))
    {
        device_->FreeCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, view->GetDescriptor());
    }
}

const TextureDesc &D3D12Texture::GetDesc() const
{
    return desc_;
}

TextureSRVRef D3D12Texture::GetSRV(const TextureSRVDesc &rawDesc)
{
    const auto desc = PreprocessSRVDesc(desc_, rawDesc);

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
    d3d12Desc.Format = FormatToD3D12SRVFormat(desc.format);
    d3d12Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


    switch(desc_.dimension)
    {
    case TextureDimension::Tex1D:
    {
        if(desc.isArray)
        {
            d3d12Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
            d3d12Desc.Texture1DArray.ArraySize           = desc.arrayLayers;
            d3d12Desc.Texture1DArray.FirstArraySlice     = desc.firstArrayLayer;
            d3d12Desc.Texture1DArray.MipLevels           = desc.mipLevels;
            d3d12Desc.Texture1DArray.MostDetailedMip     = desc.firstMipLevel;
            d3d12Desc.Texture1DArray.ResourceMinLODClamp = 0;
        }
        else
        {
            d3d12Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE1D;
            d3d12Desc.Texture1D.MipLevels           = desc.mipLevels;
            d3d12Desc.Texture1D.MostDetailedMip     = desc.firstMipLevel;
            d3d12Desc.Texture1D.ResourceMinLODClamp = 0;
        }
        break;
    }
    case TextureDimension::Tex2D:
    {
        if(desc.isArray)
        {
            if(IsMSAAEnabled())
            {
                d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
                d3d12Desc.Texture2DMSArray.ArraySize       = desc.arrayLayers;
                d3d12Desc.Texture2DMSArray.FirstArraySlice = desc.firstArrayLayer;
            }
            else
            {
                d3d12Desc.ViewDimension                      = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                d3d12Desc.Texture2DArray.ArraySize           = desc.arrayLayers;
                d3d12Desc.Texture2DArray.FirstArraySlice     = desc.firstArrayLayer;
                d3d12Desc.Texture2DArray.MipLevels           = desc.mipLevels;
                d3d12Desc.Texture2DArray.MostDetailedMip     = desc.firstMipLevel;
                d3d12Desc.Texture2DArray.ResourceMinLODClamp = 0;
                d3d12Desc.Texture2DArray.PlaneSlice          = 0;
            }
        }
        else
        {
            if(IsMSAAEnabled())
            {
                d3d12Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
                d3d12Desc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                d3d12Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
                d3d12Desc.Texture2D.MipLevels           = desc.mipLevels;
                d3d12Desc.Texture2D.MostDetailedMip     = desc.firstMipLevel;
                d3d12Desc.Texture2D.ResourceMinLODClamp = 0;
                d3d12Desc.Texture2D.PlaneSlice          = 0;
            }
        }
        break;
    }
    case TextureDimension::Tex3D:
    {
        assert(!desc.isArray);
        d3d12Desc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE3D;
        d3d12Desc.Texture3D.MipLevels           = desc.mipLevels;
        d3d12Desc.Texture3D.MostDetailedMip     = desc.firstMipLevel;
        d3d12Desc.Texture3D.ResourceMinLODClamp = 0;
        break;
    }
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device_->GetNativeDevice()->CreateShaderResourceView(resource_.Get(), &d3d12Desc, descriptor.handle);

    auto SRV = MakeReferenceCountedPtr<D3D12TextureSRV>(this, descriptor);
    SRVMap.insert({ desc, SRV });
    return SRV;
}

TextureUAVRef D3D12Texture::GetUAV(const TextureUAVDesc &rawDesc)
{
    const auto desc = PreprocessUAVDesc(desc_, rawDesc);

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
    d3d12Desc.Format = FormatToD3D12UAVFormat(desc.format);

    switch(desc_.dimension)
    {
    case TextureDimension::Tex1D:
    {
        if(desc.isArray)
        {
            d3d12Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
            d3d12Desc.Texture1DArray.FirstArraySlice = desc.firstArrayLayer;
            d3d12Desc.Texture1DArray.ArraySize       = desc.arrayLayers;
            d3d12Desc.Texture1DArray.MipSlice        = desc.mipLevel;
        }
        else
        {
            d3d12Desc.ViewDimension      = D3D12_UAV_DIMENSION_TEXTURE1D;
            d3d12Desc.Texture1D.MipSlice = desc.mipLevel;
        }
        break;
    }
    case TextureDimension::Tex2D:
    {
        if(desc.isArray)
        {
            if(IsMSAAEnabled())
            {
                d3d12Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY;
                d3d12Desc.Texture2DMSArray.ArraySize       = desc.arrayLayers;
                d3d12Desc.Texture2DMSArray.FirstArraySlice = desc.firstArrayLayer;
            }
            else
            {
                d3d12Desc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                d3d12Desc.Texture2DArray.FirstArraySlice = desc.firstArrayLayer;
                d3d12Desc.Texture2DArray.ArraySize       = desc.arrayLayers;
                d3d12Desc.Texture2DArray.MipSlice        = desc.mipLevel;
                d3d12Desc.Texture2DArray.PlaneSlice      = 0;
            }
        }
        else
        {
            if(IsMSAAEnabled())
            {
                d3d12Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DMS;
                d3d12Desc.Texture2DMS.UnusedField_NothingToDefine = 0;
            }
            else
            {
                d3d12Desc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
                d3d12Desc.Texture2D.MipSlice   = desc.mipLevel;
                d3d12Desc.Texture2D.PlaneSlice = 0;
            }
        }
        break;
    }
    case TextureDimension::Tex3D:
    {
        assert(!desc.isArray);
        d3d12Desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
        d3d12Desc.Texture3D.FirstWSlice = desc.firstDepthLayer;
        d3d12Desc.Texture3D.WSize       = desc.depthLayers;
        d3d12Desc.Texture3D.MipSlice    = desc.mipLevel;
        break;
    }
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    device_->GetNativeDevice()->CreateUnorderedAccessView(resource_.Get(), nullptr, &d3d12Desc, descriptor.handle);

    auto UAV = MakeReferenceCountedPtr<D3D12TextureUAV>(this, descriptor);
    UAVMap.insert({ desc, UAV });
    return UAV;
}

TextureRTVRef D3D12Texture::GetRTV(const TextureRTVDesc &rawDesc)
{
    const auto desc = PreprocessRTVDesc(desc_, rawDesc);

    {
        std::shared_lock lock(viewMapLock_);
        if(auto it = RTVMap.find(desc); it != RTVMap.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewMapLock_);
    if(auto it = RTVMap.find(desc); it != RTVMap.end())
    {
        return it->second;
    }

    assert(desc_.dimension == TextureDimension::Tex2D);
    assert(desc.mipLevel < desc_.mipLevels);

    D3D12_RENDER_TARGET_VIEW_DESC d3d12Desc = {};
    d3d12Desc.Format = FormatToD3D12RTVFormat(desc.format);

    if(IsMSAAEnabled())
    {
        d3d12Desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        d3d12Desc.Texture2DMS.UnusedField_NothingToDefine = 0;
    }
    else
    {
        d3d12Desc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
        d3d12Desc.Texture2D.MipSlice   = desc.mipLevel;
        d3d12Desc.Texture2D.PlaneSlice = 0;
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    device_->GetNativeDevice()->CreateRenderTargetView(resource_.Get(), &d3d12Desc, descriptor.handle);

    auto RTV = MakeReferenceCountedPtr<D3D12TextureRTV>(this, descriptor);
    RTVMap.insert({ desc, RTV });
    return RTV;
}

TextureDSVRef D3D12Texture::GetDSV(const TextureDSVDesc &rawDesc)
{
    const auto desc = PreprocessDSVDesc(desc_, rawDesc);

    {
        std::shared_lock lock(viewMapLock_);
        if(auto it = DSVMap.find(desc); it != DSVMap.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewMapLock_);
    if(auto it = DSVMap.find(desc); it != DSVMap.end())
    {
        return it->second;
    }

    assert(desc_.dimension == TextureDimension::Tex2D);
    assert(desc.mipLevel < desc_.mipLevels);

    D3D12_DEPTH_STENCIL_VIEW_DESC d3d12Desc = {};
    d3d12Desc.Format = FormatToD3D12DSVFormat(desc.format);
    d3d12Desc.Flags  = D3D12_DSV_FLAG_NONE;

    if(IsMSAAEnabled())
    {
        d3d12Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        d3d12Desc.Texture2DMS.UnusedField_NothingToDefine = 0;
    }
    else
    {
        d3d12Desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        d3d12Desc.Texture2D.MipSlice = desc.mipLevel;
    }

    if(desc.isDepthReadOnly)
    {
        d3d12Desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    }
    if(desc.isStencilReadOnly)
    {
        d3d12Desc.Flags |= D3D12_DSV_FLAG_READ_ONLY_STENCIL;
    }

    auto descriptor = device_->AllocateCPUDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    device_->GetNativeDevice()->CreateDepthStencilView(resource_.Get(), &d3d12Desc, descriptor.handle);

    auto DSV = MakeReferenceCountedPtr<D3D12TextureDSV>(this, descriptor);
    DSVMap.insert({ desc, DSV });
    return DSV;
}

TextureSRVDesc D3D12Texture::PreprocessSRVDesc(const TextureDesc &texDesc, const TextureSRVDesc &rawSRVDesc)
{
    TextureSRVDesc SRVDesc = rawSRVDesc;
    if(SRVDesc.format == Format::Unknown)
    {
        SRVDesc.format = texDesc.format;
    }
    if(SRVDesc.isArray && SRVDesc.arrayLayers == 0)
    {
        assert(SRVDesc.firstArrayLayer < texDesc.arrayLayers);
        SRVDesc.arrayLayers = texDesc.arrayLayers - SRVDesc.firstArrayLayer;
    }
    if(SRVDesc.mipLevels == 0)
    {
        assert(SRVDesc.firstMipLevel < texDesc.mipLevels);
        SRVDesc.mipLevels = texDesc.mipLevels - SRVDesc.firstMipLevel;
    }
    assert(SRVDesc.isArray || (SRVDesc.firstArrayLayer == 0 && SRVDesc.arrayLayers == 0));
    assert(SRVDesc.firstArrayLayer + SRVDesc.arrayLayers <= texDesc.arrayLayers);
    assert(SRVDesc.firstMipLevel + SRVDesc.mipLevels <= texDesc.mipLevels);
    return SRVDesc;
}

TextureUAVDesc D3D12Texture::PreprocessUAVDesc(const TextureDesc &texDesc, const TextureUAVDesc &rawUAVDesc)
{
    TextureUAVDesc ret = rawUAVDesc;
    if(ret.format == Format::Unknown)
    {
        ret.format = texDesc.format;
    }
    if(ret.isArray && ret.arrayLayers == 0)
    {
        assert(ret.firstArrayLayer < texDesc.arrayLayers);
        ret.arrayLayers = texDesc.arrayLayers - ret.firstArrayLayer;
    }
    if(texDesc.dimension == TextureDimension::Tex3D && ret.depthLayers == 0)
    {
        assert(ret.firstDepthLayer < texDesc.depth);
        ret.depthLayers = texDesc.depth - ret.firstDepthLayer;
    }
    assert(ret.isArray || (ret.firstArrayLayer == 0 && ret.arrayLayers == 0));
    assert(ret.firstArrayLayer + ret.arrayLayers <= texDesc.arrayLayers);
    assert(texDesc.dimension != TextureDimension::Tex3D || (ret.firstDepthLayer == 0 && ret.depthLayers == 0));
    return ret;
}

TextureRTVDesc D3D12Texture::PreprocessRTVDesc(const TextureDesc &texDesc, const TextureRTVDesc &rawRTVDesc)
{
    TextureRTVDesc ret = rawRTVDesc;
    if(ret.format == Format::Unknown)
    {
        ret.format = texDesc.format;
    }
    return ret;
}

TextureDSVDesc D3D12Texture::PreprocessDSVDesc(const TextureDesc &texDesc, const TextureDSVDesc &rawRTVDesc)
{
    TextureDSVDesc ret = rawRTVDesc;
    if(ret.format == Format::Unknown)
    {
        ret.format = texDesc.format;
    }
    return ret;
}

RTRC_RHI_D3D12_END
