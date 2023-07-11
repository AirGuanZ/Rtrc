#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/Scene/RenderScene.h>

RTRC_RENDERER_BEGIN

class ShadowMaskPass : public Uncopyable
{
public:
    
    ShadowMaskPass(
        ObserverPtr<Device>                 device,
        ObserverPtr<BuiltinResourceManager> builtinResources);
    
    RG::TextureResource *Render(
        const RenderCamera               &camera,
        const Light::SharedRenderingData *light,
        const GBuffers                   &gbuffers,
        RG::RenderGraph                  &renderGraph);

private:

    void InitializeShaders();
    
    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;
    
    RC<Shader> collectLowResShadowMaskShader_;
    
    RC<Shader>             blurLowResXShader_;
    RC<Shader>             blurLowResYShader_;
    
    RC<Shader>             blurXShader_;
    RC<Shader>             blurYShader_;
};

RTRC_RENDERER_END
