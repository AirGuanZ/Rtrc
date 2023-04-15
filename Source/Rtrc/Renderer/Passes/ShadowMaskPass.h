#pragma once

#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/Scene/CachedScene.h>

RTRC_RENDERER_BEGIN

class ShadowMaskPass : public Uncopyable
{
public:

    struct RenderGraphOutput
    {
        RG::Pass            *shadowMaskPass = nullptr;
        RG::TextureResource *shadowMask = nullptr;
    };

    ShadowMaskPass(
        ObserverPtr<Device>                       device,
        ObserverPtr<const BuiltinResourceManager> builtinResources);

    RenderGraphOutput RenderShadowMask(
        const CachedScenePerCamera &scene,
        int                         lightIndex,
        const RGScene              &rgScene,
        RG::RenderGraph            &renderGraph);

private:
    
    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;

    RC<Shader>             shader_;
    RC<BindingGroupLayout> passBindingGroupLayout_;
};

RTRC_RENDERER_END
