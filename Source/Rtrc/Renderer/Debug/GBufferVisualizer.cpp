#include <Rtrc/Renderer/Debug/GBufferVisualizer.h>
#include <Rtrc/Renderer/GBufferBinding.h>

RTRC_RENDERER_BEGIN

void GBufferVisualizer::Render(
    Mode                 mode,
    RG::RenderGraph     &renderGraph,
    const GBuffers      &gbuffers,
    RG::TextureResource *renderTarget)
{
    rtrc_group(Pass)
    {
        rtrc_inline(GBufferBindings_All, gbuffers);
        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2, outputResolution);
    };

    auto pass = renderGraph.CreatePass("VisualizeGBuffer");
    DeclareGBufferUses<Pass>(pass, gbuffers, RHI::PipelineStage::ComputeShader);
    pass->Use(renderTarget, RG::CS_RWTexture_WriteOnly);
    pass->SetCallback([mode, gbuffers, renderTarget, this]
    {
        Pass passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.Output = renderTarget;
        passData.outputResolution = renderTarget->GetSize();
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        RC<Shader> shader;
        if(mode == Mode::Normal)
        {
            shader = resources_->GetMaterialManager()->GetCachedShader<"Builtin/Debug/VisualizeNormal">();
        }
        else
        {
            throw Exception(fmt::format("GBufferVisualizer: unknown mode {}", std::to_underlying(mode)));
        }

        auto &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(renderTarget->GetSize());
    });
}

RTRC_RENDERER_END
