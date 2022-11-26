#pragma once

#include <Rtrc/Graphics/Device/Texture/Texture.h>

RTRC_BEGIN

template<typename T>
class TTextureView
{
public:

    TTextureView() = default;
    TTextureView(RC<Texture> texture, uint32_t mipLevel, uint32_t arrayLayer, bool isArrayView);
    // for levelCount/layerCount, 0 means all
    TTextureView(
        RC<Texture> texture,
        uint32_t    mipLevel,
        uint32_t    levelCount,
        uint32_t    arrayLayer,
        uint32_t    layerCount,
        bool        isArrayView);

    const T &GetRHIObject() const;

    operator typename T::ElementType *() const;

private:

    static_assert(
        std::is_same_v<T, RHI::TextureSRVPtr> ||
        std::is_same_v<T, RHI::TextureUAVPtr> ||
        std::is_same_v<T, RHI::TextureRTVPtr> ||
        std::is_same_v<T, RHI::TextureDSVPtr>);

    RC<Texture> texture_;
    T view_;
};

template<typename T>
TTextureView<T>::TTextureView(RC<Texture> texture, uint32_t mipLevel, uint32_t arrayLayer, bool isArrayView)
    : TTextureView(std::move(texture), mipLevel, arrayLayer, 0, isArrayView ? 0 : 1, isArrayView)
{
    
}

template<typename T>
TTextureView<T>::TTextureView(
    RC<Texture> texture,
    uint32_t    mipLevel,
    uint32_t    levelCount,
    uint32_t    arrayLayer,
    uint32_t    layerCount,
    bool        isArrayView)
    : texture_(std::move(texture))
{
    if(levelCount == 0)
    {
        levelCount = texture_->GetDesc().mipLevels;
    }
    if(layerCount == 0)
    {
        layerCount = texture_->GetDesc().arraySize;
    }
    assert(isArrayView || layerCount == 1);

    if constexpr(std::is_same_v<T, RHI::TextureSRVPtr>)
    {
        const RHI::TextureSRVDesc desc =
        {
            .isArray = isArrayView,
            .format = RHI::Format::Unknown,
            .baseMipLevel = mipLevel,
            .levelCount = levelCount,
            .baseArrayLayer = arrayLayer,
            .layerCount = layerCount
        };
        view_ = texture_->GetRHIObject()->CreateSRV(desc);
    }
    else if constexpr(std::is_same_v<T, RHI::TextureUAVPtr>)
    {
        assert(levelCount == 1);
        const RHI::TextureUAVDesc desc =
        {
            .isArray = isArrayView,
            .format = RHI::Format::Unknown,
            .mipLevel = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount = layerCount
        };
        view_ = texture_->GetRHIObject()->CreateUAV(desc);
    }
    else if constexpr(std::is_same_v<T, RHI::TextureRTVPtr>)
    {
        assert(levelCount == 1 && layerCount == 1 && !isArrayView);
        const RHI::TextureRTVDesc desc =
        {
            .format = RHI::Format::Unknown,
            .mipLevel = mipLevel,
            .arrayLayer = arrayLayer
        };
        view_ = texture_->GetRHIObject()->CreateRTV(desc);
    }
    else
    {
        static_assert(std::is_same_v<T, RHI::TextureDSVPtr>);
        assert(levelCount == 1 && layerCount == 1 && !isArrayView);
        const RHI::TextureDSVDesc desc =
        {
            .format = RHI::Format::Unknown,
            .mipLevel = mipLevel,
            .arrayLayer = arrayLayer
        };
        view_ = texture_->GetRHIObject()->CreateDSV(desc);
    }
}

template<typename T>
const T &TTextureView<T>::GetRHIObject() const
{
    return view_;
}

template<typename T>
TTextureView<T>::operator typename T::ElementType *() const
{
    return view_.Get();
}

inline TextureSRV Texture::CreateSRV()
{
    if(GetDesc().arraySize > 1)
    {
        return CreateSRV(0, 0, 0, 0);
    }
    return CreateSRV(0, 0, 0);
}

inline TextureSRV Texture::CreateSRV(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer)
{
    return TextureSRV(shared_from_this(), mipLevel, levelCount, arrayLayer, 1, false);
}

inline TextureSRV Texture::CreateSRV(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount)
{
    return TextureSRV(shared_from_this(), mipLevel, levelCount, arrayLayer, layerCount, true);
}

inline TextureUAV Texture::CreateUAV()
{
    if(GetDesc().arraySize > 1)
    {
        return CreateUAV(0, 0, 0);
    }
    return CreateUAV(0, 0);
}

inline TextureUAV Texture::CreateUAV(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureUAV(shared_from_this(), mipLevel, 1, arrayLayer, 1, false);
}

inline TextureUAV Texture::CreateUAV(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    return TextureUAV(shared_from_this(), mipLevel, 1, arrayLayer, layerCount, true);
}

inline TextureRTV Texture::CreateRTV(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureRTV(shared_from_this(), mipLevel, arrayLayer, false);
}

inline TextureDSV Texture::CreateDSV(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureDSV(shared_from_this(), mipLevel, arrayLayer, false);
}

RTRC_END
