#pragma once

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Texture.h>

RTRC_BEGIN

class Device;

enum class BuiltinTexture
{
    Black2D,      // R8G8B8A8_UNorm, RGB = 0, A = 1
    White2D,      // R8G8B8A8_UNorm, RGB = 1, A = 1
    BlueNoise256, // R8_UNorm, 256 * 256
    Count
};

enum class BuiltinMesh
{
    Cube,
    Count
};

class ResourceManager : public Uncopyable
{
public:
    
    explicit ResourceManager(Ref<Device> device);

    Ref<Device> GetDevice() const { return device_; }
    
    const RC<Texture> &GetBuiltinTexture(BuiltinTexture texture) const;

    const RC<Buffer> &GetPoissonDiskSamples2048() const;

private:

    void LoadBuiltinTextures();
    void GeneratePoissonDiskSamples();

    Ref<Device> device_;
    
    std::array<RC<Texture>, EnumCount<BuiltinTexture>> textures_;

    RC<Buffer> poissonDiskSamples2048_;
};

inline const RC<Texture> &ResourceManager::GetBuiltinTexture(BuiltinTexture texture) const
{
    return textures_[std::to_underlying(texture)];
}

inline const RC<Buffer> &ResourceManager::GetPoissonDiskSamples2048() const
{
    return poissonDiskSamples2048_;
}

RTRC_END
