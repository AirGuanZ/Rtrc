#pragma once

#include <Rtrc/Renderer/Passes/GBufferPass.h>

RTRC_RENDERER_BEGIN

class DeferredLightingPass : public Uncopyable
{
public:
    
    struct RenderGraphOutput
    {
        RG::Pass *lightingPass = nullptr;
    };

    DeferredLightingPass(ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources);

    RenderGraphOutput RenderDeferredLighting(
        const CachedCamera &scene,
        const RGScene              &rgScene,
        RG::RenderGraph            &renderGraph,
        RG::TextureResource        *renderTarget);

private:

    enum class LightingPass
    {
        Regular,
        Sky
    };

    void DoDeferredLighting(
        const CachedCamera  &sceneCamera,
        const RGScene       &rgScene,
        RG::TextureResource *renderTarget,
        RG::PassContext     &context);

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
