#pragma once

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/RHI/D3D12/D3D12DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

class D3D12Device;
class D3D12Texture;

template<typename Base>
class D3D12TextureView : public Base
{
public:

    D3D12TextureView(ObserverPtr<D3D12Texture> texture, const D3D12CPUDescriptor &descriptor)
        : texture_(texture), descriptor_(descriptor)
    {
        
    }

    const D3D12CPUDescriptor &GetDescriptor() const { return descriptor_; }

private:

    ObserverPtr<D3D12Texture> texture_;
    D3D12CPUDescriptor descriptor_;
};

using D3D12TextureSRV = D3D12TextureView<TextureSRV>;
using D3D12TextureUAV = D3D12TextureView<TextureUAV>;
using D3D12TextureRTV = D3D12TextureView<TextureRTV>;
using D3D12TextureDSV = D3D12TextureView<TextureDSV>;

class D3D12Texture : public Texture
{
public:

    D3D12Texture(
        ObserverPtr<D3D12Device> device,
        ComPtr<ID3D12Resource> resource,
        D3D12MemoryAllocation allocation,
        const TextureDesc &desc);
    ~D3D12Texture() RTRC_RHI_OVERRIDE;

    const TextureDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    TextureSRVRef GetSRV(const TextureSRVDesc &desc) RTRC_RHI_OVERRIDE;
    TextureUAVRef GetUAV(const TextureUAVDesc &desc) RTRC_RHI_OVERRIDE;
    TextureRTVRef GetRTV(const TextureRTVDesc &desc) RTRC_RHI_OVERRIDE;
    TextureDSVRef GetDSV(const TextureDSVDesc &desc) RTRC_RHI_OVERRIDE;

private:

    static TextureSRVDesc PreprocessSRVDesc(const TextureDesc &texDesc, const TextureSRVDesc &rawSRVDesc);
    static TextureUAVDesc PreprocessUAVDesc(const TextureDesc &texDesc, const TextureUAVDesc &rawUAVDesc);
    static TextureRTVDesc PreprocessRTVDesc(const TextureDesc &texDesc, const TextureRTVDesc &rawRTVDesc);
    static TextureDSVDesc PreprocessDSVDesc(const TextureDesc &texDesc, const TextureDSVDesc &rawRTVDesc);

    ObserverPtr<D3D12Device> device_;
    D3D12MemoryAllocation    allocation_;
    ComPtr<ID3D12Resource>   resource_;
    TextureDesc              desc_;

    tbb::spin_rw_mutex viewMapLock_;
    std::map<TextureSRVDesc, ReferenceCountedPtr<D3D12TextureSRV>> SRVMap;
    std::map<TextureUAVDesc, ReferenceCountedPtr<D3D12TextureUAV>> UAVMap;
    std::map<TextureRTVDesc, ReferenceCountedPtr<D3D12TextureRTV>> RTVMap;
    std::map<TextureDSVDesc, ReferenceCountedPtr<D3D12TextureDSV>> DSVMap;
};

RTRC_RHI_D3D12_END
