#pragma once

#include <Rtrc/Graphics/Misc/PipelineCache.h>
#include <Rtrc/Renderer/Scene/PersistentSceneRenderingData.h>

RTRC_RENDERER_BEGIN

class GBufferPass : public Uncopyable
{
public:

    struct RenderGraphOutput
    {
        GBuffers gbuffers;
        RG::Pass *gbufferPass = nullptr;
    };

    explicit GBufferPass(ObserverPtr<Device> device);

    RenderGraphOutput RenderGBuffers(
        const PersistentSceneCameraRenderingData     &sceneCamera,
        const RC<BindingGroup> &bindlessTextureGroup,
        RG::RenderGraph        &renderGraph,
        const Vector2u         &rtSize);

private:

    struct MeshGroup
    {
        const Mesh::SharedRenderingData *meshRenderingData = nullptr;
        uint32_t                         objectDataOffset  = 0;
        uint32_t                         objectCount       = 0;
    };

    struct MaterialInstanceGroup
    {
        const MaterialInstance::SharedRenderingData *materialRenderingData = nullptr;
        std::vector<MeshGroup>                       meshGroups;
    };

    struct MaterialGroup
    {
        const Material                    *material          = nullptr;
        ShaderTemplate                    *shaderTemplate    = nullptr;
        bool                               supportInstancing = false;
        int                                gbufferPassIndex  = 0;
        std::vector<MaterialInstanceGroup> materialInstanceGroups;
    };

    GBuffers AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize);

    std::vector<MaterialGroup> CollectPipelineGroups(const PersistentSceneCameraRenderingData &scene) const;

    void DoRenderGBuffers(
        RG::PassContext        &passContext,
        const PersistentSceneCameraRenderingData     &scene,
        const RC<BindingGroup> &bindlessTextureGroup,
        const GBuffers         &gbuffers);

    ObserverPtr<Device>    device_;
    PipelineCache          gbufferPipelineCache_;
    RC<BindingGroupLayout> perPassBindingGroupLayout_;
};

RTRC_RENDERER_END
