#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

struct MaterialRenderingCache
{
    const LegacyMaterialInstance *materialInstance = nullptr;
    const LegacyMaterial *material = nullptr;

    BindlessTextureEntry albedoTextureEntry;
    float albedoScale = 1;

    int gbufferPassIndex = -1;
    bool supportGBufferInstancing = false;
};

class MaterialRenderingCacheManager : public Uncopyable
{
public:

    void Update(const Scene &scene);

    const MaterialRenderingCache *FindMaterialRenderingCache(const LegacyMaterialInstance *material) const;

private:

    std::vector<Box<MaterialRenderingCache>> cachedMaterials_;
};

RTRC_RENDERER_END
