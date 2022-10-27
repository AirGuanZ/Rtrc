#pragma once

#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Renderer/Camera.h>
#include <Rtrc/Renderer/Scene.h>

RTRC_BEGIN

class DeferredRenderer : public Uncopyable
{
public:

    void Render(
        RG::RenderGraph     *renderGraph,
        RG::TextureResource *renderTarget,
        const Scene         &scene,
        const RenderCamera  &camera);

private:

    RG::RenderGraph     *renderGraph_  = nullptr;
    RG::TextureResource *renderTarget_ = nullptr;

    const Scene        *scene_  = nullptr;
    const RenderCamera *camera_ = nullptr;

    RG::TextureResource *normal_         = nullptr;
    RG::TextureResource *albedoMetallic_ = nullptr;
    RG::TextureResource *roughness_      = nullptr;
};

RTRC_END
