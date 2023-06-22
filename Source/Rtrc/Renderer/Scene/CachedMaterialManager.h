#pragma once

#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class CachedMaterialManager : public Uncopyable
{
public:

    struct CachedMaterial
    {
        UniqueId        materialId = {};
        const Material *material   = nullptr;

        UniqueId                                     materialRenderingDataId = {};
        const MaterialInstance::SharedRenderingData *materialRenderingData   = nullptr;
        
        const BindlessTextureEntry *albedoTextureEntry = nullptr;
        float                       albedoScale = 1;

        int  gbufferPassIndex             = -1;;
        bool gbufferPassSupportInstancing = false;
    };
    
    void UpdateCachedMaterialData(const RenderCommand_RenderStandaloneFrame &frame);

    const CachedMaterial *FindCachedMaterial(UniqueId materialId) const;

private:

    std::list<Box<CachedMaterial>> cachedMaterials_;
    std::vector<CachedMaterial *>  linearCachedMaterials_;
};

RTRC_RENDERER_END
