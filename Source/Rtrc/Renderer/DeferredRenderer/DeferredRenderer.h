#pragma once

#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>
#include <Rtrc/Renderer/DeferredRenderer/GBufferRenderer.h>

RTRC_BEGIN

namespace RendererDetail
{

    enum class StencilMaskBit : uint8_t
    {
        Nothing = 0,
        Regular = 1 << 0
    };

    RTRC_DEFINE_ENUM_FLAGS(StencilMaskBit)

} // namespace RendererDetail

class DeferredRenderer : public Uncopyable
{
public:
    
    using StencilMask = RendererDetail::EnumFlagsStencilMaskBit;

    struct RenderGraphInterface
    {
        RG::Pass *passIn = nullptr;
        RG::Pass *passOut = nullptr;
    };

    DeferredRenderer(Device &device, const BuiltinResourceManager &builtinResources);

    AtmosphereRenderer &GetAtmosphereRenderer();

    RenderGraphInterface AddToRenderGraph(
        RG::RenderGraph     &renderGraph,
        RG::TextureResource *renderTarget,
        const SceneProxy    &scene,
        const Camera        &camera,
        float                deltaTime);

private:
    
    struct DeferredLightingPassData
    {
        GBufferRenderer::GBuffers gbuffers;
        RG::TextureResource *image = nullptr;
        RG::TextureResource *skyLut = nullptr;
    };
    
    void DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData);
    
    Device                       &device_;
    const BuiltinResourceManager &builtinResources_;

    Box<GBufferRenderer>    gbufferRenderer_;
    Box<AtmosphereRenderer> atmosphereRenderer_;
    
    RC<GraphicsPipeline> deferredLightingPipeline_;

    TransientConstantBufferAllocator transientConstantBufferAllocator_;
    DeferredRendererCommon::FrameContext frameContext_;

    Mesh fullscreenTriangleWithRays_;
    Mesh fullscreenQuadWithRays_;
};

RTRC_END
