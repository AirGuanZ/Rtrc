#include <Rtrc/Renderer/DeferredLighting/DeferredLighting.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/GBufferBinding.h>

#include <Rtrc/Renderer/DeferredLighting/Shader/DeferredLighting.shader.outh>

RTRC_RENDERER_BEGIN

void DeferredLighting::Render(
    ObserverPtr<RenderCamera>    renderCamera,
    ObserverPtr<RG::RenderGraph> renderGraph,
    RG::TextureResource         *directIllum,
    RG::TextureResource         *sky,
    RG::TextureResource         *renderTarget) const
{
    StaticShaderInfo<"DeferredLighting">::Pass passData;
    FillBindingGroupGBuffers(passData, renderCamera->GetGBuffers());
    passData.Camera             = renderCamera->GetCameraCBuffer();
    passData.DirectIllumination = directIllum;
    passData.SkyLut             = sky;
    passData.Output             = renderTarget;
    passData.outputResolution   = renderTarget->GetSize();

    renderGraph->CreateComputePassWithThreadCount(
        "DeferredLighting",
        resources_->GetMaterialManager()->GetCachedShader<"DeferredLighting">(),
        renderTarget->GetSize(),
        passData);
}

RTRC_RENDERER_END
