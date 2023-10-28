#include <Rtrc/Renderer/Debug/GBufferVisualizer.h>
#include <Rtrc/Renderer/Debug/Shader/GBufferVisualizer.shader.outh>
#include <Rtrc/Renderer/GBufferBinding.h>

RTRC_RENDERER_BEGIN

void GBufferVisualizer::Render(
    Mode                 mode,
    RG::RenderGraph     &renderGraph,
    const GBuffers      &gbuffers,
    RG::TextureResource *renderTarget)
{
    RC<Shader> shader;
    if(mode == Mode::Normal)
    {
        shader = resources_->GetMaterialManager()->GetCachedShader<"VisualizeNormal">();
    }
    else
    {
        throw Exception(fmt::format("GBufferVisualizer: unknown mode {}", std::to_underlying(mode)));
    }

    StaticShaderInfo<"VisualizeNormal">::Pass passData;
    FillBindingGroupGBuffers(passData, gbuffers);
    passData.Output           = renderTarget;
    passData.Output.writeOnly = true;
    passData.outputResolution = renderTarget->GetSize();

    renderGraph.CreateComputePassWithThreadCount(
        "VisualizeGBuffer",
        shader,
        renderTarget->GetSize(),
        passData);
}

RTRC_RENDERER_END
