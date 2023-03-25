#pragma once

#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class RenderLoopMaterialManager : public Uncopyable
{
public:

    struct CachedMaterial
    {
        MaterialInstance::SharedRenderingData::UniqueID materialId = {};
        const MaterialInstance::SharedRenderingData    *material = nullptr;
        
        const BindlessTextureEntry *albedoTextureEntry;
    };
    
    void UpdateCachedMaterialData(const RenderCommand_RenderStandaloneFrame &frame);

    const CachedMaterial *FindCachedMaterial(MaterialInstance::SharedRenderingData::UniqueID materialId) const;

private:

    std::list<Box<CachedMaterial>> cachedMaterials_;
    std::vector<CachedMaterial *>  linearCachedMaterials_;
};

RTRC_RENDERER_END
