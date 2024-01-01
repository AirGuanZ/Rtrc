#pragma once

#include <Rtrc/Graphics/Device/Texture/StatefulTexture.h>
#include <Rtrc/Core/Container/ObjectPool.h>

RTRC_BEGIN

class PooledTexture : public StatefulTexture
{
    friend class PooledTextureManager;
};

class PooledTextureManager : public Uncopyable, public TextureImpl::TextureManagerInterface
{
public:

    PooledTextureManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<PooledTexture> Create(const RHI::TextureDesc &desc);

    void _internalRelease(Texture &texture) override;

private:

    struct PooledRecord
    {
        RHI::TextureRPtr rhiTexture;
        TextureSubrscMap<TextureSubrscState> state;
    };

    RHI::DeviceOPtr device_;
    DeviceSynchronizer &sync_;

    ObjectPool<RHI::TextureDesc, PooledRecord, true, true> pool_;
};

RTRC_END
