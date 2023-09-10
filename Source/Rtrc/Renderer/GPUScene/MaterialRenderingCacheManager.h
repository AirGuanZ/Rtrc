#pragma once

#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

struct MaterialRenderingCache
{
    const MaterialInstance *materialInstance = nullptr;
    const Material *material = nullptr;

    BindlessTextureEntry albedoTextureEntry;
    float albedoScale = 1;

    int gbufferPassIndex = -1;
    bool supportGBufferInstancing = false;
};

class MaterialRenderingCacheManager : public Uncopyable
{
public:

    void Update(const Scene &scene);

    const MaterialRenderingCache *FindMaterialRenderingCache(const MaterialInstance *material) const;

private:

    std::vector<Box<MaterialRenderingCache>> cachedMaterials_;
};

RTRC_RENDERER_END
