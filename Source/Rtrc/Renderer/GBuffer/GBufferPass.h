#pragma once

#include <Graphics/Misc/PipelineCache.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/GPUScene/RenderScene.h>

RTRC_RENDERER_BEGIN

class GBufferPass : public Uncopyable
{
public:

    explicit GBufferPass(ObserverPtr<Device> device);

    void Render(
        const RenderCamera     &sceneCamera,
        const RC<BindingGroup> &bindlessTextureGroup,
        RG::RenderGraph        &renderGraph);

private:

    struct MeshGroup
    {
        const MeshRenderingCache *meshCache        = nullptr;
        uint32_t                  objectDataOffset = 0;
        uint32_t                  objectCount      = 0;
    };

    struct MaterialInstanceGroup
    {
        const MaterialRenderingCache *materialCache = nullptr;
        std::vector<MeshGroup>        meshGroups;
    };

    struct MaterialGroup
    {
        const LegacyMaterial                    *material          = nullptr;
        ShaderTemplate                    *shaderTemplate    = nullptr;
        bool                               supportInstancing = false;
        int                                gbufferPassIndex  = 0;
        std::vector<MaterialInstanceGroup> materialInstanceGroups;
    };
    
    std::vector<MaterialGroup> CollectPipelineGroups(const RenderCamera &scene) const;

    void DoRenderGBuffers(
        const RenderCamera     &camera,
        const RC<BindingGroup> &bindlessTextureGroup,
        const GBuffers         &gbuffers);

    ObserverPtr<Device> device_;
    PipelineCache       gbufferPipelineCache_;
};

RTRC_RENDERER_END
