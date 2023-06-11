#pragma once

#include <Rtrc/Graphics/RHI/DirectX12/Resource/Texture.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12TextureSrv, TextureSrv)
{
public:

    DirectX12TextureSrv(const TextureSrvDesc &desc, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
        : desc_(desc), cpuHandle_(cpuHandle)
    {
        
    }

    const TextureSrvDesc &GetDesc() const RTRC_RHI_OVERRIDE
    {
        return desc_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetDescriptorHandle() const
    {
        return cpuHandle_;
    }

private:

    TextureSrvDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
};

RTRC_RHI_IMPLEMENT(DirectX12TextureUav, TextureUav)
{
public:

    DirectX12TextureUav(const TextureUavDesc & desc, D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
        : desc_(desc), cpuHandle_(cpuHandle)
    {

    }

    const TextureUavDesc &GetDesc() const RTRC_RHI_OVERRIDE
    {
        return desc_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetDescriptorHandle() const
    {
        return cpuHandle_;
    }

private:

    TextureUavDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
};

RTRC_RHI_IMPLEMENT(DirectX12TextureRtv, TextureRtv)
{
public:

    DirectX12TextureRtv(
        const DirectX12Texture     *tex,
        const TextureRtvDesc       &desc,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
        : tex_(tex), desc_(desc), cpuHandle_(cpuHandle)
    {
        if(desc_.format == Format::Unknown)
        {
            desc_.format = tex_->GetDesc().format;
        }
    }

    const TextureRtvDesc &GetDesc() const RTRC_RHI_OVERRIDE
    {
        return desc_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetDescriptorHandle() const
    {
        return cpuHandle_;
    }

    const DirectX12Texture *_internalGetTexture() const
    {
        return tex_;
    }

private:

    const DirectX12Texture *tex_;
    TextureRtvDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;
};


RTRC_RHI_IMPLEMENT(DirectX12TextureDsv, TextureDsv)
{
public:

    DirectX12TextureDsv(
        const DirectX12Texture     *tex,
        const TextureDsvDesc       &desc,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle)
        : tex_(tex), desc_(desc), cpuHandle_(cpuHandle)
    {
        if(desc_.format == Format::Unknown)
        {
            desc_.format = tex_->GetDesc().format;
        }
    }

    const TextureDsvDesc &GetDesc() const RTRC_RHI_OVERRIDE
    {
        return desc_;
    }

    const D3D12_CPU_DESCRIPTOR_HANDLE &_internalGetDescriptorHandle() const
    {
        return cpuHandle_;
    }

    const DirectX12Texture *_internalGetTexture() const
    {
        return tex_;
    }

private:

    const DirectX12Texture *tex_;
    TextureDsvDesc desc_;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle_;

};

RTRC_RHI_D3D12_END
