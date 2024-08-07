#pragma once

#include <Rtrc/Graphics/Device/Texture/Texture.h>

RTRC_BEGIN

template<typename T>
class TTextureView
{
public:

    TTextureView() = default;

    TTextureView(
        RC<Texture> texture,
        uint32_t    mipLevel,
        uint32_t    arrayLayer,
        bool        isArrayView);

    // For levelCount/layerCount, 0 means all
    TTextureView(
        RC<Texture> texture,
        uint32_t    mipLevel,
        uint32_t    levelCount,
        uint32_t    arrayLayer,
        uint32_t    layerCount,
        bool        isArrayView);

    const T &GetRHIObject() const;

    operator T() const;

    const RC<Texture> &GetTexture() const { return texture_; }

    RHI::Format GetFinalFormat() const;

private:

    static_assert(
        std::is_same_v<T, RHI::TextureSrvRPtr> ||
        std::is_same_v<T, RHI::TextureUavRPtr> ||
        std::is_same_v<T, RHI::TextureRtvRPtr> ||
        std::is_same_v<T, RHI::TextureDsvRPtr>);

    RC<Texture> texture_;
    T view_;
};

template<typename T>
TTextureView<T>::TTextureView(
    RC<Texture> texture,
    uint32_t    mipLevel,
    uint32_t    arrayLayer,
    bool        isArrayView)
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
    
    if constexpr(std::is_same_v<T, RHI::TextureSrvRPtr>)
    {
        const RHI::TextureSrvDesc desc =
        {
            .isArray        = isArrayView,
            .format         = RHI::Format::Unknown,
            .baseMipLevel   = mipLevel,
            .levelCount     = levelCount,
            .baseArrayLayer = arrayLayer,
            .layerCount     = layerCount,
        };
        view_ = texture_->GetRHIObject()->CreateSrv(desc);
    }
    else if constexpr(std::is_same_v<T, RHI::TextureUavRPtr>)
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
    else if constexpr(std::is_same_v<T, RHI::TextureRtvRPtr>)
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
        static_assert(std::is_same_v<T, RHI::TextureDsvRPtr>);
        assert(levelCount == 1 && layerCount == 1 && !isArrayView);
        const RHI::TextureDsvDesc desc =
        {
            .format     = RHI::Format::Unknown,
            .mipLevel   = mipLevel,
            .arrayLayer = arrayLayer,
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
TTextureView<T>::operator T() const
{
    return view_;
}

template<typename T>
RHI::Format TTextureView<T>::GetFinalFormat() const
{
    RHI::Format ret = GetRHIObject().GetDesc().format;
    if(ret == RHI::Format::Unknown)
    {
        ret = texture_->GetFormat();
    }
    return ret;
}

inline TextureSrv Texture::GetSrv()
{
    if(GetDesc().arraySize > 1)
    {
        return GetSrv(0, 0, 0, 0);
    }
    return GetSrv(0, 0, 0);
}

inline TextureSrv Texture::GetSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer)
{
    return TextureSrv(shared_from_this(), mipLevel, levelCount, arrayLayer, 1, false);
}

inline TextureSrv Texture::GetSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount)
{
    return TextureSrv(shared_from_this(), mipLevel, levelCount, arrayLayer, layerCount, true);
}

inline TextureUav Texture::GetUav()
{
    if(GetDesc().arraySize > 1)
    {
        return GetUav(0, 0, 0);
    }
    return GetUav(0, 0);
}

inline TextureUav Texture::GetUav(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureUav(shared_from_this(), mipLevel, 1, arrayLayer, 1, false);
}

inline TextureUav Texture::GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    return TextureUav(shared_from_this(), mipLevel, 1, arrayLayer, layerCount, true);
}

inline TextureRtv Texture::GetRtv(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureRtv(shared_from_this(), mipLevel, arrayLayer, false);
}

inline TextureDsv Texture::GetDsv(uint32_t mipLevel, uint32_t arrayLayer)
{
    return TextureDsv(shared_from_this(), mipLevel, arrayLayer, false);
}

RTRC_END
