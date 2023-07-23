#pragma once

#include <Rtrc/Renderer/DeferredLighting/ShadowMaskPass.h>
#include <Rtrc/Renderer/GBuffer/GBufferPass.h>

RTRC_RENDERER_BEGIN

class DeferredLightingPass : public Uncopyable
{
public:

    enum MainLightMode
    {
        MainLightMode_None = 0,
        MainLightMode_Point = 1,
        MainLightMode_Directional = 2,
    };

    DeferredLightingPass(ObserverPtr<Device> device, ObserverPtr<ResourceManager> resources);

    void Render(
        const RenderCamera  &scene,
        const GBuffers      &gbuffers,
        RG::TextureResource *skyLut,
        RG::TextureResource *indirectDiffuse,
        RG::RenderGraph     &renderGraph,
        RG::TextureResource *renderTarget);

private:

    enum class LightingPass
    {
        Regular,
        Sky
    };

    template<MainLightMode Mode>
    void DoDeferredLighting(
        const RenderCamera &sceneCamera,
        const GBuffers      &gbuffers,
        RG::TextureResource *skyLut,
        RG::TextureResource *indirectDiffuse,
        RG::TextureResource *shadowMask,
        RG::TextureResource *renderTarget);

    ObserverPtr<Device>          device_;
    ObserverPtr<ResourceManager> resources_;
    
    RC<Shader> regularShader_PointMainLight_;
    RC<Shader> regularShader_DirectionalMainLight_;
    RC<Shader> regularShader_NoMainLight_;
    RC<Shader> skyShader_;
    
    UploadBufferPool<> lightBufferPool_;

    Box<ShadowMaskPass> shadowMaskPass_;
};

RTRC_RENDERER_END
