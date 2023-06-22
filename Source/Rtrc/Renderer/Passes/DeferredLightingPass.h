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
        const PersistentSceneCameraRenderingData &scene,
        const RGScene                            &rgScene,
        RG::RenderGraph                          &renderGraph,
        RG::TextureResource                      *renderTarget);

private:

    enum class LightingPass
    {
        Regular,
        Sky
    };

    void DoDeferredLighting(
        const PersistentSceneCameraRenderingData  &sceneCamera,
        const RGScene       &rgScene,
        RG::TextureResource *renderTarget,
        RG::PassContext     &context);

    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;

    RC<BindingGroupLayout> perPassBindingGroupLayout_;
    RC<Shader> regularShader_;
    RC<Shader> skyShader_;
    
    UploadBufferPool<> lightBufferPool_;
};

RTRC_RENDERER_END
