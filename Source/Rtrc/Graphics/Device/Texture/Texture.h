#pragma once

#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Core/Container/ObjectCache.h>

RTRC_BEGIN

class Texture;
template<typename T>
class TTextureView;

using TextureSrv = TTextureView<RHI::TextureSrvRPtr>;
using TextureUav = TTextureView<RHI::TextureUavRPtr>;
using TextureRtv = TTextureView<RHI::TextureRtvRPtr>;
using TextureDsv = TTextureView<RHI::TextureDsvRPtr>;

namespace TextureImpl
{

    class TextureManagerInterface;

    struct TextureData
    {
        RHI::TextureDesc desc_;
        RHI::TextureRPtr rhiTexture_;

        TextureManagerInterface *manager_ = nullptr;
    };

    class TextureManagerInterface
    {
    public:

        virtual ~TextureManagerInterface() = default;
        virtual void _internalRelease(Texture &texture) = 0;

    protected:

        static TextureData &GetTextureData(Texture &texture);
    };

} // namespace TextureImpl

class Texture :
    protected TextureImpl::TextureData,
    public std::enable_shared_from_this<Texture>,
    public InObjectCache
{
public:

    static RC<Texture> FromRHIObject(RHI::TextureRPtr rhiTexture);

    ~Texture() override;

    void SetName(std::string name);

    const RHI::TextureRPtr &GetRHIObject() const;
    const RHI::TextureDesc &GetDesc() const;

    RHI::TextureDimension GetDimension() const;
    RHI::Format GetFormat() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    Vector2u GetSize() const;
    uint32_t GetWidth(uint32_t mipLevel) const;
    uint32_t GetHeight(uint32_t mipLevel) const;
    Vector2u GetSize(uint32_t mipLevel) const;
    uint32_t GetDepth() const;
    uint32_t GetArraySize() const;
    uint32_t GetMipmapLevelCount() const;

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureSrv GetSrv();
    // non-array view
    TextureSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer);
    // array view
    TextureSrv GetSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount);

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureUav GetUav();
    // non-array view
    TextureUav GetUav(uint32_t mipLevel, uint32_t arrayLayer);
    // array view
    TextureUav GetUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount);
    
    TextureRtv GetRtv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
    
    TextureDsv GetDsv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

private:

    friend class TextureImpl::TextureManagerInterface;
};

class TextureManager : public Uncopyable, public TextureImpl::TextureManagerInterface
{
public:

    TextureManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<Texture> Create(const RHI::TextureDesc &desc);

    void _internalRelease(Texture &texture) override;

private:

    RHI::DeviceOPtr device_;
    DeviceSynchronizer &sync_;
};

inline TextureImpl::TextureData &TextureImpl::TextureManagerInterface::GetTextureData(Texture &texture)
{
    return texture;
}

inline RC<Texture> Texture::FromRHIObject(RHI::TextureRPtr rhiTexture)
{
    auto ret = MakeRC<Texture>();
    ret->desc_ = rhiTexture->GetDesc();
    ret->rhiTexture_ = std::move(rhiTexture);
    return ret;
}

inline Texture::~Texture()
{
    if(manager_)
    {
        manager_->_internalRelease(*this);
    }
}

inline void Texture::SetName(std::string name)
{
    rhiTexture_->SetName(std::move(name));
}

inline const RHI::TextureRPtr &Texture::GetRHIObject() const
{
    return rhiTexture_;
}

inline const RHI::TextureDesc &Texture::GetDesc() const
{
    return desc_;
}

inline RHI::TextureDimension Texture::GetDimension() const
{
    return desc_.dim;
}

inline RHI::Format Texture::GetFormat() const
{
    return desc_.format;
}

inline uint32_t Texture::GetWidth() const
{
    return desc_.width;
}

inline uint32_t Texture::GetHeight() const
{
    return desc_.height;
}

inline Vector2u Texture::GetSize() const
{
    return { GetWidth(), GetHeight() };
}

inline uint32_t Texture::GetWidth(uint32_t mipLevel) const
{
    return std::max(desc_.width >> mipLevel, 1u);
}

inline uint32_t Texture::GetHeight(uint32_t mipLevel) const
{
    return std::max(desc_.height >> mipLevel, 1u);
}

inline Vector2u Texture::GetSize(uint32_t mipLevel) const
{
    return { GetWidth(mipLevel), GetHeight(mipLevel) };
}

inline uint32_t Texture::GetDepth() const
{
    return desc_.depth;
}

inline uint32_t Texture::GetArraySize() const
{
    return desc_.arraySize;
}

inline uint32_t Texture::GetMipmapLevelCount() const
{
    return desc_.mipLevels;
}

inline RHI::Viewport Texture::GetViewport(float minDepth, float maxDepth) const
{
    return RHI::Viewport::Create(desc_, minDepth, maxDepth);
}

inline RHI::Scissor Texture::GetScissor() const
{
    return RHI::Scissor::Create(desc_);
}

RTRC_END
