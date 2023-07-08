#pragma once

#include <Rtrc/Renderer/RenderCommand.h>

RTRC_RENDERER_BEGIN

class RenderMaterials : public Uncopyable
{
public:

    struct RenderMaterial
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

    const RenderMaterial *FindCachedMaterial(UniqueId materialId) const;

private:

    std::list<Box<RenderMaterial>> cachedMaterials_;
    std::vector<RenderMaterial *>  linearCachedMaterials_;
};

RTRC_RENDERER_END
