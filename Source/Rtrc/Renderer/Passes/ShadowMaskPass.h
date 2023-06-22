#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/Scene/PersistentSceneRenderingData.h>

RTRC_RENDERER_BEGIN

class ShadowMaskPass : public Uncopyable
{
public:

    struct RenderGraphOutput
    {
        RG::TextureResource *shadowMask = nullptr;
    };

    ShadowMaskPass(
        ObserverPtr<Device>                       device,
        ObserverPtr<const BuiltinResourceManager> builtinResources);
    
    RenderGraphOutput Render(
        const PersistentSceneCameraRenderingData &sceneCamera,
        int                 lightIndex,
        bool                enableLowResOptimization,
        const RGScene      &rgScene,
        RG::RenderGraph    &renderGraph);

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
