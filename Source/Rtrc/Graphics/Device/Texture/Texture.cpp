#include <Rtrc/Graphics/Device/Texture/Texture.h>

RTRC_BEGIN

TextureManager::TextureManager(RHI::DevicePtr device, DeviceSynchronizer &sync)
    : device_(std::move(device)), sync_(sync)
{
    
}

RC<Texture> TextureManager::Create(const RHI::TextureDesc &desc)
{
    auto tex = MakeRC<Texture>();
    auto &texData = GetTextureData(*tex);
    texData.desc_ = desc;
    texData.rhiTexture_ = device_->CreateTexture(desc);
    texData.manager_ = this;
    return tex;
}

void TextureManager::_internalRelease(Texture &texture)
{
    auto &texData = GetTextureData(texture);
    assert(texData.manager_ == this);
    sync_.OnFrameComplete([rhiTexture = std::move(texData.rhiTexture_)] {});
}

RTRC_END
