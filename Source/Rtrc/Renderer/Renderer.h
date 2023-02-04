#pragma once

#include <Rtrc/Graphics/Material/MaterialPassToGraphicsPipeline.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Graphics/Resource/BuiltinResources.h>
#include <Rtrc/Renderer/Camera/Camera.h>
#include <Rtrc/Renderer/Scene/Scene.h>
#include <Rtrc/Renderer/Utility/PerObjectConstantBufferUtility.h>

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

class Renderer : public Uncopyable
{
public:

    using StencilMaskBit = RendererDetail::StencilMaskBit;
    using StencilMask = EnumFlags<StencilMaskBit>;

    struct RenderGraphInterface
    {
        RG::Pass *inPass = nullptr;
        RG::Pass *outPass = nullptr;
    };

    struct Parameters
    {
        RG::TextureResource *skyLut;
    };

    Renderer(Device &device, const BuiltinResourceManager &builtinResources);

    RenderGraphInterface AddToRenderGraph(
        const Parameters    &parameters,
        RG::RenderGraph     *renderGraph,
        RG::TextureResource *renderTarget,
        const Scene         &scene,
        const Camera        &camera);

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

    Device &device_;
    const BuiltinResourceManager &builtinResources_;

    MaterialPassToGraphicsPipeline renderGBuffersPipelines_;
    
    RC<GraphicsPipeline> deferredLightingPipeline_;

    PerObjectConstantBufferBatch staticMeshConstantBufferBatch_;

    // Per-frame

    const Scene  *scene_  = nullptr;
    const Camera *camera_ = nullptr;

    Mesh fullscreenTriangleWithRays_;
    Mesh fullscreenQuadWithRays_;
};

RTRC_END
