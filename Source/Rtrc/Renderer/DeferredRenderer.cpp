#include <Rtrc/Renderer/DeferredRenderer.h>

RTRC_BEGIN

void DeferredRenderer::Render(
    RG::RenderGraph     *renderGraph,
    RG::TextureResource *renderTarget,
    const Scene         &scene,
    const RenderCamera  &camera)
{
    assert(!renderGraph_ && renderGraph);
    assert(!renderTarget_ && renderTarget);
    renderGraph_ = renderGraph;
    renderTarget_ = renderTarget;
    scene_ = &scene;
    camera_ = &camera;

    const uint32_t rtWidth = renderTarget->GetWidth();
    const uint32_t rtHeight = renderTarget->GetHeight();

    // TODO
    normal_ = renderGraph_->CreateTexture2D(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R10G10B10A2_UNorm,
        .width                = rtWidth,
        .height               = rtHeight,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
}

RTRC_END
