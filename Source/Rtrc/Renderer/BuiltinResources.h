#pragma once

#include <Rtrc/Graphics/Material/Material.h>
#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

enum class BuiltinMaterial
{
    DeferredLighting,
    Count
};

class BuiltinResourceManager : public Uncopyable
{
public:

    explicit BuiltinResourceManager(Device &device);

    const RC<Material> &GetBuiltinMaterial(BuiltinMaterial material) const;

private:

    void LoadBuiltinMaterials();

    Device &device_;

    MaterialManager materialManager_;
    std::array<RC<Material>, EnumCount<BuiltinMaterial>> materials_;
};

RTRC_END
