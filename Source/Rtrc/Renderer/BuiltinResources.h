#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

enum class BuiltinMaterial
{
    DeferredLighting,
    Count
};

enum class BuiltinTexture
{
    RGBA_0_2D,
    RGB_0_A_255_2D,
    RGBA_255_2D,
    Count
};

class BuiltinResourceManager : public Uncopyable
{
public:

    explicit BuiltinResourceManager(Device &device);

    const RC<Material> &GetBuiltinMaterial(BuiltinMaterial material) const;

private:

    void LoadBuiltinMaterials();

    void LoadBuiltinTextures();

    Device &device_;

    MaterialManager materialManager_;
    std::array<RC<Material>, EnumCount<BuiltinMaterial>> materials_;
    std::array<RC<Texture>, EnumCount<BuiltinTexture>> textures_;
};

RTRC_END
