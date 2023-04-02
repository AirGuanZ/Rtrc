#pragma once

#include <Rtrc/Renderer/Passes/GBufferPass.h>

RTRC_RENDERER_BEGIN

class DeferredLightingPass : public Uncopyable
{
public:

    using GBuffers = GBufferPass::GBuffers;

    struct RenderGraphOutput
    {
        RG::Pass *lightingPass = nullptr;
    };

    DeferredLightingPass(ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources);

    RenderGraphOutput RenderDeferredLighting(
        const CachedScenePerCamera &scene,
        RG::RenderGraph            &renderGraph,
        const GBuffers             &gbuffers,
        RG::TextureResource        *renderTarget);

private:

    enum class LightingPass
    {
        Regular,
        Sky
    };

    void DoDeferredLighting(
        const CachedScenePerCamera &scene,
        const GBuffers             &rgGBuffers,
        RG::TextureResource        *renderTarget,
        RG::PassContext            &context);

    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;

    RC<BindingGroupLayout> perPassBindingGroupLayout_;
    RC<ShaderTemplate>     regularShaderTemplate_;
    RC<ShaderTemplate>     skyShaderTemplate_;

    GraphicsPipeline::Desc pipelineTemplate_;
    PipelineCache          pipelineCache_;

    UploadBufferPool<> lightBufferPool_;
};

RTRC_RENDERER_END
