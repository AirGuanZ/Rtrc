#include <Rtrc/Renderer/BuiltinResources.h>

RTRC_BEGIN

BuiltinResourceManager::BuiltinResourceManager(Device &device)
    : device_(device)
{
    LoadBuiltinMaterials();
}

const RC<Material> &BuiltinResourceManager::GetBuiltinMaterial(BuiltinMaterial material) const
{
    return materials_[static_cast<int>(material)];
}

void BuiltinResourceManager::LoadBuiltinMaterials()
{
    materialManager_.SetDebugMode(RTRC_DEBUG);
    materialManager_.SetDevice(&device_);
    materialManager_.SetRootDirectory("./Asset/Builtin/Material/");

#define LOAD_BUILTIN_MATERIAL(NAME) \
    materials_[EnumToInt(BuiltinMaterial::NAME)] = materialManager_.GetMaterial("Builtin/" #NAME)

    LOAD_BUILTIN_MATERIAL(DeferredLighting);

#undef LOAD_BUILTIN_MATERIAL
}

RTRC_END
