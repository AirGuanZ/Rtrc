#pragma once

#include <Rtrc/Graphics/Pipeline/SubMaterialToGraphicsPipeline.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Renderer/Camera.h>
#include <Rtrc/Renderer/Scene.h>
#include <Rtrc/Renderer/Utility/PerObjectConstantBufferUtility.h>

RTRC_BEGIN

class DeferredRenderer : public Uncopyable
{
public:

    DeferredRenderer(Device &device, MaterialManager &materialManager);

    void AddToRenderGraph(
        RG::RenderGraph     *renderGraph,
        RG::TextureResource *renderTarget,
        const Scene         &scene,
        const Camera        &camera);

private:

    struct RenderGBuffersPassData
    {
        RG::TextureResource *normal         = nullptr;
        RG::TextureResource *albedoMetallic = nullptr;
        RG::TextureResource *roughness      = nullptr;
        RG::TextureResource *depth          = nullptr;
    };

    struct DeferredLightingPassData
    {
        RG::TextureResource *normal         = nullptr;
        RG::TextureResource *albedoMetallic = nullptr;
        RG::TextureResource *roughness      = nullptr;
        RG::TextureResource *depth          = nullptr;
        RG::TextureResource *image          = nullptr;
    };

    void DoRenderGBuffersPass(RG::PassContext &passContext, const RenderGBuffersPassData &passData);

    void DoDeferredLightingPass(RG::PassContext &passContext, const DeferredLightingPassData &passData);

    // Persistent

    Device &device_;
    MaterialManager &materialManager_;

    SubMaterialToGraphicsPipeline renderGBuffersPipelines_;
    RC<GraphicsPipeline> deferredLightingPipeline_;

    PerObjectConstantBufferBatch staticMeshConstantBufferBatch_;

    // Per-frame

    RG::RenderGraph     *renderGraph_  = nullptr;
    RG::TextureResource *renderTarget_ = nullptr;

    const Scene  *scene_  = nullptr;
    const Camera *camera_ = nullptr;

    Mesh fullscreenTriangleWithRays_;
    Mesh fullscreenQuadWithRays_;
};

RTRC_END
