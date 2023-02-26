#pragma once

#include <Rtrc/Graphics/Material/MaterialPassToGraphicsPipeline.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/AtmosphereRenderer.h>
#include <Rtrc/Renderer/Camera/Camera.h>
#include <Rtrc/Renderer/Scene/Scene.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>

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

    using StencilMaskBit = RendererDetail::StencilMaskBit;
    using StencilMask    = EnumFlags<StencilMaskBit>;

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

    struct RenderGBuffersPassData
    {
        RG::TextureResource *gbufferA     = nullptr;
        RG::TextureResource *gbufferB     = nullptr;
        RG::TextureResource *gbufferC     = nullptr;
        RG::TextureResource *gbufferDepth = nullptr;
    };

    struct DeferredLightingPassData
    {
        RG::TextureResource *gbufferA     = nullptr;
        RG::TextureResource *gbufferB     = nullptr;
        RG::TextureResource *gbufferC     = nullptr;
        RG::TextureResource *gbufferDepth = nullptr;
        RG::TextureResource *image        = nullptr;
        RG::TextureResource *skyLut       = nullptr;
    };

    void DoRenderGBuffersPass(RG::PassContext &passContext, const RenderGBuffersPassData &passData);

    void DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData);

    // Persistent

    Device                       &device_;
    const BuiltinResourceManager &builtinResources_;

    Box<AtmosphereRenderer> atmosphereRenderer_;

    MaterialPassToGraphicsPipeline renderGBuffersPipelines_;
    RC<GraphicsPipeline>           deferredLightingPipeline_;

    TransientConstantBufferAllocator transientConstantBufferAllocator_;

    // Per-frame

    const SceneProxy *scene_  = nullptr;
    const Camera     *camera_ = nullptr;

    Mesh fullscreenTriangleWithRays_;
    Mesh fullscreenQuadWithRays_;
};

RTRC_END
