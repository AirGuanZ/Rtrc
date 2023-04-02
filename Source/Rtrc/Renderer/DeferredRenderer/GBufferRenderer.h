#pragma once

#include <Rtrc/Graphics/Material/GraphicsPipelineCache.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Renderer/DeferredRenderer/Common.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>

RTRC_RENDERER_BEGIN

class GBufferRenderer : public Uncopyable
{
public:

    struct GBuffers
    {
        // gbufferA: normal
        // gbufferB: albedo, metallic
        // gbufferC: roughness

        RG::TextureResource *a     = nullptr;
        RG::TextureResource *b     = nullptr;
        RG::TextureResource *c     = nullptr;
        RG::TextureResource *depth = nullptr;
    };

    struct RenderGraphInterface
    {
        GBuffers  gbuffers;
        RG::Pass *gbufferPass = nullptr;
    };

    explicit GBufferRenderer(Device &device);
    
    RenderGraphInterface AddToRenderGraph(
        DeferredRendererCommon::FrameContext &frameContext,
        RG::RenderGraph                      &renderGraph,
        const Vector2u                       &rtSize);

private:

    GBuffers AllocateGBuffers(RG::RenderGraph &renderGraph, const Vector2u &rtSize) const;
    
    void DoRenderGBufferPass(
        RG::PassContext                      &passContext,
        DeferredRendererCommon::FrameContext &frameContext,
        const GBuffers                       &gbuffers);

    Device &device_;

    GraphicsPipelineCache renderGBuffersPipelines_;
};

RTRC_RENDERER_END
