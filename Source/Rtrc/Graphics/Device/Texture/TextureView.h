#pragma once

#include <Rtrc/Graphics/Device/Texture/Texture.h>

RTRC_BEGIN

template<typename T>
class TTextureView
{
public:

    TTextureView() = default;
    TTextureView(
        RC<Texture> texture, uint32_t mipLevel, uint32_t arrayLayer, bool isArrayView, RHI::TextureViewFlag flags = 0);
    // for levelCount/layerCount, 0 means all
    TTextureView(
        RC<Texture>          texture,
        uint32_t             mipLevel,
        uint32_t             levelCount,
        uint32_t             arrayLayer,
        uint32_t             layerCount,
        bool                 isArrayView,
        RHI::TextureViewFlag flags = 0);

    const T &GetRHIObject() const;

    operator typename T::ElementType *() const;

    const RC<Texture> &GetTexture() const { return texture_; }

private:

    static_assert(
        std::is_same_v<T, RHI::TextureSrvPtr> ||
        std::is_same_v<T, RHI::TextureUavPtr> ||
        std::is_same_v<T, RHI::TextureRtvPtr> ||
        std::is_same_v<T, RHI::TextureDsvPtr>);

    RC<Texture> texture_;
    T view_;
};

template<typename T>
TTextureView<T>::TTextureView(
    RC<Texture> texture, uint32_t mipLevel, uint32_t arrayLayer, bool isArrayView, RHI::TextureViewFlag flags)
    : TTextureView(std::move(texture), mipLevel, arrayLayer, 0, isArrayView ? 0 : 1, isArrayView, flags)
{
    
}

template<typename T>
TTextureView<T>::TTextureView(
    RC<Texture>          texture,
    uint32_t             mipLevel,
    uint32_t             levelCount,
    uint32_t             arrayLayer,
    uint32_t             layerCount,
    bool                 isArrayView,
    RHI::TextureViewFlag flags)
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
    assert((std::is_same_v<T, RHI::TextureSrvPtr> || std::is_same_v<T, RHI::TextureDsvPtr>) || flags.GetInteger() == 0);

    if constexpr(std::is_same_v<T, RHI::TextureSrvPtr>)
    {
        const RHI::TextureSrvDesc desc =
        {
            .isArray        = isArrayView,
            .format         = RHI::Format::Unknown,
            .baseMipLevel   = mipLevel,
            .levelCount     = levelCount,
            .baseArrayLayer = arrayLayer,
            .layerCount     = layerCount,
            .flags          = flags
        };
        view_ = texture_->GetRHIObject()->CreateSrv(desc);
    }
    else if constexpr(std::is_same_v<T, RHI::TextureUavPtr>)
    {
        assert(levelCount == 1);
        const RHI::TextureUavDesc desc =
        {
            .isArray        = isArrayView,
            .format         = RHI::Format::Unknown,
            .mipLevel       = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount     = layerCount
        };
        view_ = texture_->GetRHIObject()->CreateUav(desc);
    }
    else if constexpr(std::is_same_v<T, RHI::TextureRtvPtr>)
    {
        assert(levelCount == 1 && layerCount == 1 && !isArrayView);
        const RHI::TextureRtvDesc desc =
        {
            .format     = RHI::Format::Unknown,
            .mipLevel   = mipLevel,
            .arrayLayer = arrayLayer
        };
        view_ = texture_->GetRHIObject()->CreateRtv(desc);
    }
    else
    {
        static_assert(std::is_same_v<T, RHI::TextureDsvPtr>);
        assert(levelCount == 1 && layerCount == 1 && !isArrayView);
        const RHI::TextureDsvDesc desc =
        {
            .format     = RHI::Format::Unknown,
            .mipLevel   = mipLevel,
            .arrayLayer = arrayLayer,
            .flags      = flags
        };
        view_ = texture_->GetRHIObject()->CreateDsv(desc);
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

inline TextureSrv Texture::CreateSrv(RHI::TextureViewFlag flags)
{
    if(GetDesc().arraySize > 1)
    {
        return CreateSrv(0, 0, 0, 0, flags);
    }
    return CreateSrv(0, 0, 0, flags);
}

inline TextureSrv Texture::CreateSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, RHI::TextureViewFlag flags)
{
    return TextureSrv(shared_from_this(), mipLevel, levelCount, arrayLayer, 1, false, flags);
}

inline TextureSrv Texture::CreateSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount, RHI::TextureViewFlag flags)
{
    return TextureSrv(shared_from_this(), mipLevel, levelCount, arrayLayer, layerCount, true, flags);
}

inline TextureUav Texture::CreateUav()
{
    if(GetDesc().arraySize > 1)
    {
        return CreateUav(0, 0, 0);
    }
    return CreateUav(0, 0);
}

inline TextureUav Texture::CreateUav(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureUav(shared_from_this(), mipLevel, 1, arrayLayer, 1, false);
}

inline TextureUav Texture::CreateUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    return TextureUav(shared_from_this(), mipLevel, 1, arrayLayer, layerCount, true);
}

inline TextureRtv Texture::CreateRtv(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureRtv(shared_from_this(), mipLevel, arrayLayer, false);
}

inline TextureDsv Texture::CreateDsv(RHI::TextureViewFlag flags)
{
    return CreateDsv(0, 0, flags);
}

inline TextureDsv Texture::CreateDsv(uint32_t mipLevel, uint32_t arrayLayer, RHI::TextureViewFlag flags)
{
    return TextureDsv(shared_from_this(), mipLevel, arrayLayer, false, flags);
}

RTRC_END
