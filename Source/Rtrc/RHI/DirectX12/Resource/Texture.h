#pragma once

#include <map>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_D3D12_BEGIN

RTRC_RHI_IMPLEMENT(DirectX12Texture, Texture)
{
public:

#ifdef RTRC_STATIC_RHI
    RTRC_RHI_TEXTURE_COMMON
#endif

    RTRC_D3D12_IMPL_SET_NAME(texture_)

    DirectX12Texture(
        const TextureDesc        &desc,
        DirectX12Device          *device,
        ComPtr<ID3D12Resource>    texture,
        DirectX12MemoryAllocation alloc);

    ~DirectX12Texture() override;

    const TextureDesc &GetDesc() const RTRC_RHI_OVERRIDE { return desc_; }

    RPtr<TextureRtv> CreateRtv(const TextureRtvDesc &desc) const RTRC_RHI_OVERRIDE;
    RPtr<TextureSrv> CreateSrv(const TextureSrvDesc &desc) const RTRC_RHI_OVERRIDE;
    RPtr<TextureUav> CreateUav(const TextureUavDesc &desc) const RTRC_RHI_OVERRIDE;
    RPtr<TextureDsv> CreateDsv(const TextureDsvDesc &desc) const RTRC_RHI_OVERRIDE;

    const ComPtr<ID3D12Resource> &_internalGetNativeTexture() const { return texture_; }

private:

    TextureDesc      desc_;
    DirectX12Device *device_;

    DirectX12MemoryAllocation alloc_;
    ComPtr<ID3D12Resource>    texture_;

    mutable std::map<TextureSrvDesc, D3D12_CPU_DESCRIPTOR_HANDLE> srvs_;
    mutable std::map<TextureUavDesc, D3D12_CPU_DESCRIPTOR_HANDLE> uavs_;
    mutable std::map<TextureRtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE> rtvs_;
    mutable std::map<TextureDsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE> dsvs_;
    mutable tbb::spin_rw_mutex viewMutex_;
};

RTRC_RHI_D3D12_END
