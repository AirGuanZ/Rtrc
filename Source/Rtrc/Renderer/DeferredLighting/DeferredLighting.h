#pragma once

#include <Rtrc/Renderer/Common.h>

RTRC_RENDERER_BEGIN

class DeferredLighting : public RenderAlgorithm
{
public:

    using RenderAlgorithm::RenderAlgorithm;

    void Render(
        ObserverPtr<RenderCamera>    renderCamera,
        ObserverPtr<RG::RenderGraph> renderGraph,
        RG::TextureResource         *renderTarget) const;
};

RTRC_RENDERER_END
