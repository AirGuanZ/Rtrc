#include <Graphics/Device/Texture/PooledTexture.h>

RTRC_BEGIN

PooledTextureManager::PooledTextureManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync)
    : device_(device), sync_(sync)
{
    
}

RC<PooledTexture> PooledTextureManager::Create(const RHI::TextureDesc &desc)
{
    auto ret = MakeRC<PooledTexture>();
    auto &texData = GetTextureData(*ret);
    texData.manager_ = this;
    texData.desc_ = desc;
    if(auto record = pool_.Get(desc))
    {
        texData.rhiTexture_ = std::move(record->rhiTexture);
        ret->state_ = std::move(record->state);
    }
    else
    {
        texData.rhiTexture_ = device_->CreateTexture(desc).ToRC();
        ret->state_ = TextureSubrscMap<TextureSubrscState>(desc);
    }
    return ret;
}

void PooledTextureManager::_internalRelease(Texture &texture)
{
    auto &pooledTexture = static_cast<PooledTexture &>(texture);
    auto &texData = GetTextureData(pooledTexture);

    auto record = MakeBox<PooledRecord>();
    record->rhiTexture = std::move(texData.rhiTexture_);
    record->state = std::move(pooledTexture.state_);

    auto handle = pool_.Insert(texData.desc_, std::move(record));
    sync_.OnFrameComplete([h = std::move(handle)] {});
}

RTRC_END
