#pragma once

#include <Rtrc/Renderer/Passes/GBufferPass.h>

RTRC_RENDERER_BEGIN

class DeferredLightingPass : public Uncopyable
{
public:

    using GBuffers = GBufferPass::GBuffers;

    struct RenderGraphInput
    {
        GBuffers             inGBuffers;
        RG::TextureResource *outRenderTarget;
    };

    struct RenderGraphOutput
    {
        RG::Pass *lightingPass = nullptr;
    };

    DeferredLightingPass(ObserverPtr<Device> device, ObserverPtr<BuiltinResourceManager> builtinResources);

    RenderGraphOutput RenderDeferredLighting(
        const CachedScenePerCamera &scene,
        RG::RenderGraph            &renderGraph,
        const RenderGraphInput     &rgInput);

private:

    void DoDeferredLighting(
        const CachedScenePerCamera &scene,
        const RenderGraphInput     &rgInput,
        RG::PassContext            &context);

    ObserverPtr<Device>                 device_;
    ObserverPtr<BuiltinResourceManager> builtinResources_;

    RC<BindingGroupLayout> perPassBindingGroupLayout_;
    RC<ShaderTemplate>     shaderTemplate_;

    GraphicsPipeline::Desc pipelineTemplate_;
    PipelineCache          pipelineCache_;

    UploadBufferPool<> lightBufferPool_;
};

RTRC_RENDERER_END