#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/Scene/RenderScene.h>

RTRC_RENDERER_BEGIN

class ShadowMaskPass : public Uncopyable
{
public:
    
    ShadowMaskPass(
        ObserverPtr<Device>                       device,
        ObserverPtr<const BuiltinResourceManager> builtinResources);
    
    RG::TextureResource *Render(
        const RenderSceneCamera          &sceneCamera,
        const Light::SharedRenderingData *light,
        bool                              enableLowResOptimization,
        const GBuffers                   &gbuffers,
        RG::RenderGraph                  &renderGraph);

private:

    void InitializeShaders();
    
    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;
    
    RC<Shader>             collectLowResShadowMaskShader_;
    RC<BindingGroupLayout> collectLowResShadowMaskBindingGroupLayout_;

    RC<Shader>             blurLowResXShader_;
    RC<Shader>             blurLowResYShader_;
    RC<BindingGroupLayout> blurLowResShadowMaskBindingGroupLayout_;
    
    RC<BindingGroupLayout> shadowMaskPassBindingGroupLayout_;

    RC<Shader>             blurXShader_;
    RC<Shader>             blurYShader_;
    RC<BindingGroupLayout> blurPassBindingGroupLayout_;
};

RTRC_RENDERER_END
