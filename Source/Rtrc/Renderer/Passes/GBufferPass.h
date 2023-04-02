#pragma once

#include <Rtrc/Graphics/Misc/PipelineCache.h>
#include <Rtrc/Renderer/Scene/CachedScene.h>

RTRC_RENDERER_BEGIN

class GBufferPass : public Uncopyable
{
public:

    struct GBuffers
    {
        // gbufferA: normal
        // gbufferB: albedo, metallic
        // gbufferC: roughness

        RG::TextureResource *a     = nullptr;
        RG::TextureResource *b     = nullptr;
        RG::TextureResource *c     = nullptr;
        RG::TextureResource *depth = nullptr;
    };

    struct RenderGraphOutput
    {
        GBuffers gbuffers;
        RG::Pass *gbufferPass = nullptr;
    };

    explicit GBufferPass(ObserverPtr<Device> device);

    RenderGraphOutput RenderGBuffers(
        const CachedScenePerCamera &scene,
        RG::RenderGraph            &renderGraph,
        const Vector2u             &rtSize);

private:

    GBuffers AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize);

    void DoRenderGBuffers(RG::PassContext &passContext, const CachedScenePerCamera &scene, const GBuffers &gbuffers);

    ObserverPtr<Device>    device_;
    PipelineCache          gbufferPipelineCache_;
    RC<BindingGroupLayout> perPassBindingGroupLayout_;
};

RTRC_RENDERER_END
