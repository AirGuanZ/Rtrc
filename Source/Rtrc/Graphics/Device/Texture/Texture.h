#pragma once

#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>

RTRC_BEGIN

class Texture;
template<typename T>
class TTextureView;

using TextureSrv = TTextureView<RHI::TextureSrvPtr>;
using TextureUav = TTextureView<RHI::TextureUavPtr>;
using TextureRtv = TTextureView<RHI::TextureRtvPtr>;
using TextureDsv = TTextureView<RHI::TextureDsvPtr>;

namespace TextureImpl
{

    class TextureManagerInterface;

    struct TextureData
    {
        RHI::TextureDesc desc_;
        RHI::TexturePtr rhiTexture_;

        TextureManagerInterface *manager_ = nullptr;
        void *managerSpecificData_ = nullptr;
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

class Texture : protected TextureImpl::TextureData, public Uncopyable, public std::enable_shared_from_this<Texture>
{
public:

    static RC<Texture> FromRHIObject(RHI::TexturePtr rhiTexture);

    virtual ~Texture();

    void SetName(std::string name);

    const RHI::TexturePtr &GetRHIObject() const;
    const RHI::TextureDesc &GetDesc() const;

    RHI::TextureDimension GetDimension() const;
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    uint32_t GetDepth() const;
    uint32_t GetArraySize() const;
    uint32_t GetMipmapLevelCount() const;

    TextureSrv CreateSrv(RHI::TextureSrvFlag flags = 0); // non-array view for single-layer texture, array view for multi-layer texture
    TextureSrv CreateSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, RHI::TextureSrvFlag flags = 0);                      // non-array view
    TextureSrv CreateSrv(
        uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount, RHI::TextureSrvFlag flags = 0); // array view
    
    TextureUav CreateUav(); // non-array view for single-layer texture, array view for multi-layer texture
    TextureUav CreateUav(uint32_t mipLevel, uint32_t arrayLayer);                      // non-array view
    TextureUav CreateUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount); // array view

    TextureRtv CreateRtv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
    TextureDsv CreateDsv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

private:

    friend class TextureImpl::TextureManagerInterface;
};

class TextureManager : public Uncopyable, public TextureImpl::TextureManagerInterface
{
public:

    TextureManager(RHI::DevicePtr device, DeviceSynchronizer &sync);

    RC<Texture> Create(const RHI::TextureDesc &desc);

    void _internalRelease(Texture &texture) override;

private:

    RHI::DevicePtr device_;
    DeviceSynchronizer &sync_;
};

inline TextureImpl::TextureData &TextureImpl::TextureManagerInterface::GetTextureData(Texture &texture)
{
    return texture;
}

inline RC<Texture> Texture::FromRHIObject(RHI::TexturePtr rhiTexture)
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

inline const RHI::TexturePtr &Texture::GetRHIObject() const
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

inline uint32_t Texture::GetWidth() const
{
    return desc_.width;
}

inline uint32_t Texture::GetHeight() const
{
    return desc_.height;
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
